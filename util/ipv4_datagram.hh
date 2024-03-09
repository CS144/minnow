#pragma once

#include "ipv4_header.hh"
#include "parser.hh"

#include <memory>
#include <string>
#include <vector>

//! \brief [IPv4](\ref rfc::rfc791) Internet datagram
struct IPv4Datagram
{
  IPv4Header header {};
  std::vector<std::string> payload {};

  void parse( Parser& parser )
  {
    header.parse( parser );

    // The Ethernet frame can have padding on the end. We must ignore it by only
    // taking the number of bytes specified in the header.
    //
    // TODO: Efficiency. There's no need to concatenate all the remaining
    // elements of the `vector`, as long as we take only the first `N`. This
    // would save a a few copies.
    payload.emplace_back( header.payload_length(), '\x00' );
    parser.string( payload.back() );
  }

  void serialize( Serializer& serializer ) const
  {
    header.serialize( serializer );
    for ( const auto& x : payload ) {
      serializer.buffer( x );
    }
  }
};

using InternetDatagram = IPv4Datagram;
