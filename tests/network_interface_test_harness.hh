#pragma once

#include <compare>
#include <optional>
#include <utility>

#include "arp_message.hh"
#include "common.hh"
#include "network_interface.hh"

class FramesOut : public NetworkInterface::OutputPort
{
public:
  std::queue<EthernetFrame> frames {};
  void transmit( const NetworkInterface& n [[maybe_unused]], const EthernetFrame& x ) override { frames.push( x ); }
};

using Output = std::shared_ptr<FramesOut>;
using InterfaceAndOutput = std::pair<NetworkInterface, Output>;

class NetworkInterfaceTestHarness : public TestHarness<InterfaceAndOutput>
{
public:
  NetworkInterfaceTestHarness( std::string test_name,
                               const EthernetAddress& ethernet_address,
                               const Address& ip_address )
    : TestHarness( move( test_name ), "eth=" + to_string( ethernet_address ) + ", ip=" + ip_address.ip(), [&] {
      const Output output { std::make_shared<FramesOut>() };
      const NetworkInterface iface { "test", output, ethernet_address, ip_address };
      return InterfaceAndOutput { iface, output };
    }() )
  {}
};

inline std::string summary( const EthernetFrame& frame );

struct SendDatagram : public Action<InterfaceAndOutput>
{
  InternetDatagram dgram;
  Address next_hop;

  std::string description() const override
  {
    return "request to send datagram (to next hop " + next_hop.ip() + "): " + dgram.header.to_string();
  }

  void execute( InterfaceAndOutput& interface ) const override { interface.first.send_datagram( dgram, next_hop ); }

  SendDatagram( InternetDatagram d, Address n ) : dgram( std::move( d ) ), next_hop( n ) {}
};

inline std::string concat( const std::vector<std::string>& buffers )
{
  return std::accumulate( buffers.begin(), buffers.end(), std::string {} );
}

template<class T>
bool equal( const T& t1, const T& t2 )
{
  const std::vector<std::string> t1s = serialize( t1 );
  const std::vector<std::string> t2s = serialize( t2 );

  return concat( t1s ) == concat( t2s );
}

struct ReceiveFrame : public Action<InterfaceAndOutput>
{
  EthernetFrame frame;
  std::optional<InternetDatagram> expected;

  std::string description() const override { return "frame arrives (" + summary( frame ) + ")"; }
  void execute( InterfaceAndOutput& interface ) const override
  {
    interface.first.recv_frame( frame );

    auto& inbound = interface.first.datagrams_received();

    if ( not expected.has_value() ) {
      if ( inbound.empty() ) {
        return;
      }
      throw ExpectationViolation(
        "an arriving Ethernet frame was passed up the stack as an Internet datagram, but was not expected to be "
        "(did destination address match our interface?)" );
    }

    if ( inbound.empty() ) {
      throw ExpectationViolation(
        "an arriving Ethernet frame was expected to be passed up the stack as an Internet datagram, but wasn't" );
    }

    if ( not equal( inbound.front(), expected.value() ) ) {
      throw ExpectationViolation(
        std::string( "NetworkInterface::recv_frame() produced a different Internet datagram than was expected: " )
        + "actual={" + inbound.front().header.to_string() + "}" );
    }

    inbound.pop();
  }

  ReceiveFrame( EthernetFrame f, std::optional<InternetDatagram> e )
    : frame( std::move( f ) ), expected( std::move( e ) )
  {}
};

struct ExpectFrame : public Expectation<InterfaceAndOutput>
{
  EthernetFrame expected;

  std::string description() const override { return "frame transmitted (" + summary( expected ) + ")"; }
  void execute( InterfaceAndOutput& interface ) const override
  {
    if ( interface.second->frames.empty() ) {
      throw ExpectationViolation( "NetworkInterface was expected to send an Ethernet frame, but did not" );
    }

    const EthernetFrame frame = std::move( interface.second->frames.front() );
    interface.second->frames.pop();

    if ( not equal( frame, expected ) ) {
      throw ExpectationViolation( "NetworkInterface sent a different Ethernet frame than was expected: actual={"
                                  + summary( frame ) + "}" );
    }
  }

  explicit ExpectFrame( EthernetFrame e ) : expected( std::move( e ) ) {}
};

struct ExpectNoFrame : public Expectation<InterfaceAndOutput>
{
  std::string description() const override { return "no frame transmitted"; }
  void execute( InterfaceAndOutput& interface ) const override
  {
    if ( not interface.second->frames.empty() ) {
      throw ExpectationViolation( "NetworkInterface sent an Ethernet frame although none was expected" );
    }
  }
};

struct Tick : public Action<InterfaceAndOutput>
{
  size_t _ms;

  std::string description() const override { return to_string( _ms ) + " ms pass"; }
  void execute( InterfaceAndOutput& interface ) const override { interface.first.tick( _ms ); }

  explicit Tick( const size_t ms ) : _ms( ms ) {}
};

inline std::string summary( const EthernetFrame& frame )
{
  std::string out = frame.header.to_string() + " payload: ";
  switch ( frame.header.type ) {
    case EthernetHeader::TYPE_IPv4: {
      InternetDatagram dgram;
      if ( parse( dgram, frame.payload ) ) {
        out.append( dgram.header.to_string() + " payload=\"" + Printer::prettify( concat( dgram.payload ) )
                    + "\"" );
      } else {
        out.append( "bad IPv4 datagram" );
      }
    } break;
    case EthernetHeader::TYPE_ARP: {
      ARPMessage arp;
      if ( parse( arp, frame.payload ) ) {
        out.append( arp.to_string() );
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
