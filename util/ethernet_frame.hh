#pragma once

#include "buffer.hh"
#include "ethernet_header.hh"
#include "parser.hh"

#include <vector>

struct EthernetFrame
{
  EthernetHeader header {};
  std::vector<Buffer> payload {};

  void parse( Parser& parser )
  {
    header.parse( parser );
    parser.all_remaining( payload );
  }

  void serialize( Serializer& serializer ) const
  {
    header.serialize( serializer );
    serializer.buffer( payload );
  }
};
