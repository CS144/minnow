#include "tcp_segment.hh"
#include "checksum.hh"
#include "wrapping_integers.hh"

#include <cstddef>

static constexpr uint32_t TCPHeaderMinLen = 5; // 32-bit words

using namespace std;

void TCPSegment::parse( Parser& parser, uint32_t datagram_layer_pseudo_checksum )
{
  /* verify checksum */
  InternetChecksum check { datagram_layer_pseudo_checksum };
  check.add( parser.buffer() );
  if ( check.value() ) {
    parser.set_error();
    return;
  }

  uint32_t raw32 {};
  uint16_t raw16 {};
  uint8_t octet {};

  parser.integer( udinfo.src_port );
  parser.integer( udinfo.dst_port );

  parser.integer( raw32 );
  message.sender.seqno = Wrap32 { raw32 };

  parser.integer( raw32 );
  message.receiver.ackno = Wrap32 { raw32 };

  parser.integer( octet );
  const uint8_t data_offset = octet >> 4;

  parser.integer( octet ); // flags
  if ( not( octet & 0b0001'0000 ) ) {
    message.receiver.ackno.reset(); // no ACK
  }

  message.sender.RST = message.receiver.RST = octet & 0b0000'0100;
  message.sender.SYN = octet & 0b0000'0010;
  message.sender.FIN = octet & 0b0000'0001;

  parser.integer( message.receiver.window_size );
  parser.integer( udinfo.cksum );
  parser.integer( raw16 ); // urgent pointer

  // skip any options or anything extra in the header
  if ( data_offset < TCPHeaderMinLen ) {
    parser.set_error();
  }
  parser.remove_prefix( data_offset * 4 - TCPHeaderMinLen * 4 );

  parser.all_remaining( message.sender.payload );
}

class Wrap32Serializable : public Wrap32
{
public:
  uint32_t raw_value() const { return raw_value_; }
};

void TCPSegment::serialize( Serializer& serializer ) const
{
  serializer.integer( udinfo.src_port );
  serializer.integer( udinfo.dst_port );
  serializer.integer( Wrap32Serializable { message.sender.seqno }.raw_value() );
  serializer.integer( Wrap32Serializable { message.receiver.ackno.value_or( Wrap32 { 0 } ) }.raw_value() );
  serializer.integer( uint8_t { TCPHeaderMinLen << 4 } ); // data offset
  const bool reset = message.sender.RST or message.receiver.RST;
  const uint8_t flags = ( message.receiver.ackno.has_value() ? 0b0001'0000U : 0 ) | ( reset ? 0b0000'0100U : 0 )
                        | ( message.sender.SYN ? 0b0000'0010U : 0 ) | ( message.sender.FIN ? 0b0000'0001U : 0 );
  serializer.integer( flags );
  serializer.integer( message.receiver.window_size );
  serializer.integer( udinfo.cksum );
  serializer.integer( uint16_t { 0 } ); // urgent pointer
  serializer.buffer( message.sender.payload );
}

void TCPSegment::compute_checksum( uint32_t datagram_layer_pseudo_checksum )
{
  udinfo.cksum = 0;
  Serializer s;
  serialize( s );

  InternetChecksum check { datagram_layer_pseudo_checksum };
  check.add( s.output() );
  udinfo.cksum = check.value();
}
