#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return abs_sender_num-abs_acked_num;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return my_timer.peek_count();
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if  ( report_window_size && abs_sender_num-abs_acked_num >= report_window_size  ) {
    return;
  }
  string payload {};
  if(!my_timer.is_running()) my_timer.state_reset(initial_RTO_ms_);
  TCPSenderMessage my_tcp_msg;
  Wrap32 tcp_32_seq = Wrap32::wrap(abs_sender_num,isn_);
  bool has_finished=reader().is_finished();
  if(!report_window_size){
    my_tcp_msg = {tcp_32_seq,false,payload,false,false};
    transmit(my_tcp_msg);
    return;
  }
  uint16_t win = report_window_size - sequence_numbers_in_flight() - static_cast<uint16_t>( tcp_32_seq == isn_ );
  while(win || tcp_32_seq==isn_ ||  (!FIN&& has_finished)){
    uint16_t len = win<TCPConfig::MAX_PAYLOAD_SIZE?win:TCPConfig::MAX_PAYLOAD_SIZE;
    win-=len;
    read(input_.reader(),len,payload);
    my_tcp_msg.seqno=tcp_32_seq;
    my_tcp_msg.SYN= tcp_32_seq==isn_;
    my_tcp_msg.FIN= false;
    my_tcp_msg.RST=reader().has_error();
    my_tcp_msg.payload=payload;

    if ( !FIN && has_finished && win==0
         && ( sequence_numbers_in_flight() + my_tcp_msg.sequence_length() < report_window_size
              || ( report_window_size == 0 && my_tcp_msg.sequence_length() == 0 ) ) ) {
      FIN = my_tcp_msg.FIN = true;
    }
    transmit(my_tcp_msg);
    my_sender_queue.push(my_tcp_msg);
    abs_sender_num+=my_tcp_msg.sequence_length();
    tcp_32_seq = Wrap32::wrap(abs_sender_num,isn_);
  }
  
}


TCPSenderMessage TCPSender::make_empty_message() const
{
  Wrap32 temp = Wrap32::wrap(abs_sender_num,isn_);
  return TCPSenderMessage {temp,false,"",false,false};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if(msg.RST) {
    writer().set_error();
    return;
  }
  report_window_size=msg.window_size;
  if(msg.ackno.has_value()){
    uint64_t temp = (*msg.ackno).unwrap(isn_,abs_acked_num);

    while(!my_sender_queue.empty() && temp>=(abs_acked_num + my_sender_queue.front().sequence_length())&&temp<=abs_sender_num){
      abs_acked_num+=my_sender_queue.front().sequence_length();
      my_sender_queue.pop();
      my_timer.state_reset(initial_RTO_ms_);
      my_timer.clear_count();
    }
  }
  if(my_sender_queue.empty()){
    my_timer.state_off();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if(my_timer.check_out_of_date(ms_since_last_tick)){
    transmit(my_sender_queue.front());
    if(report_window_size){
      my_timer.add_count();
      my_timer.state_reset(my_timer.get_current_RTO()*2);
    }
  }
}
