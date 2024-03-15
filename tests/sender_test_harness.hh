#pragma once

#include "common.hh"
#include "tcp_config.hh"
#include "tcp_receiver_message.hh"
#include "tcp_segment.hh"
#include "tcp_sender.hh"
#include "wrapping_integers.hh"

#include <optional>
#include <queue>
#include <sstream>
#include <utility>

const unsigned int DEFAULT_TEST_WINDOW = 137;

struct SenderAndOutput
{
  TCPSender sender;
  std::queue<TCPSenderMessage> output {};

  auto make_transmit()
  {
    return [&]( const TCPSenderMessage& x ) { output.push( x ); };
  }
};

inline std::string to_string( const TCPSenderMessage& msg )
{
  std::ostringstream o;
  o << "(";
  o << "seqno=" << msg.seqno;
  if ( msg.SYN ) {
    o << " +SYN";
  }
  if ( not msg.payload.empty() ) {
    o << " payload=\"" << Printer::prettify( msg.payload ) << "\"";
  }
  if ( msg.FIN ) {
    o << " +FIN";
  }
  o << ")";
  return o.str();
}

struct ExpectSeqno : public ExpectNumber<SenderAndOutput, Wrap32>
{
  using ExpectNumber::ExpectNumber;
  std::string name() const override { return "make_empty_message().seqno"; }

  Wrap32 value( SenderAndOutput& ss ) const override
  {
    auto seg = ss.sender.make_empty_message();
    if ( seg.sequence_length() ) {
      throw ExpectationViolation( "TCPSender::make_empty_message() returned non-empty message" );
    }
    return seg.seqno;
  }
};

struct ExpectReset : public ExpectBool<SenderAndOutput>
{
  using ExpectBool::ExpectBool;
  std::string name() const override { return "make_empty_message().RST"; }

  bool value( SenderAndOutput& ss ) const override { return ss.sender.make_empty_message().RST; }
};

struct ExpectSeqnosInFlight : public ExpectNumber<SenderAndOutput, uint64_t>
{
  using ExpectNumber::ExpectNumber;
  std::string name() const override { return "sequence_numbers_in_flight"; }
  uint64_t value( SenderAndOutput& ss ) const override { return ss.sender.sequence_numbers_in_flight(); }
};

struct ExpectConsecutiveRetransmissions : public ExpectNumber<SenderAndOutput, uint64_t>
{
  using ExpectNumber::ExpectNumber;
  std::string name() const override { return "consecutive_retransmissions"; }
  uint64_t value( SenderAndOutput& ss ) const override { return ss.sender.consecutive_retransmissions(); }
};

struct ExpectNoSegment : public Expectation<SenderAndOutput>
{
  std::string description() const override { return "nothing to send"; }
  void execute( SenderAndOutput& ss ) const override
  {
    if ( not ss.output.empty() ) {
      throw ExpectationViolation { "TCPSender sent an unexpected segment: " + to_string( ss.output.front() ) };
    }
  }
};

struct SetError : public Action<SenderAndOutput>
{
  std::string description() const override { return "set_error"; }
  void execute( SenderAndOutput& ss ) const override { ss.sender.writer().set_error(); }
};

struct HasError : public ExpectBool<SenderAndOutput>
{
  using ExpectBool::ExpectBool;
  std::string name() const override { return "has_error"; }
  bool value( SenderAndOutput& ss ) const override { return ss.sender.writer().has_error(); }
};

struct Push : public Action<SenderAndOutput>
{
  std::string data_;
  bool close_ {};

  explicit Push( std::string data = "" ) : data_( move( data ) ) {}
  std::string description() const override
  {
    if ( data_.empty() and not close_ ) {
      return "push TCPSender";
    }

    if ( data_.empty() and close_ ) {
      return "close stream, then push to TCPSender";
    }

    return "push \"" + Printer::prettify( data_ ) + "\" to stream" + ( close_ ? ", close it" : "" )
           + ", then push to TCPSender";
  }
  void execute( SenderAndOutput& ss ) const override
  {
    if ( not data_.empty() ) {
      ss.sender.writer().push( data_ );
    }
    if ( close_ ) {
      ss.sender.writer().close();
    }
    ss.sender.push( ss.make_transmit() );
  }

  Push& with_close()
  {
    close_ = true;
    return *this;
  }
};

struct Tick : public Action<SenderAndOutput>
{
  uint64_t ms_;
  std::optional<bool> max_retx_exceeded_ {};

  explicit Tick( uint64_t ms ) : ms_( ms ) {}

  Tick& with_max_retx_exceeded( bool val )
  {
    max_retx_exceeded_ = val;
    return *this;
  }

  std::string description() const override
  {
    std::ostringstream desc;
    desc << ms_ << " ms pass";
    if ( max_retx_exceeded_.has_value() ) {
      desc << " with max_retx_exceeded = " << max_retx_exceeded_.value();
    }
    return desc.str();
  }

  void execute( SenderAndOutput& ss ) const override
  {
    ss.sender.tick( ms_, ss.make_transmit() );
    if ( max_retx_exceeded_.has_value()
         and max_retx_exceeded_ != ( ss.sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS ) ) {
      std::ostringstream desc;
      desc << "after " << ms_ << " ms passed the TCP Sender reported\n\tconsecutive_retransmissions = "
           << ss.sender.consecutive_retransmissions() << "\nbut it should have been\n\t";
      if ( max_retx_exceeded_.value() ) {
        desc << "greater than ";
      } else {
        desc << "less than or equal to ";
      }
      desc << TCPConfig::MAX_RETX_ATTEMPTS << "\n";
      throw ExpectationViolation( desc.str() );
    }
  }
};

struct Receive : public Action<SenderAndOutput>
{
  TCPReceiverMessage msg_;
  bool push_ = true;

  explicit Receive( TCPReceiverMessage msg ) : msg_( msg ) {}
  std::string description() const override
  {
    std::ostringstream desc;
    desc << "receive(ack=" << to_string( msg_.ackno ) << ", win=" << msg_.window_size << ")";
    if ( push_ ) {
      desc << ", then push stream to TCPSender";
    }
    return desc.str();
  }

  Receive& with_win( uint16_t win )
  {
    msg_.window_size = win;
    return *this;
  }

  void execute( SenderAndOutput& ss ) const override
  {
    ss.sender.receive( msg_ );
    if ( push_ ) {
      ss.sender.push( ss.make_transmit() );
    }
  }

  Receive& without_push()
  {
    push_ = false;
    return *this;
  }
};

struct AckReceived : public Receive
{
  explicit AckReceived( Wrap32 ackno ) : Receive( { ackno, DEFAULT_TEST_WINDOW } ) {}
};

struct Close : public Push
{
  Close() : Push( "" ) { with_close(); }
};

struct ExpectMessage : public Expectation<SenderAndOutput>
{
  std::optional<bool> syn {};
  std::optional<bool> fin {};
  std::optional<bool> rst {};
  std::optional<Wrap32> seqno {};
  std::optional<std::string> data {};
  std::optional<size_t> payload_size {};

  ExpectMessage& with_syn( bool syn_ )
  {
    syn = syn_;
    return *this;
  }

  ExpectMessage& with_fin( bool fin_ )
  {
    fin = fin_;
    return *this;
  }

  ExpectMessage& with_no_flags()
  {
    syn = fin = rst = false;
    return *this;
  }

  ExpectMessage& with_rst( bool rst_ )
  {
    rst = rst_;
    return *this;
  }

  ExpectMessage& with_seqno( Wrap32 seqno_ )
  {
    seqno = seqno_;
    return *this;
  }

  ExpectMessage& with_seqno( uint32_t seqno_ ) { return with_seqno( Wrap32 { seqno_ } ); }

  ExpectMessage& with_payload_size( size_t payload_size_ )
  {
    payload_size = payload_size_;
    return *this;
  }

  ExpectMessage& with_data( std::string data_ )
  {
    data = std::move( data_ );
    return *this;
  }

  std::string message_description() const
  {
    std::ostringstream o;
    if ( seqno.has_value() ) {
      o << " seqno=" << seqno.value();
    }
    if ( syn.has_value() ) {
      o << ( syn.value() ? " +SYN" : " (no SYN)" );
    }
    if ( payload_size.has_value() ) {
      if ( payload_size.value() ) {
        o << " payload_len=" << payload_size.value();
      } else {
        o << " (no payload)";
      }
    }
    if ( data.has_value() ) {
      o << " payload=\"" << Printer::prettify( data.value() ) << "\"";
    }
    if ( fin.has_value() ) {
      o << ( fin.value() ? " +FIN" : " (no FIN)" );
    }
    if ( rst.has_value() ) {
      o << ( rst.value() ? " +RST" : " (no RST)" );
    }
    return o.str();
  }

  std::string description() const override { return "message sent with" + message_description(); }

  void execute( SenderAndOutput& ss ) const override
  {
    if ( payload_size.has_value() and data.has_value() and payload_size.value() != data.value().size() ) {
      throw std::runtime_error( "inconsistent test: invalid ExpectMessage" );
    }

    if ( ss.output.empty() ) {
      throw ExpectationViolation( "expected a message, but none was sent" );
    }

    const TCPSenderMessage& seg = ss.output.front();

    if ( syn.has_value() and seg.SYN != syn.value() ) {
      throw ExpectationViolation( "SYN flag", syn.value(), seg.SYN );
    }
    if ( fin.has_value() and seg.FIN != fin.value() ) {
      throw ExpectationViolation( "FIN flag", fin.value(), seg.FIN );
    }
    if ( rst.has_value() and seg.RST != rst.value() ) {
      throw ExpectationViolation( "RST flag", rst.value(), seg.RST );
    }
    if ( seqno.has_value() and seg.seqno != seqno.value() ) {
      throw ExpectationViolation( "sequence number", seqno.value(), seg.seqno );
    }
    if ( payload_size.has_value() and seg.payload.size() != payload_size.value() ) {
      throw ExpectationViolation( "payload_size", payload_size.value(), seg.payload.size() );
    }
    if ( seg.payload.size() > TCPConfig::MAX_PAYLOAD_SIZE ) {
      throw ExpectationViolation( "payload has length (" + std::to_string( seg.payload.size() )
                                  + ") greater than the maximum" );
    }
    if ( data.has_value() and data.value() != static_cast<std::string>( seg.payload ) ) {
      throw ExpectationViolation( "Expecting payload of \"" + Printer::prettify( data.value() )
                                  + "\", but instead it was \"" + Printer::prettify( seg.payload ) + "\"" );
    }

    ss.output.pop();
  }
};

class TCPSenderTestHarness : public TestHarness<SenderAndOutput>
{
public:
  TCPSenderTestHarness( std::string name, TCPConfig config )
    : TestHarness( move( name ),
                   "initial_RTO_ms=" + to_string( config.rt_timeout ),
                   { TCPSender { ByteStream { config.send_capacity }, config.isn, config.rt_timeout } } )
  {}
};
