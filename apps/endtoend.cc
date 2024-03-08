#include "address.hh"
#include "arp_message.hh"
#include "bidirectional_stream_copy.hh"
#include "exception.hh"
#include "router.hh"
#include "tcp_minnow_socket_impl.hh"
#include "tcp_over_ip.hh"

#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

using namespace std;

EthernetAddress random_host_ethernet_address()
{
  EthernetAddress addr;
  for ( auto& byte : addr ) {
    byte = random_device()(); // use a random local Ethernet address
  }
  addr.at( 0 ) |= 0x02; // "10" in last two binary digits marks a private Ethernet address
  addr.at( 0 ) &= 0xfe;

  return addr;
}

EthernetAddress random_router_ethernet_address()
{
  EthernetAddress addr;
  for ( auto& byte : addr ) {
    byte = random_device()(); // use a random local Ethernet address
  }
  addr.at( 0 ) = 0x02; // "10" in last two binary digits marks a private Ethernet address
  addr.at( 1 ) = 0;
  addr.at( 2 ) = 0;

  return addr;
}

string summary( const EthernetFrame& frame )
{
  std::string out = frame.header.to_string() + ", payload: ";
  switch ( frame.header.type ) {
    case EthernetHeader::TYPE_IPv4: {
      InternetDatagram dgram;
      if ( parse( dgram, frame.payload ) ) {
        out.append( "IPv4: " + dgram.header.to_string() );
      } else {
        out.append( "bad IPv4 datagram" );
      }
    } break;
    case EthernetHeader::TYPE_ARP: {
      ARPMessage arp;
      if ( parse( arp, frame.payload ) ) {
        out.append( "ARP: " + arp.to_string() );
      } else {
        out.append( "bad ARP message" );
      }
    } break;
    default:
      out.append( "unknown frame type" );
      break;
  }
  return out;
}

optional<EthernetFrame> maybe_receive_frame( FileDescriptor& fd )
{
  vector<string> strs( 3 );
  strs.at( 0 ).resize( EthernetHeader::LENGTH );
  strs.at( 1 ).resize( IPv4Header::LENGTH );
  fd.read( strs );

  EthernetFrame frame;
  vector<string> buffers;
  ranges::transform( strs, back_inserter( buffers ), identity() );
  if ( not parse( frame, buffers ) ) {
    return {};
  }

  return frame;
}

inline std::pair<FileDescriptor, FileDescriptor> make_socket_pair()
{
  std::array<int, 2> fds {};
  CheckSystemCall( "socketpair", ::socketpair( AF_UNIX, SOCK_DGRAM, 0, fds.data() ) );
  return { FileDescriptor { fds[0] }, FileDescriptor { fds[1] } };
}

class NetworkInterfaceAdapter : public TCPOverIPv4Adapter
{
private:
  struct Sender : public NetworkInterface::OutputPort
  {
    pair<FileDescriptor, FileDescriptor> sockets { make_socket_pair() };

    void transmit( const NetworkInterface& n [[maybe_unused]], const EthernetFrame& x ) override
    {
      sockets.first.write( serialize( x ) );
    }
  };

  shared_ptr<Sender> sender_ = make_shared<Sender>();
  NetworkInterface _interface;
  Address _next_hop;

public:
  NetworkInterfaceAdapter( const Address& ip_address, const Address& next_hop ) // NOLINT(*-swappable-*)
    : _interface( "network interface adapter", sender_, random_host_ethernet_address(), ip_address )
    , _next_hop( next_hop )
  {}

  optional<TCPMessage> read()
  {
    auto frame_opt = maybe_receive_frame( sender_->sockets.first );
    if ( not frame_opt ) {
      return {};
    }
    EthernetFrame frame = move( frame_opt.value() );

    // Give the frame to the NetworkInterface. Get back an Internet datagram if frame was carrying one.
    _interface.recv_frame( frame );

    // Try to interpret IPv4 datagram as TCP
    if ( _interface.datagrams_received().empty() ) {
      return {};
    }

    InternetDatagram dgram = move( _interface.datagrams_received().front() );
    _interface.datagrams_received().pop();
    return unwrap_tcp_in_ip( dgram );
  }
  void write( const TCPMessage& msg ) { _interface.send_datagram( wrap_tcp_in_ip( msg ), _next_hop ); }
  void tick( const size_t ms_since_last_tick ) { _interface.tick( ms_since_last_tick ); }
  NetworkInterface& interface() { return _interface; }

  FileDescriptor& fd() { return sender_->sockets.first; }
  FileDescriptor& frame_fd() { return sender_->sockets.second; }
};

class TCPSocketEndToEnd : public TCPMinnowSocket<NetworkInterfaceAdapter>
{
  Address _local_address;

public:
  TCPSocketEndToEnd( const Address& ip_address, const Address& next_hop )
    : TCPMinnowSocket<NetworkInterfaceAdapter>( NetworkInterfaceAdapter( ip_address, next_hop ) )
    , _local_address( ip_address )
  {}

  void connect( const Address& address )
  {
    FdAdapterConfig multiplexer_config;

    _local_address = Address { _local_address.ip(), uint16_t( random_device()() ) };
    cerr << "DEBUG: Connecting from " << _local_address.to_string() << "...\n";
    multiplexer_config.source = _local_address;
    multiplexer_config.destination = address;

    TCPMinnowSocket<NetworkInterfaceAdapter>::connect( {}, multiplexer_config );
  }

  void bind( const Address& address )
  {
    if ( address.ip() != _local_address.ip() ) {
      throw runtime_error( "Cannot bind to " + address.to_string() );
    }
    _local_address = Address { _local_address.ip(), address.port() };
  }

  void listen_and_accept()
  {
    FdAdapterConfig multiplexer_config;
    multiplexer_config.source = _local_address;
    TCPMinnowSocket<NetworkInterfaceAdapter>::listen_and_accept( {}, multiplexer_config );
  }

  NetworkInterfaceAdapter& adapter() { return _datagram_adapter; }
};

// NOLINTBEGIN(*-cognitive-complexity)
void program_body( bool is_client, const string& bounce_host, const string& bounce_port, const bool debug )
{
  class FramesOut : public NetworkInterface::OutputPort
  {
  public:
    std::queue<EthernetFrame> frames {};
    void transmit( const NetworkInterface& n [[maybe_unused]], const EthernetFrame& x ) override
    {
      frames.push( x );
    }
  };

  auto router_to_host = make_shared<FramesOut>();
  auto router_to_internet = make_shared<FramesOut>();

  UDPSocket internet_socket;
  Address bounce_address { bounce_host, bounce_port };

  /* let bouncer know where we are */
  internet_socket.sendto( bounce_address, "" );
  internet_socket.sendto( bounce_address, "" );
  internet_socket.sendto( bounce_address, "" );
  internet_socket.connect( bounce_address );

  /* set up the router */
  Router router;

  unsigned int host_side {};
  unsigned int internet_side {};

  if ( is_client ) {
    host_side = router.add_interface( make_shared<NetworkInterface>(
      "host_side", router_to_host, random_router_ethernet_address(), Address { "192.168.0.1" } ) );
    internet_side = router.add_interface( make_shared<NetworkInterface>(
      "internet side", router_to_internet, random_router_ethernet_address(), Address { "10.0.0.192" } ) );
    router.add_route( Address { "192.168.0.0" }.ipv4_numeric(), 16, {}, host_side );
    router.add_route( Address { "10.0.0.0" }.ipv4_numeric(), 8, {}, internet_side );
    router.add_route( Address { "172.16.0.0" }.ipv4_numeric(), 12, Address { "10.0.0.172" }, internet_side );
  } else {
    host_side = router.add_interface( make_shared<NetworkInterface>(
      "host_side", router_to_host, random_router_ethernet_address(), Address { "172.16.0.1" } ) );
    internet_side = router.add_interface( make_shared<NetworkInterface>(
      "internet side", router_to_internet, random_router_ethernet_address(), Address { "10.0.0.172" } ) );
    router.add_route( Address { "172.16.0.0" }.ipv4_numeric(), 12, {}, host_side );
    router.add_route( Address { "10.0.0.0" }.ipv4_numeric(), 8, {}, internet_side );
    router.add_route( Address { "192.168.0.0" }.ipv4_numeric(), 16, Address { "10.0.0.192" }, internet_side );
  }

  /* set up the client */
  TCPSocketEndToEnd sock = is_client ? TCPSocketEndToEnd { Address { "192.168.0.50" }, Address { "192.168.0.1" } }
                                     : TCPSocketEndToEnd { Address { "172.16.0.100" }, Address { "172.16.0.1" } };

  atomic<bool> exit_flag {};

  /* set up the network */
  thread network_thread( [&]() {
    try {
      EventLoop event_loop;
      // Frames from host to router
      event_loop.add_rule( "frames from host to router", sock.adapter().frame_fd(), Direction::In, [&] {
        auto frame_opt = maybe_receive_frame( sock.adapter().frame_fd() );
        if ( not frame_opt ) {
          return;
        }
        EthernetFrame frame = move( frame_opt.value() );
        if ( debug ) {
          cerr << "     Host->router:     " << summary( frame ) << "\n";
        }
        router.interface( host_side )->recv_frame( frame );
        router.route();
      } );

      // Frames from router to host
      event_loop.add_rule(
        "frames from router to host",
        sock.adapter().frame_fd(),
        Direction::Out,
        [&] {
          auto& f = router_to_host;
          if ( debug ) {
            cerr << "     Router->host:     " << summary( f->frames.front() ) << "\n";
          }
          sock.adapter().frame_fd().write( serialize( f->frames.front() ) );
          f->frames.pop();
        },
        [&] { return not router_to_host->frames.empty(); } );

      // Frames from router to Internet
      event_loop.add_rule(
        "frames from router to Internet",
        internet_socket,
        Direction::Out,
        [&] {
          auto& f = router_to_internet;
          if ( debug ) {
            cerr << "     Router->Internet: " << summary( f->frames.front() ) << "\n";
          }
          internet_socket.write( serialize( f->frames.front() ) );
          f->frames.pop();
        },
        [&] { return not router_to_internet->frames.empty(); } );

      // Frames from Internet to router
      event_loop.add_rule( "frames from Internet to router", internet_socket, Direction::In, [&] {
        auto frame_opt = maybe_receive_frame( internet_socket );
        if ( not frame_opt ) {
          return;
        }
        EthernetFrame frame = move( frame_opt.value() );
        if ( debug ) {
          cerr << "     Internet->router: " << summary( frame ) << "\n";
        }
        router.interface( internet_side )->recv_frame( frame );
        router.route();
      } );

      while ( true ) {
        if ( EventLoop::Result::Exit == event_loop.wait_next_event( 10 ) ) {
          cerr << "Exiting...\n";
          return;
        }
        router.interface( host_side )->tick( 10 );
        router.interface( internet_side )->tick( 10 );

        if ( exit_flag ) {
          return;
        }
      }
    } catch ( const exception& e ) {
      cerr << "Thread ending from exception: " << e.what() << "\n";
    }
  } );

  try {
    if ( is_client ) {
      sock.connect( Address { "172.16.0.100", 1234 } );
    } else {
      sock.bind( Address { "172.16.0.100", 1234 } );
      sock.listen_and_accept();
    }

    bidirectional_stream_copy( sock, "172.16.0.100" );
    sock.wait_until_closed();
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << "\n";
  }

  cerr << "Exiting... ";
  exit_flag = true;
  network_thread.join();
  cerr << "done.\n";
}
// NOLINTEND(*-cognitive-complexity)

void print_usage( const string& argv0 )
{
  cerr << "Usage: " << argv0 << " client HOST PORT [debug]\n";
  cerr << "or     " << argv0 << " server HOST PORT [debug]\n";
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    if ( argc != 4 and argc != 5 ) {
      print_usage( args[0] );
      return EXIT_FAILURE;
    }

    if ( args[1] != "client"s and args[1] != "server"s ) {
      print_usage( args[0] );
      return EXIT_FAILURE;
    }

    program_body( args[1] == "client"s, args[2], args[3], argc == 5 );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
