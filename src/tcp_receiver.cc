#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  uint64_t checkpoint = reassembler_.writer().bytes_pushed() + syn_flag_;
  if ( message.RST ) {
    reassembler_.reader().set_error();
  } else if ( checkpoint > 0 && checkpoint <= UINT32_MAX && message.seqno == isn_ )
    return;
  if ( !syn_flag_ && !message.SYN )
    return;
  if ( !syn_flag_ ) {
    isn_ = Wrap32( message.seqno );
    syn_flag_ = true;
  }
  uint64_t pos = message.seqno.unwrap( isn_, checkpoint );
  if ( pos == 0 ) {
    reassembler_.insert( pos, message.payload, message.FIN );
  } else {
    reassembler_.insert( pos - 1, message.payload, message.FIN );
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  const uint64_t checkpoint = reassembler_.writer().bytes_pushed() + syn_flag_;
  const uint64_t capacity = reassembler_.writer().available_capacity();
  const uint16_t wnd_size = capacity > UINT16_MAX ? UINT16_MAX : capacity;
  if ( !syn_flag_ )
    return { {}, wnd_size, reassembler_.writer().has_error() };
  return { Wrap32::wrap( checkpoint + reassembler_.writer().is_closed(), isn_ ),
           wnd_size,
           reassembler_.writer().has_error() };
}
