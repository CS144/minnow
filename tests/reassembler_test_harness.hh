#pragma once

#include "byte_stream_test_harness.hh"
#include "common.hh"
#include "reassembler.hh"

#include <optional>
#include <sstream>
#include <utility>

using StreamAndReassembler = std::pair<ByteStream, Reassembler>;

template<std::derived_from<TestStep<ByteStream>> T>
struct ReassemblerByteStreamTestStep : public TestStep<StreamAndReassembler>
{
  T step_;

  template<typename... Targs>
  explicit ReassemblerByteStreamTestStep( T byte_stream_test_step )
    : TestStep<StreamAndReassembler>(), step_( std::move( byte_stream_test_step ) )
  {}

  std::string str() const override { return step_.str(); }
  uint8_t color() const override { return step_.color(); }
  void execute( StreamAndReassembler& sr ) const override { step_.execute( sr.first ); }
};

class ReassemblerTestHarness : public TestHarness<StreamAndReassembler>
{
public:
  ReassemblerTestHarness( std::string test_name, uint64_t capacity )
    : TestHarness( move( test_name ),
                   "capacity=" + std::to_string( capacity ),
                   { ByteStream { capacity }, Reassembler {} } )
  {}

  template<std::derived_from<TestStep<ByteStream>> T>
  void execute( const T& test )
  {
    TestHarness<StreamAndReassembler>::execute( ReassemblerByteStreamTestStep { test } );
  }

  using TestHarness<StreamAndReassembler>::execute;
};

struct BytesPending : public ExpectNumber<StreamAndReassembler, uint64_t>
{
  using ExpectNumber::ExpectNumber;
  std::string name() const override { return "bytes_pending"; }
  uint64_t value( StreamAndReassembler& sr ) const override { return sr.second.bytes_pending(); }
};

struct Insert : public Action<StreamAndReassembler>
{
  std::string data_;
  uint64_t first_index_;
  bool is_last_substring_ {};

  Insert( std::string data, uint64_t first_index ) : data_( move( data ) ), first_index_( first_index ) {}

  Insert& is_last( bool status = true )
  {
    is_last_substring_ = status;
    return *this;
  }

  std::string description() const override
  {
    std::ostringstream ss;
    ss << "insert \"" << Printer::prettify( data_ ) << "\" @ index " << first_index_;
    if ( is_last_substring_ ) {
      ss << " [last substring]";
    }
    return ss.str();
  }

  void execute( StreamAndReassembler& sr ) const override
  {
    sr.second.insert( first_index_, data_, is_last_substring_, sr.first.writer() );
  }
};
