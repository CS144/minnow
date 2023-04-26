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
    {
      const size_t cap = 4000;
      const uint32_t isn = 23452;
      TCPReceiverTestHarness test { "window size decreases appropriately", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ExpectWindow { cap } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "abcd" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 5 } } );
      test.execute( ExpectWindow { cap - 4 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 9 ).with_data( "ijkl" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 5 } } );
      test.execute( ExpectWindow { cap - 4 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 5 ).with_data( "efgh" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 13 } } );
      test.execute( ExpectWindow { cap - 12 } );
    }

    {
      const size_t cap = 4000;
      const uint32_t isn = 23452;
      TCPReceiverTestHarness test { "window size expands after pop", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ExpectWindow { cap } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "abcd" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 5 } } );
      test.execute( ExpectWindow { cap - 4 } );
      test.execute( ReadAll { "abcd" } );
      test.execute( ExpectAckno { Wrap32 { isn + 5 } } );
      test.execute( ExpectWindow { cap } );
    }

    {
      const size_t cap = 2;
      const uint32_t isn = 23452;
      TCPReceiverTestHarness test { "arriving segment with high seqno", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 2 ).with_data( "bc" ) );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "a" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 3 } } );
      test.execute( ExpectWindow { 0 } );
      test.execute( BytesPushed { 2 } );
      test.execute( ReadAll { "ab" } );
      test.execute( ExpectWindow { 2 } );
    }

    {
      const size_t cap = 4;
      const uint32_t isn = 294058;
      TCPReceiverTestHarness test { "arriving segment with low seqno", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_data( "ab" ).with_seqno( isn + 1 ) );
      test.execute( BytesPushed { 2 } );
      test.execute( ExpectWindow { cap - 2 } );
      test.execute( SegmentArrives {}.with_data( "abc" ).with_seqno( isn + 1 ) );
      test.execute( BytesPushed { 3 } );
      test.execute( ExpectWindow { cap - 3 } );
      test.execute( ReadAll { "abc" } );
    }

    {
      const size_t cap = 4;
      const uint32_t isn = 23452;
      TCPReceiverTestHarness test { "segment overflowing window on left side", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "ab" ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 3 ).with_data( "cdef" ) );
      test.execute( ReadAll { "abcd" } );
    }

    {
      const size_t cap = 4;
      const uint32_t isn = 23452;
      TCPReceiverTestHarness test { "segment matching window", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "ab" ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 3 ).with_data( "cd" ) );
      test.execute( ReadAll { "abcd" } );
    }

    // credit for test: Jared Wasserman + Anonymous
    {
      const size_t cap = 4;
      const uint32_t isn = 23452;
      TCPReceiverTestHarness test { "byte with invalid stream index should be ignored", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_seqno( isn ).with_data( "a" ) );
      test.execute( BytesPushed { 0 } );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( BytesPending( 0 ) );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
