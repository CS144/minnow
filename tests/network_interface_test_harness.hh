#pragma once

#include <compare>
#include <optional>
#include <utility>

#include "arp_message.hh"
#include "common.hh"
#include "network_interface.hh"

class NetworkInterfaceTestHarness : public TestHarness<NetworkInterface>
{
public:
  NetworkInterfaceTestHarness( std::string test_name,
                               const EthernetAddress& ethernet_address,
                               const Address& ip_address )
    : TestHarness( move( test_name ),
                   "eth=" + to_string( ethernet_address ) + ", ip=" + ip_address.ip(),
                   NetworkInterface { ethernet_address, ip_address } )
  {}
};

inline std::string summary( const EthernetFrame& frame );

struct SendDatagram : public Action<NetworkInterface>
{
  InternetDatagram dgram;
  Address next_hop;

  std::string description() const override
  {
    return "request to send datagram (to next hop " + next_hop.ip() + "): " + dgram.header.to_string();
  }

  void execute( NetworkInterface& interface ) const override { interface.send_datagram( dgram, next_hop ); }

  SendDatagram( InternetDatagram d, Address n ) : dgram( std::move( d ) ), next_hop( n ) {}
};

template<class T>
bool equal( const T& t1, const T& t2 )
{
  const std::vector<Buffer> t1s = serialize( t1 );
  const std::vector<Buffer> t2s = serialize( t2 );

  std::string t1concat;
  for ( const auto& x : t1s ) {
    t1concat.append( x );
  }

  std::string t2concat;
  for ( const auto& x : t2s ) {
    t2concat.append( x );
  }

  return t1concat == t2concat;
}

struct ReceiveFrame : public Action<NetworkInterface>
{
  EthernetFrame frame;
  std::optional<InternetDatagram> expected;

  std::string description() const override { return "frame arrives (" + summary( frame ) + ")"; }
  void execute( NetworkInterface& interface ) const override
  {
    const std::optional<InternetDatagram> result = interface.recv_frame( frame );

    if ( ( not result.has_value() ) and ( not expected.has_value() ) ) {
      return;
    }

    if ( result.has_value() and not expected.has_value() ) {
      throw ExpectationViolation(
        "an arriving Ethernet frame was passed up the stack as an Internet datagram, but was not expected to be "
        "(did destination address match our interface?)" );
    }

    if ( expected.has_value() and not result.has_value() ) {
      throw ExpectationViolation(
        "an arriving Ethernet frame was expected to be passed up the stack as an Internet datagram, but wasn't" );
    }

    if ( not equal( result.value(), expected.value() ) ) {
      throw ExpectationViolation(
        std::string( "NetworkInterface::recv_frame() produced a different Internet datagram than was expected: " )
        + "actual={" + result.value().header.to_string() + "}" );
    }
  }

  ReceiveFrame( EthernetFrame f, std::optional<InternetDatagram> e )
    : frame( std::move( f ) ), expected( std::move( e ) )
  {}
};

struct ExpectFrame : public Expectation<NetworkInterface>
{
  EthernetFrame expected;

  std::string description() const override { return "frame transmitted (" + summary( expected ) + ")"; }
  void execute( NetworkInterface& interface ) const override
  {
    auto frame = interface.maybe_send();
    if ( not frame.has_value() ) {
      throw ExpectationViolation( "NetworkInterface was expected to send an Ethernet frame, but did not" );
    }

    if ( not equal( frame.value(), expected ) ) {
      throw ExpectationViolation( "NetworkInterface sent a different Ethernet frame than was expected: actual={"
                                  + summary( frame.value() ) + "}" );
    }
  }

  explicit ExpectFrame( EthernetFrame e ) : expected( std::move( e ) ) {}
};

struct ExpectNoFrame : public Expectation<NetworkInterface>
{
  std::string description() const override { return "no frame transmitted"; }
  void execute( NetworkInterface& interface ) const override
  {
    if ( interface.maybe_send().has_value() ) {
      throw ExpectationViolation( "NetworkInterface sent an Ethernet frame although none was expected" );
    }
  }
};

struct Tick : public Action<NetworkInterface>
{
  size_t _ms;

  std::string description() const override { return to_string( _ms ) + " ms pass"; }
  void execute( NetworkInterface& interface ) const override { interface.tick( _ms ); }

  explicit Tick( const size_t ms ) : _ms( ms ) {}
};

inline std::string summary( const EthernetFrame& frame )
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
