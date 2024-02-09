#pragma once

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_receiver_message.hh"
#include "tcp_segment.hh"
#include "tcp_sender.hh"
#include "tcp_sender_message.hh"

#include <functional>
#include <optional>

class TCPPeer
{
  auto make_send( const auto& transmit )
  {
    return [&]( const TCPSenderMessage& x ) { send( x, transmit ); };
  }

public:
  explicit TCPPeer( const TCPConfig& cfg ) : cfg_( cfg ) {}

  Writer& outbound_writer() { return sender_.writer(); }
  Reader& inbound_reader() { return receiver_.reader(); }

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( TCPMessage )>;

  /* Passthrough methods */
  void push( const TransmitFunction& transmit ) { sender_.push( make_send( transmit ) ); }
  void tick( uint64_t t, const TransmitFunction& transmit )
  {
    cumulative_time_ += t;
    sender_.tick( t, make_send( transmit ) );
  }
  bool has_ackno() const { return receiver_.send().ackno.has_value(); }

  /* Is the peer still active? */
  bool active() const
  {
    const bool any_errors = receiver_.reader().has_error() or sender_.writer().has_error();
    const bool sender_active = sender_.sequence_numbers_in_flight() or not sender_.reader().is_finished();
    const bool receiver_active = not receiver_.writer().is_closed();
    const bool lingering
      = linger_after_streams_finish_ and ( cumulative_time_ < time_of_last_receipt_ + 10UL * cfg_.rt_timeout );

    return ( not any_errors ) and ( sender_active or receiver_active or lingering );
  }

  void receive( TCPMessage msg, const TransmitFunction& transmit )
  {
    if ( not active() ) {
      return;
    }

    // Record time in case this peer has to linger after streams finish.
    time_of_last_receipt_ = cumulative_time_;

    // If SenderMessage occupies a sequence number, make sure to reply.
    need_send_ |= ( msg.sender.sequence_length() > 0 );

    // If SenderMessage is a "keep-alive" (with intentionally invalid seqno), make sure to reply.
    // (N.B. orthodox TCP rules require a reply on any unacceptable segment.)
    const auto our_ackno = receiver_.send().ackno;
    need_send_ |= ( our_ackno.has_value() and msg.sender.seqno + 1 == our_ackno.value() );

    // Did the inbound stream finish before the outbound stream? If so, no need to linger after streams finish.
    if ( receiver_.writer().is_closed() and not sender_.reader().is_finished() ) {
      linger_after_streams_finish_ = false;
    }

    // Give incoming TCPSenderMessage to receiver.
    receiver_.receive( std::move( msg.sender ) );

    // Give incoming TCPReceiverMessage to sender.
    sender_.receive( msg.receiver );

    // Send reply if needed.
    if ( need_send_ ) {
      send( sender_.make_empty_message(), transmit );
    }
  }

  // Testing interface
  const TCPReceiver& receiver() const { return receiver_; }
  const TCPSender& sender() const { return sender_; }

private:
  TCPConfig cfg_;
  TCPSender sender_ { ByteStream { cfg_.send_capacity }, cfg_.isn, cfg_.rt_timeout };
  TCPReceiver receiver_ { Reassembler { ByteStream { cfg_.recv_capacity } } };

  bool need_send_ {};

  void send( const TCPSenderMessage& sender_message, const TransmitFunction& transmit )
  {
    TCPMessage msg { sender_message, receiver_.send() };
    transmit( std::move( msg ) );
    need_send_ = false;
  }

  bool linger_after_streams_finish_ { true }; // one peer may need to linger to make sure all closure conditions met
  uint64_t cumulative_time_ {};
  uint64_t time_of_last_receipt_ {};
};
