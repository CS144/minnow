#pragma once

#include "byte_stream.hh"
#include "eventloop.hh"
#include "file_descriptor.hh"
#include "socket.hh"
#include "tcp_config.hh"
#include "tcp_peer.hh"
#include "tuntap_adapter.hh"

#include <atomic>
#include <cstdint>
#include <optional>
#include <thread>
#include <vector>

//! Multithreaded wrapper around TCPPeer that approximates the Unix sockets API
template<TCPDatagramAdapter AdaptT>
class TCPMinnowSocket : public LocalStreamSocket
{
public:
  //! Construct from the interface that the TCPPeer thread will use to read and write datagrams
  explicit TCPMinnowSocket( AdaptT&& datagram_interface );

  //! Close socket, and wait for TCPPeer to finish
  //! \note Calling this function is only advisable if the socket has reached EOF,
  //! or else may wait foreever for remote peer to close the TCP connection.
  void wait_until_closed();

  //! Connect using the specified configurations; blocks until connect succeeds or fails
  void connect( const TCPConfig& c_tcp, const FdAdapterConfig& c_ad );

  //! Listen and accept using the specified configurations; blocks until accept succeeds or fails
  void listen_and_accept( const TCPConfig& c_tcp, const FdAdapterConfig& c_ad );

  //! When a connected socket is destructed, it will send a RST
  ~TCPMinnowSocket();

  //! \name
  //! This object cannot be safely moved or copied, since it is in use by two threads simultaneously

  //!@{
  TCPMinnowSocket( const TCPMinnowSocket& ) = delete;
  TCPMinnowSocket( TCPMinnowSocket&& ) = delete;
  TCPMinnowSocket& operator=( const TCPMinnowSocket& ) = delete;
  TCPMinnowSocket& operator=( TCPMinnowSocket&& ) = delete;
  //!@}

  //! \name
  //! Some methods of the parent Socket wouldn't work as expected on the TCP socket, so delete them

  //!@{
  void bind( const Address& address ) = delete;
  Address local_address() const = delete;
  void set_reuseaddr() = delete;
  //!@}

  // Return peer address from underlying datagram adapter
  const Address& peer_address() const { return _datagram_adapter.config().destination; }

protected:
  //! Adapter to underlying datagram socket (e.g., UDP or IP)
  AdaptT _datagram_adapter;

private:
  //! Stream socket for reads and writes between owner and TCP thread
  LocalStreamSocket _thread_data;

  //! Set up the TCPPeer and the event loop
  void _initialize_TCP( const TCPConfig& config );

  //! TCP state machine
  std::optional<TCPPeer> _tcp {};

  //! eventloop that handles all the events (new inbound datagram, new outbound bytes, new inbound bytes)
  EventLoop _eventloop {};

  //! Process events while specified condition is true
  void _tcp_loop( const std::function<bool()>& condition );

  //! Main loop of TCPPeer thread
  void _tcp_main();

  //! Handle to the TCPPeer thread; owner thread calls join() in the destructor
  std::thread _tcp_thread {};

  //! Construct LocalStreamSocket fds from socket pair, initialize eventloop
  TCPMinnowSocket( std::pair<FileDescriptor, FileDescriptor> data_socket_pair, AdaptT&& datagram_interface );

  std::atomic_bool _abort { false }; //!< Flag used by the owner to force the TCPPeer thread to shut down

  bool _inbound_shutdown { false }; //!< Has TCPMinnowSocket shut down the incoming data to the owner?

  bool _outbound_shutdown { false }; //!< Has the owner shut down the outbound data to the TCP connection?

  bool _fully_acked { false }; //!< Has the outbound data been fully acknowledged by the peer?
};

using TCPOverIPv4MinnowSocket = TCPMinnowSocket<TCPOverIPv4OverTunFdAdapter>;
using LossyTCPOverIPv4MinnowSocket = TCPMinnowSocket<LossyFdAdapter<TCPOverIPv4OverTunFdAdapter>>;

//! \class TCPMinnowSocket
//! This class involves the simultaneous operation of two threads.
//!
//! One, the "owner" or foreground thread, interacts with this class in much the
//! same way as one would interact with a TCPSocket: it connects or listens, writes to
//! and reads from a reliable data stream, etc. Only the owner thread calls public
//! methods of this class.
//!
//! The other, the "TCPPeer" thread, takes care of the back-end tasks that the kernel would
//! perform for a TCPSocket: reading and parsing datagrams from the wire, filtering out
//! segments unrelated to the connection, etc.
//!
//! There are a few notable differences between the TCPMinnowSocket and TCPSocket interfaces:
//!
//! - a TCPMinnowSocket can only accept a single connection
//! - listen_and_accept() is a blocking function call that acts as both [listen(2)](\ref man2::listen)
//!   and [accept(2)](\ref man2::accept)
//! - if TCPMinnowSocket is destructed while a TCP connection is open, the connection is
//!   immediately terminated with a RST (call `wait_until_closed` to avoid this)

//! Helper class that makes a TCPOverIPv4MinnowSocket behave more like a (kernel) TCPSocket
class CS144TCPSocket : public TCPOverIPv4MinnowSocket
{
public:
  CS144TCPSocket() : TCPOverIPv4MinnowSocket( TCPOverIPv4OverTunFdAdapter { TunFD { "tun144" } } ) {}
  void connect( const Address& address )
  {
    TCPConfig tcp_config;
    tcp_config.rt_timeout = 100;

    FdAdapterConfig multiplexer_config;
    multiplexer_config.source = { "169.254.144.9", std::to_string( uint16_t( std::random_device()() ) ) };
    multiplexer_config.destination = address;

    TCPOverIPv4MinnowSocket::connect( tcp_config, multiplexer_config );
  }
};
