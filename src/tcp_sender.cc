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

// void TCPSender::push( const TransmitFunction& transmit )
// {
//   if  ( report_window_size && abs_sender_num-abs_acked_num >= report_window_size  ) {
//     return;
//   }
//   string payload {};
//   if(!my_timer.is_running()) my_timer.state_reset(initial_RTO_ms_);
//   TCPSenderMessage my_tcp_msg;
//   Wrap32 tcp_32_seq = Wrap32::wrap(abs_sender_num,isn_);
//   bool has_finished=reader().is_finished();
//   if(!report_window_size){
//     my_tcp_msg = {tcp_32_seq,false,payload,false,reader().has_error()};
//     transmit(my_tcp_msg);
//     return;
//   }
//   uint16_t win = report_window_size - sequence_numbers_in_flight() - static_cast<uint16_t>( tcp_32_seq == isn_ );
//   while(win || tcp_32_seq==isn_ ||  (!FIN&& has_finished)){
//     uint16_t len = win<TCPConfig::MAX_PAYLOAD_SIZE?win:TCPConfig::MAX_PAYLOAD_SIZE;
//     win-=len;
//     read(input_.reader(),len,payload);
//     my_tcp_msg ={tcp_32_seq,tcp_32_seq==isn_,payload,false,reader().has_error()};

//     if ( !FIN && has_finished && win==0
//          && ( sequence_numbers_in_flight() + my_tcp_msg.sequence_length() < report_window_size
//               || ( report_window_size == 0 && my_tcp_msg.sequence_length() == 0 ) ) ) {
//       FIN = my_tcp_msg.FIN = true;
//     }
//     transmit(my_tcp_msg);
//     my_sender_queue.push(my_tcp_msg);
//     abs_sender_num+=my_tcp_msg.sequence_length();
//   if ( !FIN && has_finished && len == payload.size() ) {
//     break;
//   }
//     tcp_32_seq = Wrap32::wrap(abs_sender_num,isn_);
//   }
  
// }

void TCPSender::push( const TransmitFunction& transmit )
{
  // 1.达到最大传输字节数（窗口大小）
  // 2.仅传输中的序列号没了且window_size=0才需要发送假消息，如果还有序列号才传输中，可以利用这些得到ack更新size（传输失败就重传）
  if ( ( report_window_size && sequence_numbers_in_flight() >= report_window_size )
       || ( report_window_size == 0 && sequence_numbers_in_flight() >= 1 ) ) {
    return;
  }

  auto seqno = Wrap32::wrap( abs_sender_num, isn_ );

  // 限制从buffer中取出来的字节数
  auto win
    = report_window_size == 0 ? 1 : report_window_size - sequence_numbers_in_flight() - static_cast<uint16_t>( seqno == isn_ );

  string out;
  read(input_.reader(),win,out);

  size_t len;
  string_view view( out );

  while ( !view.empty() || seqno == isn_ || ( !FIN_ && writer().is_closed() ) ) {
    len = min( view.size(), TCPConfig::MAX_PAYLOAD_SIZE );

    string payload( view.substr( 0, len ) );

    TCPSenderMessage message { seqno, seqno == isn_, move( payload ), false, writer().has_error() };

    // 1.当前窗口大小限制携带不了FIN，留着以后发，没有新的消息了直接退出，否则携带
    // 2.zero窗口仅当message为0时才能携带（因为视为窗口大小为1）
    if ( !FIN_ && writer().is_closed() && len == view.size()
         && ( sequence_numbers_in_flight() + message.sequence_length() < report_window_size
              || ( report_window_size == 0 && message.sequence_length() == 0 ) ) ) {
      FIN_ = message.FIN = true;
    }

    transmit( message );

    abs_sender_num += message.sequence_length();
    my_sender_queue.emplace( move( message ) );

    // 当前窗口大小限制携带不了FIN，留着以后发，没有新的消息了直接退出
    if ( !FIN_ && writer().is_closed() && len == view.size() ) {
      break;
    }
    seqno = Wrap32::wrap( abs_sender_num, isn_ );
    view.remove_prefix( len );
  }
}



TCPSenderMessage TCPSender::make_empty_message() const
{
  Wrap32 temp = Wrap32::wrap(abs_sender_num,isn_);
  return TCPSenderMessage {temp,false,"",false,reader().has_error()};
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
