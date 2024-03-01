#include "router.hh"
#include "arp_message.hh"
#include "network_interface_test_harness.hh"
#include "random.hh"

#include <iostream>
#include <list>
#include <unordered_map>
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

uint32_t ip( const string& str )
{
  return Address { str }.ipv4_numeric();
}

class NetworkSegment : public NetworkInterface::OutputPort
{
  vector<weak_ptr<NetworkInterface>> connections_ {};

public:
  void transmit( const NetworkInterface& sender, const EthernetFrame& frame ) override
  {
    ranges::for_each( connections_, [&]( auto& weak_ref ) {
      const shared_ptr<NetworkInterface> interface( weak_ref );
      if ( &sender != interface.get() ) {
        cerr << "Transferring frame from " << sender.name() << " to " << interface->name() << ": "
             << summary( frame ) << "\n";
        interface->recv_frame( frame );
      }
    } );
  }

  void connect( const shared_ptr<NetworkInterface>& interface ) { connections_.push_back( interface ); }
};

class Host
{
  string _name;
  Address _my_address;
  shared_ptr<NetworkInterface> _interface;
  Address _next_hop;

  std::list<InternetDatagram> _expecting_to_receive {};

  bool expecting( const InternetDatagram& expected ) const
  {
    return ranges::any_of( _expecting_to_receive, [&expected]( const auto& x ) { return equal( x, expected ); } );
  }

  void remove_expectation( const InternetDatagram& expected )
  {
    for ( auto it = _expecting_to_receive.begin(); it != _expecting_to_receive.end(); ++it ) {
      if ( equal( *it, expected ) ) {
        _expecting_to_receive.erase( it );
        return;
      }
    }
  }

public:
  // NOLINTNEXTLINE(*-easily-swappable-*)
  Host( string name, const Address& my_address, const Address& next_hop, shared_ptr<NetworkSegment> network )
    : _name( std::move( name ) )
    , _my_address( my_address )
    , _interface(
        make_shared<NetworkInterface>( _name, move( network ), random_host_ethernet_address(), _my_address ) )
    , _next_hop( next_hop )
  {}

  InternetDatagram send_to( const Address& destination, const uint8_t ttl = 64 )
  {
    InternetDatagram dgram;
    dgram.header.src = _my_address.ipv4_numeric();
    dgram.header.dst = destination.ipv4_numeric();
    dgram.payload.emplace_back( string { "Cardinal " + to_string( random_device()() % 1000 ) } );
    dgram.header.len = static_cast<uint64_t>( dgram.header.hlen ) * 4 + dgram.payload.back().size();
    dgram.header.ttl = ttl;
    dgram.header.compute_checksum();

    cerr << "Host " << _name << " trying to send datagram (with next hop = " << _next_hop.ip()
         << "): " << dgram.header.to_string()
         << +" payload=\"" + Printer::prettify( concat( dgram.payload ) ) + "\"\n";

    _interface->send_datagram( dgram, _next_hop );

    return dgram;
  }

  const Address& address() { return _my_address; }

  shared_ptr<NetworkInterface> interface() { return _interface; }

  void expect( const InternetDatagram& expected ) { _expecting_to_receive.push_back( expected ); }

  const string& name() { return _name; }

  void check()
  {
    while ( not _interface->datagrams_received().empty() ) {
      auto& dgram_received = _interface->datagrams_received().front();
      if ( not expecting( dgram_received ) ) {
        throw runtime_error( "Host " + _name
                             + " received unexpected Internet datagram: " + dgram_received.header.to_string() );
      }
      remove_expectation( dgram_received );
      _interface->datagrams_received().pop();
    }

    if ( not _expecting_to_receive.empty() ) {
      throw runtime_error( "Host " + _name + " did NOT receive an expected Internet datagram: "
                           + _expecting_to_receive.front().header.to_string() );
    }
  }
};

class Network
{
private:
  Router _router {};

  shared_ptr<NetworkSegment> upstream { make_shared<NetworkSegment>() },
    eth0_applesauce { make_shared<NetworkSegment>() }, eth2_cherrypie { make_shared<NetworkSegment>() },
    uun { make_shared<NetworkSegment>() }, hs { make_shared<NetworkSegment>() },
    empty { make_shared<NetworkSegment>() };

  size_t default_id, eth0_id, eth1_id, eth2_id, uun3_id, hs4_id, mit5_id;

  unordered_map<string, Host> _hosts {};

public:
  Network()
    : default_id( _router.add_interface( make_shared<NetworkInterface>( "default",
                                                                        upstream,
                                                                        random_router_ethernet_address(),
                                                                        Address { "171.67.76.46" } ) ) )
    , eth0_id( _router.add_interface( make_shared<NetworkInterface>( "eth0",
                                                                     eth0_applesauce,
                                                                     random_router_ethernet_address(),
                                                                     Address { "10.0.0.1" } ) ) )
    , eth1_id( _router.add_interface( make_shared<NetworkInterface>( "eth1",
                                                                     empty,
                                                                     random_router_ethernet_address(),
                                                                     Address { "172.16.0.1" } ) ) )
    , eth2_id( _router.add_interface( make_shared<NetworkInterface>( "eth2",
                                                                     eth2_cherrypie,
                                                                     random_router_ethernet_address(),
                                                                     Address { "192.168.0.1" } ) ) )
    , uun3_id( _router.add_interface( make_shared<NetworkInterface>( "uun3",
                                                                     uun,
                                                                     random_router_ethernet_address(),
                                                                     Address { "198.178.229.1" } ) ) )
    , hs4_id( _router.add_interface(
        make_shared<NetworkInterface>( "hs4", hs, random_router_ethernet_address(), Address { "143.195.0.2" } ) ) )
    , mit5_id( _router.add_interface( make_shared<NetworkInterface>( "mit5",
                                                                     empty,
                                                                     random_router_ethernet_address(),
                                                                     Address { "128.30.76.255" } ) ) )
  {
    _hosts.insert(
      { "applesauce", { "applesauce", Address { "10.0.0.2" }, Address { "10.0.0.1" }, eth0_applesauce } } );
    _hosts.insert(
      { "default_router", { "default_router", Address { "171.67.76.1" }, Address { "0" }, upstream } } );
    _hosts.insert(
      { "cherrypie", { "cherrypie", Address { "192.168.0.2" }, Address { "192.168.0.1" }, eth2_cherrypie } } );
    _hosts.insert( { "hs_router", { "hs_router", Address { "143.195.0.1" }, Address { "0" }, hs } } );
    _hosts.insert( { "dm42", { "dm42", Address { "198.178.229.42" }, Address { "198.178.229.1" }, uun } } );
    _hosts.insert( { "dm43", { "dm43", Address { "198.178.229.43" }, Address { "198.178.229.1" }, uun } } );

    upstream->connect( _router.interface( default_id ) );
    upstream->connect( host( "default_router" ).interface() );

    eth0_applesauce->connect( _router.interface( eth0_id ) );
    eth0_applesauce->connect( host( "applesauce" ).interface() );

    eth2_cherrypie->connect( _router.interface( eth2_id ) );
    eth2_cherrypie->connect( host( "cherrypie" ).interface() );

    uun->connect( _router.interface( uun3_id ) );
    uun->connect( host( "dm42" ).interface() );
    uun->connect( host( "dm43" ).interface() );

    hs->connect( _router.interface( hs4_id ) );
    hs->connect( host( "hs_router" ).interface() );

    _router.add_route( ip( "0.0.0.0" ), 0, host( "default_router" ).address(), default_id );
    _router.add_route( ip( "10.0.0.0" ), 8, {}, eth0_id );
    _router.add_route( ip( "172.16.0.0" ), 16, {}, eth1_id );
    _router.add_route( ip( "192.168.0.0" ), 24, {}, eth2_id );
    _router.add_route( ip( "198.178.229.0" ), 24, {}, uun3_id );
    _router.add_route( ip( "143.195.0.0" ), 17, host( "hs_router" ).address(), hs4_id );
    _router.add_route( ip( "143.195.128.0" ), 18, host( "hs_router" ).address(), hs4_id );
    _router.add_route( ip( "143.195.192.0" ), 19, host( "hs_router" ).address(), hs4_id );
    _router.add_route( ip( "128.30.76.255" ), 16, Address { "128.30.0.1" }, mit5_id );
  }

  void simulate()
  {
    for ( unsigned int i = 0; i < 256; i++ ) {
      _router.route();
    }

    for ( auto& host : _hosts ) {
      host.second.check();
    }
  }

  Host& host( const string& name )
  {
    auto it = _hosts.find( name );
    if ( it == _hosts.end() ) {
      throw runtime_error( "unknown host: " + name );
    }
    if ( it->second.name() != name ) {
      throw runtime_error( "invalid host: " + name );
    }
    return it->second;
  }
};

void network_simulator()
{
  const string green = "\033[32;1m";
  const string normal = "\033[m";

  cerr << green << "Constructing network." << normal << "\n";

  Network network;

  cout << green << "\n\nTesting traffic between two ordinary hosts (applesauce to cherrypie)..." << normal
       << "\n\n";
  {
    auto dgram_sent = network.host( "applesauce" ).send_to( network.host( "cherrypie" ).address() );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "cherrypie" ).expect( dgram_sent );
    network.simulate();
  }

  cout << green << "\n\nTesting traffic between two ordinary hosts (cherrypie to applesauce)..." << normal
       << "\n\n";
  {
    auto dgram_sent = network.host( "cherrypie" ).send_to( network.host( "applesauce" ).address() );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "applesauce" ).expect( dgram_sent );
    network.simulate();
  }

  cout << green << "\n\nSuccess! Testing applesauce sending to the Internet." << normal << "\n\n";
  {
    auto dgram_sent = network.host( "applesauce" ).send_to( Address { "1.2.3.4" } );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "default_router" ).expect( dgram_sent );
    network.simulate();
  }

  cout << green << "\n\nSuccess! Testing sending to the HS network and Internet." << normal << "\n\n";
  {
    auto dgram_sent = network.host( "applesauce" ).send_to( Address { "143.195.131.17" } );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "hs_router" ).expect( dgram_sent );
    network.simulate();

    dgram_sent = network.host( "cherrypie" ).send_to( Address { "143.195.193.52" } );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "hs_router" ).expect( dgram_sent );
    network.simulate();

    dgram_sent = network.host( "cherrypie" ).send_to( Address { "143.195.223.255" } );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "hs_router" ).expect( dgram_sent );
    network.simulate();

    dgram_sent = network.host( "cherrypie" ).send_to( Address { "143.195.224.0" } );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "default_router" ).expect( dgram_sent );
    network.simulate();
  }

  cout << green << "\n\nSuccess! Testing two hosts on the same network (dm42 to dm43)..." << normal << "\n\n";
  {
    auto dgram_sent = network.host( "dm42" ).send_to( network.host( "dm43" ).address() );
    dgram_sent.header.ttl--;
    dgram_sent.header.compute_checksum();
    network.host( "dm43" ).expect( dgram_sent );
    network.simulate();
  }

  cout << green << "\n\nSuccess! Testing TTL expiration..." << normal << "\n\n";
  {
    auto dgram_sent = network.host( "applesauce" ).send_to( Address { "1.2.3.4" }, 1 );
    network.simulate();

    dgram_sent = network.host( "applesauce" ).send_to( Address { "1.2.3.4" }, 0 );
    network.simulate();
  }

  cout << "\n\n\033[32;1mCongratulations! All datagrams were routed successfully.\033[m\n";
}

int main()
{
  try {
    network_simulator();
  } catch ( const exception& e ) {
    cerr << "\n\n\n";
    cerr << "\033[31;1mError: " << e.what() << "\033[m\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
