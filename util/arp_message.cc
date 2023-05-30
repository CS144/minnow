#include "arp_message.hh"

#include <arpa/inet.h>
#include <iomanip>
#include <sstream>

using namespace std;

bool ARPMessage::supported() const
{
  return hardware_type == TYPE_ETHERNET and protocol_type == EthernetHeader::TYPE_IPv4
         and hardware_address_size == sizeof( EthernetHeader::src )
         and protocol_address_size == sizeof( IPv4Header::src )
         and ( ( opcode == OPCODE_REQUEST ) or ( opcode == OPCODE_REPLY ) );
}

string ARPMessage::to_string() const
{
  stringstream ss {};
  string opcode_str = "(unknown type)";
  if ( opcode == OPCODE_REQUEST ) {
    opcode_str = "REQUEST";
  }
  if ( opcode == OPCODE_REPLY ) {
    opcode_str = "REPLY";
  }
  ss << "opcode=" << opcode_str << ", sender=" << ::to_string( sender_ethernet_address ) << "/"
     << inet_ntoa( { htobe32( sender_ip_address ) } ) << ", target=" << ::to_string( target_ethernet_address )
     << "/" << inet_ntoa( { htobe32( target_ip_address ) } );
  return ss.str();
}

void ARPMessage::parse( Parser& parser )
{
  parser.integer( hardware_type );
  parser.integer( protocol_type );
  parser.integer( hardware_address_size );
  parser.integer( protocol_address_size );
  parser.integer( opcode );

  if ( not supported() ) {
    parser.set_error();
    return;
  }

  // read sender addresses (Ethernet and IP)
  for ( auto& b : sender_ethernet_address ) {
    parser.integer( b );
  }
  parser.integer( sender_ip_address );

  // read target addresses (Ethernet and IP)
  for ( auto& b : target_ethernet_address ) {
    parser.integer( b );
  }
  parser.integer( target_ip_address );
}

void ARPMessage::serialize( Serializer& serializer ) const
{
  if ( not supported() ) {
    throw runtime_error( "ARPMessage: unsupported field combination (must be Ethernet/IP, and request or reply)" );
  }

  serializer.integer( hardware_type );
  serializer.integer( protocol_type );
  serializer.integer( hardware_address_size );
  serializer.integer( protocol_address_size );
  serializer.integer( opcode );

  // read sender addresses (Ethernet and IP)
  for ( const auto& b : sender_ethernet_address ) {
    serializer.integer( b );
  }
  serializer.integer( sender_ip_address );

  // read target addresses (Ethernet and IP)
  for ( const auto& b : target_ethernet_address ) {
    serializer.integer( b );
  }
  serializer.integer( target_ip_address );
}
