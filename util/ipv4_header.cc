#include "ipv4_header.hh"
#include "checksum.hh"

#include <arpa/inet.h>
#include <array>
#include <cstddef>
#include <sstream>

using namespace std;

// Parse from string.
void IPv4Header::parse( Parser& parser )
{
  uint8_t first_byte {};
  parser.integer( first_byte );
  ver = first_byte >> 4;    // version
  hlen = first_byte & 0x0f; // header length
  parser.integer( tos );    // type of service
  parser.integer( len );
  parser.integer( id );

  uint16_t fo_val {};
  parser.integer( fo_val );
  df = static_cast<bool>( fo_val & 0x4000 ); // don't fragment
  mf = static_cast<bool>( fo_val & 0x2000 ); // more fragments
  offset = fo_val & 0x1fff;                  // offset

  parser.integer( ttl );
  parser.integer( proto );
  parser.integer( cksum );
  parser.integer( src );
  parser.integer( dst );

  if ( ver != 4 ) {
    parser.set_error();
  }

  if ( hlen < 5 ) {
    parser.set_error();
  }

  if ( parser.has_error() ) {
    return;
  }

  parser.remove_prefix( static_cast<uint64_t>( hlen ) * 4 - IPv4Header::LENGTH );

  // Verify checksum
  const uint16_t given_cksum = cksum;
  compute_checksum();
  if ( cksum != given_cksum ) {
    parser.set_error();
  }
}

// Serialize the IPv4Header (does not recompute the checksum)
void IPv4Header::serialize( Serializer& serializer ) const
{
  // consistency checks
  if ( ver != 4 ) {
    throw runtime_error( "wrong IP version" );
  }

  const uint8_t first_byte = ( static_cast<uint32_t>( ver ) << 4 ) | ( hlen & 0xfU );
  serializer.integer( first_byte ); // version and header length
  serializer.integer( tos );
  serializer.integer( len );
  serializer.integer( id );

  const uint16_t fo_val = ( df ? 0x4000U : 0 ) | ( mf ? 0x2000U : 0 ) | ( offset & 0x1fffU );
  serializer.integer( fo_val );

  serializer.integer( ttl );
  serializer.integer( proto );

  serializer.integer( cksum );

  serializer.integer( src );
  serializer.integer( dst );
}

uint16_t IPv4Header::payload_length() const
{
  return len - 4 * hlen;
}

//! \details This value is needed when computing the checksum of an encapsulated TCP segment.
//! ~~~{.txt}
//!   0      7 8     15 16    23 24    31
//!  +--------+--------+--------+--------+
//!  |          source address           |
//!  +--------+--------+--------+--------+
//!  |        destination address        |
//!  +--------+--------+--------+--------+
//!  |  zero  |protocol|  payload length |
//!  +--------+--------+--------+--------+
//! ~~~
uint32_t IPv4Header::pseudo_checksum() const
{
  uint32_t pcksum = ( src >> 16 ) + static_cast<uint16_t>( src ); // source addr
  pcksum += ( dst >> 16 ) + static_cast<uint16_t>( dst );
  pcksum += proto;            // protocol
  pcksum += payload_length(); // payload length
  return pcksum;
}

void IPv4Header::compute_checksum()
{
  cksum = 0;
  Serializer s;
  serialize( s );

  // calculate checksum -- taken over header only
  InternetChecksum check;
  check.add( s.output() );
  cksum = check.value();
}

std::string IPv4Header::to_string() const
{
  stringstream ss {};
  ss << hex << boolalpha << "IPv" << +ver << " len=" << dec << +len << " protocol=" << +proto
     << " ttl=" + ::to_string( ttl ) << " src=" << inet_ntoa( { htobe32( src ) } )
     << " dst=" << inet_ntoa( { htobe32( dst ) } );
  return ss.str();
}
