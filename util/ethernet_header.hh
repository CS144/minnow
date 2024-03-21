#pragma once

#include "parser.hh"

#include <array>
#include <cstdint>
#include <string>

// Helper type for an Ethernet address (an array of six bytes)
using EthernetAddress = std::array<uint8_t, 6>;

// Ethernet broadcast address (ff:ff:ff:ff:ff:ff)
constexpr EthernetAddress ETHERNET_BROADCAST = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// Printable representation of an EthernetAddress
std::string to_string( EthernetAddress address );

// Ethernet frame header
struct EthernetHeader
{
  static constexpr size_t LENGTH = 14;         //!< Ethernet header length in bytes
  static constexpr uint16_t TYPE_IPv4 = 0x800; //!< Type number for [IPv4](\ref rfc::rfc791)
  static constexpr uint16_t TYPE_ARP = 0x806;  //!< Type number for [ARP](\ref rfc::rfc826)

  EthernetAddress dst;
  EthernetAddress src;
  uint16_t type;

  // Return a string containing a header in human-readable format
  std::string to_string() const;

  void parse( Parser& parser );
  void serialize( Serializer& serializer ) const;
};
