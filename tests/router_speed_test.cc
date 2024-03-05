#include "router.hh"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>

//! \brief An output port that drops all packets
//!
//! All NetworkInterfaces must be instantiated with an OutputPort that defines
//! how to transmit on that interface. We aren't measuring transmission though.
//! Therefore, we'll instantiate network interfaces that just ignore packets.
class NoopOutputPort : public NetworkInterface::OutputPort
{
public:
  void transmit( const NetworkInterface& sender, const EthernetFrame& frame ) override
  {
    (void)sender;
    (void)frame;
  }
};

int main( void )
{

  // Construct the no-op output port used by the NICs
  auto noop_port = std::make_shared<NoopOutputPort>();
  // Create the router and add all the interfaces and routes to it
  const size_t NUM_NICS = 7ul;
  Router router {};
  router.add_interface( std::make_shared<NetworkInterface>(
    "default", noop_port, EthernetAddress { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 }, Address { "171.67.76.46" } ) );
  router.add_interface( std::make_shared<NetworkInterface>(
    "eth0", noop_port, EthernetAddress { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 }, Address { "10.0.0.1" } ) );
  router.add_interface( std::make_shared<NetworkInterface>(
    "eth1", noop_port, EthernetAddress { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 }, Address { "172.16.0.1" } ) );
  router.add_interface( std::make_shared<NetworkInterface>(
    "eth2", noop_port, EthernetAddress { 0x02, 0x00, 0x00, 0x00, 0x00, 0x03 }, Address { "192.168.0.1" } ) );
  router.add_interface( std::make_shared<NetworkInterface>(
    "uun3", noop_port, EthernetAddress { 0x02, 0x00, 0x00, 0x00, 0x00, 0x04 }, Address { "198.178.229.1" } ) );
  router.add_interface( std::make_shared<NetworkInterface>(
    "hs4", noop_port, EthernetAddress { 0x02, 0x00, 0x00, 0x00, 0x00, 0x05 }, Address { "143.195.0.2" } ) );
  router.add_interface( std::make_shared<NetworkInterface>(
    "mit5", noop_port, EthernetAddress { 0x02, 0x00, 0x00, 0x00, 0x00, 0x06 }, Address { "128.30.76.255" } ) );
  router.add_route( Address { "0.0.0.0" }.ipv4_numeric(), 0, Address { "171.67.76.1" }, 0 );
  router.add_route( Address { "10.0.0.0" }.ipv4_numeric(), 8, {}, 1 );
  router.add_route( Address { "172.16.0.0" }.ipv4_numeric(), 16, {}, 2 );
  router.add_route( Address { "192.168.0.0" }.ipv4_numeric(), 24, {}, 3 );
  router.add_route( Address { "198.178.229.0" }.ipv4_numeric(), 24, {}, 4 );
  router.add_route( Address { "143.195.0.0" }.ipv4_numeric(), 17, Address { "143.195.0.1" }, 5 );
  router.add_route( Address { "143.195.128.0" }.ipv4_numeric(), 18, Address { "143.195.0.1" }, 5 );
  router.add_route( Address { "143.195.192.0" }.ipv4_numeric(), 19, Address { "143.195.0.1" }, 5 );
  router.add_route( Address { "128.30.76.255" }.ipv4_numeric(), 16, Address { "128.30.0.1" }, 6 );

  // These are the IP addresses we can route to. We choose randomly from among
  // these.
  std::vector<uint32_t> dst_ips = {
    Address { "10.0.0.2" }.ipv4_numeric(),
    Address { "10.255.255.254" }.ipv4_numeric(),
    Address { "172.16.0.2" }.ipv4_numeric(),
    Address { "172.16.255.254" }.ipv4_numeric(),
    Address { "192.168.0.2" }.ipv4_numeric(),
    Address { "192.168.0.254" }.ipv4_numeric(),
    Address { "192.178.255.2" }.ipv4_numeric(),
    Address { "192.178.255.254" }.ipv4_numeric(),
    Address { "143.195.0.2" }.ipv4_numeric(),
    Address { "143.195.128.2" }.ipv4_numeric(),
    Address { "143.195.192.2" }.ipv4_numeric(),
    Address { "128.30.0.2" }.ipv4_numeric(),
    Address { "128.30.255.254" }.ipv4_numeric(),
  };

  // Add packets to each of the input interfaces. For each packet, we pick a
  // random output address from the list above.
  const size_t PACKETS_PER_NIC = 1000000ul;
  std::default_random_engine rng( 0x144 );
  std::uniform_int_distribution<size_t> dst_ip_sel( 0ul, dst_ips.size() - 1 );
  for ( size_t nic_idx = 0ul; nic_idx < NUM_NICS; nic_idx++ ) {
    for ( size_t i = 0ul; i < PACKETS_PER_NIC; i++ ) {

      // Select the destination IP address
      uint32_t dst_ip = dst_ips.at( dst_ip_sel( rng ) );

      // Compute the datagram. We shouldn't care about the source IP, so that
      // should be all zeros. Logic taken from send_to in the test.
      InternetDatagram dgram;
      dgram.header.src = 0u;
      dgram.header.dst = dst_ip;
      dgram.payload.emplace_back( std::string { "CS144 Rocks!" } );
      dgram.header.len = static_cast<uint64_t>( dgram.header.hlen ) * 4 + dgram.payload.back().size();
      dgram.header.ttl = 64u;
      dgram.header.compute_checksum();

      // Add the datagram to the NIC's queue
      router.interface( nic_idx )->datagrams_received().push( dgram );
    }
  }

  // Get the debug output since we can't print otherwise
  std::fstream output;
  output.open( "/dev/tty" );

  // Do the test and compute how many packets were routed per second
  output << "        Done with setup, starting ..." << std::endl;
  const auto start_time = std::chrono::steady_clock::now();
  router.route();
  const auto stop_time = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::duration<double>>( stop_time - start_time );
  const double packets_per_second = static_cast<double>( NUM_NICS * PACKETS_PER_NIC ) / duration.count();

  // Assert that all of the datagrams received queues are empty
  for ( size_t nic_idx = 0ul; nic_idx < NUM_NICS; nic_idx++ ) {
    if ( !router.interface( nic_idx )->datagrams_received().empty() ) {
      std::cout << "Didn't route all packets" << std::endl;
      return EXIT_FAILURE;
    }
  }

  output << "        Got Packets/Sec = " << std::fixed << std::setprecision( 2 ) << packets_per_second << std::endl;
  return EXIT_SUCCESS;
}
