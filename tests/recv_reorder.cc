#include "random.hh"
#include "receiver_test_harness.hh"

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
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "in-window, later segment", 2358 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( SegmentArrives {}.with_seqno( isn + 10 ).with_data( "abcd" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 4 } );
      test.execute( BytesPushed { 0 } );
    }

    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "in-window, later segment, then hole filled", 2358 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( SegmentArrives {}.with_seqno( isn + 5 ).with_data( "efgh" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 4 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "abcd" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 9 } } );
      test.execute( ReadAll { "abcdefgh" } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 8 } );
    }

    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "hole filled bit-by-bit", 2358 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( SegmentArrives {}.with_seqno( isn + 5 ).with_data( "efgh" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 4 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "ab" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 3 } } );
      test.execute( ReadAll { "ab" } );
      test.execute( BytesPending { 4 } );
      test.execute( BytesPushed { 2 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 3 ).with_data( "cd" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 9 } } );
      test.execute( ReadAll { "cdefgh" } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 8 } );
    }

    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "many gaps, filled bit-by-bit", 2358 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( SegmentArrives {}.with_seqno( isn + 5 ).with_data( "e" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 1 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 7 ).with_data( "g" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 2 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 3 ).with_data( "c" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 3 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "ab" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 4 } } );
      test.execute( ReadAll { "abc" } );
      test.execute( BytesPending { 2 } );
      test.execute( BytesPushed { 3 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 6 ).with_data( "f" ) );
      test.execute( BytesPending { 3 } );
      test.execute( BytesPushed { 3 } );
      test.execute( ReadAll { "" } );
      test.execute( SegmentArrives {}.with_seqno( isn + 4 ).with_data( "d" ) );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 7 } );
      test.execute( ReadAll { "defg" } );
    }

    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "many gaps, then subsumed", 2358 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( SegmentArrives {}.with_seqno( isn + 5 ).with_data( "e" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 1 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 7 ).with_data( "g" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 2 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 3 ).with_data( "c" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPending { 3 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "abcdefgh" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 9 } } );
      test.execute( ReadAll { "abcdefgh" } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 8 } );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
