#include "random.hh"
#include "receiver_test_harness.hh"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

int main()
{
  try {
    auto rd = get_random_engine();

    {
      TCPReceiverTestHarness test { "transmit 1", 4000 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( 0 ) );
      test.execute( SegmentArrives {}.with_seqno( 1 ).with_data( "abcd" ) );
      test.execute( ExpectAckno { Wrap32 { 5 } } );
      test.execute( ReadAll { "abcd" } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 4 } );
    }

    {
      const uint32_t isn = 384678;
      TCPReceiverTestHarness test { "transmit 2", 4000 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "abcd" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 5 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 4 } );
      test.execute( ReadAll { "abcd" } );
      test.execute( SegmentArrives {}.with_seqno( isn + 5 ).with_data( "efgh" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 9 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 8 } );
      test.execute( ReadAll { "efgh" } );
    }

    {
      const uint32_t isn = 5;
      TCPReceiverTestHarness test { "transmit 3", 4000 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "abcd" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 5 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 4 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 5 ).with_data( "efgh" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 9 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 8 } );
      test.execute( ReadAll { "abcdefgh" } );
    }

    // Many (arrive/read)s
    {
      TCPReceiverTestHarness test { "transmit 4", 4000 };
      const uint32_t max_block_size = 10;
      const uint32_t n_rounds = 10000;
      const uint32_t isn = 893472;
      size_t bytes_sent = 0;
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      for ( uint32_t i = 0; i < n_rounds; ++i ) {
        string data;
        const uint32_t block_size = uniform_int_distribution<uint32_t> { 1, max_block_size }( rd );
        for ( uint32_t j = 0; j < block_size; ++j ) {
          const uint8_t c = 'a' + ( ( i + j ) % 26 );
          data.push_back( static_cast<char>( c ) );
        }
        test.execute( ExpectAckno { make_optional<Wrap32>( isn + bytes_sent + 1 ) } );
        test.execute( BytesPushed { bytes_sent } );
        test.execute( SegmentArrives {}.with_seqno( isn + bytes_sent + 1 ).with_data( data ) );
        bytes_sent += block_size;
        test.execute( ReadAll { std::move( data ) } );
      }
    }

    // Many arrives, one read
    {
      const uint64_t max_block_size = 10;
      const uint64_t n_rounds = 100;
      TCPReceiverTestHarness test { "transmit 5", max_block_size * n_rounds };
      const uint32_t isn = 238;
      size_t bytes_sent = 0;
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      string all_data;
      for ( uint32_t i = 0; i < n_rounds; ++i ) {
        string data;
        const uint32_t block_size = uniform_int_distribution<uint32_t> { 1, max_block_size }( rd );
        for ( uint32_t j = 0; j < block_size; ++j ) {
          const uint8_t c = 'a' + ( ( i + j ) % 26 );
          const char ch = static_cast<char>( c );
          data.push_back( ch );
          all_data.push_back( ch );
        }
        test.execute( ExpectAckno { make_optional<Wrap32>( isn + bytes_sent + 1 ) } );
        test.execute( BytesPushed { bytes_sent } );
        test.execute( SegmentArrives {}.with_seqno( isn + bytes_sent + 1 ).with_data( data ) );
        bytes_sent += block_size;
      }
      test.execute( ReadAll { std::move( all_data ) } );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
