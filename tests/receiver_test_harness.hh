#pragma once

#include "common.hh"
#include "reassembler_test_harness.hh"
#include "tcp_receiver.hh"
#include "tcp_receiver_message.hh"

#include <optional>
#include <sstream>
#include <utility>

template<std::derived_from<TestStep<Reassembler>> T>
struct DirectReassemblerTest : public TestStep<TCPReceiver>
{
  T step_;

  template<typename... Targs>
  explicit DirectReassemblerTest( T sr_test_step ) : TestStep<TCPReceiver>(), step_( std::move( sr_test_step ) )
  {}

  std::string str() const override { return step_.str(); }
  uint8_t color() const override { return step_.color(); }
  void execute( TCPReceiver& sr ) const override { step_.execute( sr.reassembler() ); }
};

template<std::derived_from<TestStep<ByteStream>> T>
struct DirectByteStreamTest : public TestStep<TCPReceiver>
{
  T step_;

  template<typename... Targs>
  explicit DirectByteStreamTest( T sr_test_step ) : TestStep<TCPReceiver>(), step_( std::move( sr_test_step ) )
  {}

  std::string str() const override { return step_.str(); }
  uint8_t color() const override { return step_.color(); }
  void execute( TCPReceiver& sr ) const override { step_.execute( sr.reader() ); }
};

class TCPReceiverTestHarness : public TestHarness<TCPReceiver>
{
public:
  TCPReceiverTestHarness( std::string test_name, uint64_t capacity )
    : TestHarness( move( test_name ),
                   "capacity=" + std::to_string( capacity ),
                   { TCPReceiver { Reassembler { ByteStream { capacity } } } } )
  {}

  template<std::derived_from<TestStep<Reassembler>> T>
  void execute( const T& test )
  {
    TestHarness<TCPReceiver>::execute( DirectReassemblerTest { test } );
  }

  template<std::derived_from<TestStep<ByteStream>> T>
  void execute( const T& test )
  {
    TestHarness<TCPReceiver>::execute( DirectByteStreamTest { test } );
  }

  using TestHarness<TCPReceiver>::execute;
};

struct ExpectWindow : public ExpectNumber<TCPReceiver, uint16_t>
{
  using ExpectNumber::ExpectNumber;
  std::string name() const override { return "window_size"; }
  uint16_t value( TCPReceiver& rs ) const override { return rs.send().window_size; }
};

struct ExpectAckno : public ExpectNumber<TCPReceiver, std::optional<Wrap32>>
{
  using ExpectNumber::ExpectNumber;
  std::string name() const override { return "ackno"; }
  std::optional<Wrap32> value( TCPReceiver& rs ) const override { return rs.send().ackno; }
};

struct ExpectReset : public ExpectBool<TCPReceiver>
{
  using ExpectBool::ExpectBool;
  std::string name() const override { return "RST"; }

  bool value( TCPReceiver& rs ) const override { return rs.send().RST; }
};

struct ExpectAcknoBetween : public Expectation<TCPReceiver>
{
  Wrap32 isn_;
  uint64_t checkpoint_;
  uint64_t min_, max_;
  ExpectAcknoBetween( Wrap32 isn, uint64_t checkpoint, uint64_t min, uint64_t max ) // NOLINT(*-swappable-*)
    : isn_( isn ), checkpoint_( checkpoint ), min_( min ), max_( max )
  {}

  std::string description() const override
  {
    return "ackno unwraps to between " + to_string( min_ ) + " and " + to_string( max_ );
  }

  void execute( TCPReceiver& rs ) const override
  {
    auto ackno = rs.send().ackno;
    if ( not ackno.has_value() ) {
      throw ExpectationViolation( "TCPReceiver did not have ackno when expected" );
    }
    const uint64_t ackno_absolute = ackno.value().unwrap( isn_, checkpoint_ );

    if ( ackno_absolute < min_ or ackno_absolute > max_ ) {
      throw ExpectationViolation( "ackno outside expected range" );
    }
  }
};

struct HasAckno : public ExpectBool<TCPReceiver>
{
  using ExpectBool::ExpectBool;
  std::string name() const override { return "ackno.has_value()"; }
  bool value( TCPReceiver& rs ) const override { return rs.send().ackno.has_value(); }
};

struct SegmentArrives : public Action<TCPReceiver>
{
  TCPSenderMessage msg_ {};
  HasAckno ackno_expected_ { true };

  SegmentArrives& with_syn()
  {
    msg_.SYN = true;
    return *this;
  }

  SegmentArrives& with_fin()
  {
    msg_.FIN = true;
    return *this;
  }

  SegmentArrives& with_rst()
  {
    msg_.RST = true;
    return *this;
  }

  SegmentArrives& with_seqno( Wrap32 seqno_ )
  {
    msg_.seqno = seqno_;
    return *this;
  }

  SegmentArrives& with_seqno( uint32_t seqno_ ) { return with_seqno( Wrap32 { seqno_ } ); }

  SegmentArrives& with_data( std::string data )
  {
    msg_.payload = move( data );
    return *this;
  }

  SegmentArrives& without_ackno()
  {
    ackno_expected_ = HasAckno { false };
    return *this;
  }

  void execute( TCPReceiver& rs ) const override
  {
    rs.receive( msg_ );
    ackno_expected_.execute( rs );
  }

  std::string description() const override
  {
    std::ostringstream ss;
    ss << "receive segment: (";
    ss << "seqno=" << msg_.seqno;
    if ( msg_.SYN ) {
      ss << " +SYN";
    }
    if ( not msg_.payload.empty() ) {
      ss << " payload=\"" << Printer::prettify( msg_.payload ) << "\"";
    }
    if ( msg_.FIN ) {
      ss << " +FIN";
    }
    ss << ")";

    if ( ackno_expected_.value_ ) {
      ss << " with ackno expected";
    } else {
      ss << " with ackno not expected";
    }
    return ss.str();
  }
};
