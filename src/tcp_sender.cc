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
  if(!my_timer.is_running()) my_timer.state_reset(initial_RTO_ms_);
  TCPSenderMessage my_tcp_msg;
  string payload {};
  if(!abs_sender_num){
    my_tcp_msg = {isn_,true,"",false,false};
  }else{
    uint16_t len = report_window_size>reader().bytes_buffered()?reader().bytes_buffered():report_window_size;
    read(input_.reader(),len,payload);
    Wrap32 tcp_32_seq = Wrap32::wrap(abs_sender_num,isn_);
    my_tcp_msg ={tcp_32_seq,false,move(payload),reader().is_finished(),reader().has_error()};
  }
  my_sender_queue.push(my_tcp_msg);
  transmit(my_tcp_msg);
  abs_sender_num+=my_tcp_msg.sequence_length();

  
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
    while(!my_sender_queue.empty() && temp>(abs_acked_num + my_sender_queue.front().sequence_length())){
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
    TCPSenderMessage temp=my_sender_queue.front();
    transmit(temp);
    my_sender_queue.push(temp);
    my_sender_queue.pop();
    if(report_window_size){
      my_timer.add_count();
      my_timer.state_reset(my_timer.get_current_RTO()*2);
    }
  }
}
