#include "tcp_minnow_socket.hh"

#include "exception.hh"
#include "parser.hh"
#include "tun.hh"

#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

static constexpr size_t TCP_TICK_MS = 10;

inline uint64_t timestamp_ms()
{
  static_assert( std::is_same<std::chrono::steady_clock::duration, std::chrono::nanoseconds>::value );

  return std::chrono::steady_clock::now().time_since_epoch().count() / 1000000;
}

//! \param[in] condition is a function returning true if loop should continue
template<TCPDatagramAdapter AdaptT>
void TCPMinnowSocket<AdaptT>::_tcp_loop( const std::function<bool()>& condition )
{
  auto base_time = timestamp_ms();
  while ( condition() ) {
    auto ret = _eventloop.wait_next_event( TCP_TICK_MS );
    if ( ret == EventLoop::Result::Exit or _abort ) {
      break;
    }

    if ( not _tcp.has_value() ) {
      throw std::runtime_error( "_tcp_loop entered before TCPPeer initialized" );
    }

    if ( _tcp.value().active() ) {
      const auto next_time = timestamp_ms();
      _tcp.value().tick( next_time - base_time, [&]( auto x ) { _datagram_adapter.write( x ); } );
      _datagram_adapter.tick( next_time - base_time );
      base_time = next_time;
    }
  }
}

//! \param[in] data_socket_pair is a pair of connected AF_UNIX SOCK_STREAM sockets
//! \param[in] datagram_interface is the interface for reading and writing datagrams
template<TCPDatagramAdapter AdaptT>
TCPMinnowSocket<AdaptT>::TCPMinnowSocket( std::pair<FileDescriptor, FileDescriptor> data_socket_pair,
                                          AdaptT&& datagram_interface )
  : LocalStreamSocket( std::move( data_socket_pair.first ) )
  , _datagram_adapter( std::move( datagram_interface ) )
  , _thread_data( std::move( data_socket_pair.second ) )
{
  _thread_data.set_blocking( false );
  set_blocking( false );
}

template<TCPDatagramAdapter AdaptT>
void TCPMinnowSocket<AdaptT>::_initialize_TCP( const TCPConfig& config )
{
  _tcp.emplace( config );

  // Set up the event loop

  // There are three events to handle:
  //
  // 1) Incoming datagram received (needs to be given to TCPPeer::receive method)
  //
  // 2) Outbound bytes received from local application via a write()
  //    call (needs to be read from the local stream socket and
  //    given to TCPPeer)
  //
  // 3) Incoming bytes reassembled by the Reassembler
  //    (needs to be read from the inbound_stream and written
  //    to the local stream socket back to the application)

  // rule 1: read from filtered packet stream and dump into TCPConnection
  _eventloop.add_rule(
    "receive TCP segment from the network",
    _datagram_adapter.fd(),
    Direction::In,
    [&] {
      if ( auto seg = _datagram_adapter.read() ) {
        _tcp->receive( std::move( seg.value() ), [&]( auto x ) { _datagram_adapter.write( x ); } );
      }

      // debugging output:
      if ( _thread_data.eof() and _tcp.value().sender().sequence_numbers_in_flight() == 0 and not _fully_acked ) {
        std::cerr << "DEBUG: minnow outbound stream to " << _datagram_adapter.config().destination.to_string()
                  << " has been fully acknowledged.\n";
        _fully_acked = true;
      }
    },
    [&] { return _tcp->active(); } );

  // rule 2: read from pipe into outbound buffer
  _eventloop.add_rule(
    "push bytes to TCPPeer",
    _thread_data,
    Direction::In,
    [&] {
      std::string data;
      data.resize( _tcp->outbound_writer().available_capacity() );
      _thread_data.read( data );
      _tcp->outbound_writer().push( move( data ) );

      if ( _thread_data.eof() ) {
        _tcp->outbound_writer().close();
        _outbound_shutdown = true;

        // debugging output:
        std::cerr << "DEBUG: minnow outbound stream to " << _datagram_adapter.config().destination.to_string()
                  << " finished (" << _tcp.value().sender().sequence_numbers_in_flight() << " seqno"
                  << ( _tcp.value().sender().sequence_numbers_in_flight() == 1 ? "" : "s" )
                  << " still in flight).\n";
      }

      _tcp->push( [&]( auto x ) { _datagram_adapter.write( x ); } );
    },
    [&] {
      return ( _tcp->active() ) and ( not _outbound_shutdown )
             and ( _tcp->outbound_writer().available_capacity() > 0 );
    },
    [&] {
      _tcp->outbound_writer().close();
      _outbound_shutdown = true;
    },
    [&] {
      std::cerr << "DEBUG: minnow outbound stream had error.\n";
      _tcp->outbound_writer().set_error();
    } );

  // rule 3: read from inbound buffer into pipe
  _eventloop.add_rule(
    "read bytes from inbound stream",
    _thread_data,
    Direction::Out,
    [&] {
      Reader& inbound = _tcp->inbound_reader();
      // Write from the inbound_stream into
      // the pipe, handling the possibility of a partial
      // write (i.e., only pop what was actually written).
      if ( inbound.bytes_buffered() ) {
        const std::string_view buffer = inbound.peek();
        const auto bytes_written = _thread_data.write( buffer );
        inbound.pop( bytes_written );
      }

      if ( inbound.is_finished() or inbound.has_error() ) {
        _thread_data.shutdown( SHUT_WR );
        _inbound_shutdown = true;

        // debugging output:
        std::cerr << "DEBUG: minnow inbound stream from " << _datagram_adapter.config().destination.to_string()
                  << " finished " << ( inbound.has_error() ? "uncleanly.\n" : "cleanly.\n" );
      }
    },
    [&] {
      return _tcp->inbound_reader().bytes_buffered()
             or ( ( _tcp->inbound_reader().is_finished() or _tcp->inbound_reader().has_error() )
                  and not _inbound_shutdown );
    },
    [&] {},
    [&] {
      std::cerr << "DEBUG: minnow inbound stream had error.\n";
      _tcp->inbound_reader().set_error();
    } );
}

//! \brief Call [socketpair](\ref man2::socketpair) and return connected Unix-domain sockets of specified type
//! \param[in] type is the type of AF_UNIX sockets to create (e.g., SOCK_SEQPACKET)
//! \returns a std::pair of connected sockets
template<std::derived_from<Socket> SocketType>
inline std::pair<SocketType, SocketType> socket_pair_helper( int domain, int type, int protocol = 0 )
{
  std::array<int, 2> fds {};
  CheckSystemCall( "socketpair", ::socketpair( domain, type, protocol, fds.data() ) );
  return { SocketType { FileDescriptor { fds[0] } }, SocketType { FileDescriptor { fds[1] } } };
}

//! \param[in] datagram_interface is the underlying interface (e.g. to UDP, IP, or Ethernet)
template<TCPDatagramAdapter AdaptT>
TCPMinnowSocket<AdaptT>::TCPMinnowSocket( AdaptT&& datagram_interface )
  : TCPMinnowSocket( socket_pair_helper<LocalStreamSocket>( AF_UNIX, SOCK_STREAM ),
                     std::move( datagram_interface ) )
{}

template<TCPDatagramAdapter AdaptT>
TCPMinnowSocket<AdaptT>::~TCPMinnowSocket()
{
  try {
    if ( _tcp_thread.joinable() ) {
      std::cerr << "Warning: unclean shutdown of TCPMinnowSocket\n";
      // force the other side to exit
      _abort.store( true );
      _tcp_thread.join();
    }
  } catch ( const std::exception& e ) {
    std::cerr << "Exception destructing TCPMinnowSocket: " << e.what() << std::endl;
  }
}

template<TCPDatagramAdapter AdaptT>
void TCPMinnowSocket<AdaptT>::wait_until_closed()
{
  shutdown( SHUT_RDWR );
  if ( _tcp_thread.joinable() ) {
    std::cerr << "DEBUG: minnow waiting for clean shutdown... ";
    _tcp_thread.join();
    std::cerr << "done.\n";
  }
}

//! \param[in] c_tcp is the TCPConfig for the TCPConnection
//! \param[in] c_ad is the FdAdapterConfig for the FdAdapter
template<TCPDatagramAdapter AdaptT>
void TCPMinnowSocket<AdaptT>::connect( const TCPConfig& c_tcp, const FdAdapterConfig& c_ad )
{
  if ( _tcp ) {
    throw std::runtime_error( "connect() with TCPConnection already initialized" );
  }

  _initialize_TCP( c_tcp );

  _datagram_adapter.config_mut() = c_ad;

  std::cerr << "DEBUG: minnow connecting to " << c_ad.destination.to_string() << "...\n";

  if ( not _tcp.has_value() ) {
    throw std::runtime_error( "TCPPeer not successfully initialized" );
  }

  _tcp->push( [&]( auto x ) { _datagram_adapter.write( x ); } );

  if ( _tcp->sender().sequence_numbers_in_flight() != 1 ) {
    throw std::runtime_error( "After TCPConnection::connect(), expected sequence_numbers_in_flight() == 1" );
  }

  _tcp_loop( [&] { return _tcp->sender().sequence_numbers_in_flight() == 1; } );
  if ( _tcp->inbound_reader().has_error() ) {
    std::cerr << "DEBUG: minnow error on connecting to " << c_ad.destination.to_string() << ".\n";
  } else {
    std::cerr << "DEBUG: minnow successfully connected to " << c_ad.destination.to_string() << ".\n";
  }

  _tcp_thread = std::thread( &TCPMinnowSocket::_tcp_main, this );
}

//! \param[in] c_tcp is the TCPConfig for the TCPConnection
//! \param[in] c_ad is the FdAdapterConfig for the FdAdapter
template<TCPDatagramAdapter AdaptT>
void TCPMinnowSocket<AdaptT>::listen_and_accept( const TCPConfig& c_tcp, const FdAdapterConfig& c_ad )
{
  if ( _tcp ) {
    throw std::runtime_error( "listen_and_accept() with TCPConnection already initialized" );
  }

  _initialize_TCP( c_tcp );

  _datagram_adapter.config_mut() = c_ad;
  _datagram_adapter.set_listening( true );

  std::cerr << "DEBUG: minnow listening for incoming connection...\n";
  _tcp_loop( [&] { return ( not _tcp->has_ackno() ) or ( _tcp->sender().sequence_numbers_in_flight() ); } );
  std::cerr << "DEBUG: minnow new connection from " << _datagram_adapter.config().destination.to_string() << ".\n";

  _tcp_thread = std::thread( &TCPMinnowSocket::_tcp_main, this );
}

template<TCPDatagramAdapter AdaptT>
void TCPMinnowSocket<AdaptT>::_tcp_main()
{
  try {
    if ( not _tcp.has_value() ) {
      throw std::runtime_error( "no TCP" );
    }
    _tcp_loop( [] { return true; } );
    shutdown( SHUT_RDWR );
    if ( not _tcp.value().active() ) {
      std::cerr << "DEBUG: minnow TCP connection finished "
                << ( _tcp->inbound_reader().has_error() ? "uncleanly.\n" : "cleanly.\n" );
    }
    _tcp.reset();
  } catch ( const std::exception& e ) {
    std::cerr << "Exception in TCPConnection runner thread: " << e.what() << "\n";
    throw e;
  }
}
