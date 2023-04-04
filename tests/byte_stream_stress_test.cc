#include "byte_stream_test_harness.hh"

#include <iostream>
#include <random>

using namespace std;

void stress_test( const size_t input_len,    // NOLINT(bugprone-easily-swappable-parameters)
                  const size_t capacity,     // NOLINT(bugprone-easily-swappable-parameters)
                  const size_t random_seed ) // NOLINT(bugprone-easily-swappable-parameters)
{
  default_random_engine rd { random_seed };

  const string data = [&rd, &input_len] {
    uniform_int_distribution<char> ud;
    string ret;
    for ( size_t i = 0; i < input_len; ++i ) {
      ret += ud( rd );
    }
    return ret;
  }();

  ByteStreamTestHarness bs { "stress test input=" + to_string( input_len ) + ", capacity=" + to_string( capacity ),
                             capacity };

  size_t expected_bytes_pushed {};
  size_t expected_bytes_popped {};
  size_t expected_available_capacity { capacity };
  while ( expected_bytes_pushed < data.size() or expected_bytes_popped < data.size() ) {
    bs.execute( BytesPushed { expected_bytes_pushed } );
    bs.execute( BytesPopped { expected_bytes_popped } );
    bs.execute( AvailableCapacity { expected_available_capacity } );
    bs.execute( BytesBuffered { expected_bytes_pushed - expected_bytes_popped } );

    /* write something */
    uniform_int_distribution<size_t> bytes_to_push_dist { 0, data.size() - expected_bytes_pushed };
    const size_t amount_to_push = bytes_to_push_dist( rd );
    bs.execute( Push { data.substr( expected_bytes_pushed, amount_to_push ) } );
    expected_bytes_pushed += min( amount_to_push, expected_available_capacity );
    expected_available_capacity -= min( amount_to_push, expected_available_capacity );

    bs.execute( BytesPushed { expected_bytes_pushed } );
    bs.execute( AvailableCapacity { expected_available_capacity } );

    if ( expected_bytes_pushed == data.size() ) {
      bs.execute( Close {} );
    }

    /* read something */
    const size_t peek_size = bs.peek_size();
    if ( ( expected_bytes_pushed != expected_bytes_popped ) and peek_size == 0 ) {
      throw runtime_error( "ByteStream::reader().peek() returned empty view" );
    }
    if ( expected_bytes_popped + peek_size > expected_bytes_pushed ) {
      throw runtime_error( "ByteStream::reader().peek() returned too-large view" );
    }

    bs.execute( PeekOnce { data.substr( expected_bytes_popped, peek_size ) } );

    uniform_int_distribution<size_t> bytes_to_pop_dist { 0, peek_size };
    const size_t amount_to_pop = bytes_to_pop_dist( rd );

    bs.execute( Pop { amount_to_pop } );
    expected_bytes_popped += amount_to_pop;
    expected_available_capacity += amount_to_pop;
    bs.execute( BytesPopped { expected_bytes_popped } );
  }

  bs.execute( IsClosed { true } );
  bs.execute( IsFinished { true } );
}

void program_body()
{
  stress_test( 19, 3, 10110 );
  stress_test( 18, 17, 12345 );
  stress_test( 1111, 17, 98765 );
  stress_test( 4097, 4096, 11101 );
}

int main()
{
  try {
    program_body();
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
