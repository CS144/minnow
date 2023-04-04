#include "byte_stream.hh"

#include <chrono>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <random>

using namespace std;
using namespace std::chrono;

void speed_test( const size_t input_len,   // NOLINT(bugprone-easily-swappable-parameters)
                 const size_t capacity,    // NOLINT(bugprone-easily-swappable-parameters)
                 const size_t random_seed, // NOLINT(bugprone-easily-swappable-parameters)
                 const size_t write_size,  // NOLINT(bugprone-easily-swappable-parameters)
                 const size_t read_size )  // NOLINT(bugprone-easily-swappable-parameters)
{
  // Generate the data to be written
  const string data = [&random_seed, &input_len] {
    default_random_engine rd { random_seed };
    uniform_int_distribution<char> ud;
    string ret;
    for ( size_t i = 0; i < input_len; ++i ) {
      ret += ud( rd );
    }
    return ret;
  }();

  // Split the data into segments before writing
  queue<string> split_data;
  for ( size_t i = 0; i < data.size(); i += write_size ) {
    split_data.emplace( data.substr( i, write_size ) );
  }

  ByteStream bs { capacity };
  string output_data;
  output_data.reserve( data.size() );

  const auto start_time = steady_clock::now();
  while ( not bs.reader().is_finished() ) {
    if ( split_data.empty() ) {
      if ( not bs.writer().is_closed() ) {
        bs.writer().close();
      }
    } else {
      if ( split_data.front().size() <= bs.writer().available_capacity() ) {
        bs.writer().push( move( split_data.front() ) );
        split_data.pop();
      }
    }

    if ( bs.reader().bytes_buffered() ) {
      auto peeked = bs.reader().peek().substr( 0, read_size );
      if ( peeked.empty() ) {
        throw runtime_error( "ByteStream::reader().peek() returned empty view" );
      }
      output_data += peeked;
      bs.reader().pop( peeked.size() );
    }
  }

  const auto stop_time = steady_clock::now();

  if ( data != output_data ) {
    throw runtime_error( "Mismatch between data written and read" );
  }

  auto test_duration = duration_cast<duration<double>>( stop_time - start_time );
  auto bytes_per_second = static_cast<double>( input_len ) / test_duration.count();
  auto bits_per_second = 8 * bytes_per_second;
  auto gigabits_per_second = bits_per_second / 1e9;

  fstream debug_output;
  debug_output.open( "/dev/tty" );

  cout << "ByteStream with capacity=" << capacity << ", write_size=" << write_size << ", read_size=" << read_size
       << " reached " << fixed << setprecision( 2 ) << gigabits_per_second << " Gbit/s.\n";

  debug_output << "             ByteStream throughput: " << fixed << setprecision( 2 ) << gigabits_per_second
               << " Gbit/s\n";

  if ( gigabits_per_second < 0.1 ) {
    throw runtime_error( "ByteStream did not meet minimum speed of 0.1 Gbit/s." );
  }
}

void program_body()
{
  speed_test( 1e7, 32768, 789, 1500, 128 );
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
