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
  std::vector<Buffer> payload {};

  void parse( Parser& parser )
  {
    header.parse( parser );
    parser.all_remaining( payload );
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
