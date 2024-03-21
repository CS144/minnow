#include "tcp_over_ip.hh"

#include "ipv4_datagram.hh"
#include "ipv4_header.hh"
#include "parser.hh"

#include <arpa/inet.h>
#include <stdexcept>
#include <unistd.h>
#include <utility>

using namespace std;

//! \details This function attempts to parse a TCP segment from
//! the IP datagram's payload.
//!
//! If this succeeds, it then checks that the received segment is related to the
//! current connection. When a TCP connection has been established, this means
//! checking that the source and destination ports in the TCP header are correct.
//!
//! If the TCP connection is listening (i.e., TCPOverIPv4OverTunFdAdapter::_listen is `true`)
//! and the TCP segment read from the wire includes a SYN, this function clears the
//! `_listen` flag and records the source and destination addresses and port numbers
//! from the TCP header; it uses this information to filter future reads.
//! \returns a std::optional<TCPSegment> that is empty if the segment was invalid or unrelated
optional<TCPMessage> TCPOverIPv4Adapter::unwrap_tcp_in_ip( const InternetDatagram& ip_dgram )
{
  // is the IPv4 datagram for us?
  // Note: it's valid to bind to address "0" (INADDR_ANY) and reply from actual address contacted
  if ( not listening() and ( ip_dgram.header.dst != config().source.ipv4_numeric() ) ) {
    return {};
  }

  // is the IPv4 datagram from our peer?
  if ( not listening() and ( ip_dgram.header.src != config().destination.ipv4_numeric() ) ) {
    return {};
  }

  // does the IPv4 datagram claim that its payload is a TCP segment?
  if ( ip_dgram.header.proto != IPv4Header::PROTO_TCP ) {
    return {};
  }

  // is the payload a valid TCP segment?
  TCPSegment tcp_seg;
  if ( not parse( tcp_seg, ip_dgram.payload, ip_dgram.header.pseudo_checksum() ) ) {
    return {};
  }

  // is the TCP segment for us?
  if ( tcp_seg.udinfo.dst_port != config().source.port() ) {
    return {};
  }

  // should we target this source addr/port (and use its destination addr as our source) in reply?
  if ( listening() ) {
    if ( tcp_seg.message.sender.SYN and not tcp_seg.message.sender.RST ) {
      config_mutable().source = Address { inet_ntoa( { htobe32( ip_dgram.header.dst ) } ), config().source.port() };
      config_mutable().destination
        = Address { inet_ntoa( { htobe32( ip_dgram.header.src ) } ), tcp_seg.udinfo.src_port };
      set_listening( false );
    } else {
      return {};
    }
  }

  // is the TCP segment from our peer?
  if ( tcp_seg.udinfo.src_port != config().destination.port() ) {
    return {};
  }

  return tcp_seg.message;
}

//! Takes a TCP segment, sets port numbers as necessary, and wraps it in an IPv4 datagram
//! \param[in] seg is the TCP segment to convert
InternetDatagram TCPOverIPv4Adapter::wrap_tcp_in_ip( const TCPMessage& msg )
{
  TCPSegment seg { .message = msg };
  // set the port numbers in the TCP segment
  seg.udinfo.src_port = config().source.port();
  seg.udinfo.dst_port = config().destination.port();

  // create an Internet Datagram and set its addresses and length
  InternetDatagram ip_dgram;
  ip_dgram.header.src = config().source.ipv4_numeric();
  ip_dgram.header.dst = config().destination.ipv4_numeric();
  ip_dgram.header.len = ip_dgram.header.hlen * 4 + 20 /* tcp header len */ + seg.message.sender.payload.size();

  // set payload, calculating TCP checksum using information from IP header
  seg.compute_checksum( ip_dgram.header.pseudo_checksum() );
  ip_dgram.header.compute_checksum();
  ip_dgram.payload = serialize( seg );

  return ip_dgram;
}
