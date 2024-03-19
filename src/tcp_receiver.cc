#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }
  if ( !have_SYN && message.SYN ) {
    zero_point = message.seqno;
    have_SYN = true;
  }
  if ( !have_SYN )
    return;
  uint64_t check_point = reassembler_.writer().bytes_pushed();
  uint64_t first_index = message.seqno.unwrap( zero_point, check_point );
  if ( message.SYN ) {
    reassembler_.insert( first_index, message.payload, message.FIN );
  } else {
    reassembler_.insert( first_index - 1, message.payload, message.FIN );
  }
  next_connect = Wrap32::wrap( reassembler_.writer().bytes_pushed() + have_SYN + reassembler_.writer().is_closed(),
                               zero_point );
}

TCPReceiverMessage TCPReceiver::send() const
{
  uint64_t actually_capacity = reassembler_.writer().available_capacity();
  uint16_t window_size = actually_capacity <= UINT16_MAX ? actually_capacity : UINT16_MAX;
  return { next_connect, window_size, reassembler_.reader().has_error() };
}
