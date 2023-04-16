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

    /* segment before SYN */
    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "segment before SYN", 4000 };
      test.execute( HasAckno { false } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "hello" ).without_ackno() );
      test.execute( HasAckno { false } );
      test.execute( BytesPending { 0 } );
      test.execute( ReadAll { "" } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( HasAckno { true } );
      test.execute( IsClosed { false } );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
    }

    /* segment with SYN + data */
    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "segment with SYN + data", 4000 };
      test.execute( HasAckno { false } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ).with_data( "Hello, CS144!" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 14 } } );
      test.execute( BytesPending { 0 } );
      test.execute( ReadAll { "Hello, CS144!" } );
      test.execute( IsClosed { false } );
    }

    /* empty segment */
    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "empty segment", 4000 };
      test.execute( HasAckno { false } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( BytesPending { 0 } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn + 1 ) );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( IsClosed { false } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn + 5 ) );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( IsClosed { false } );
    }

    /* segment with null byte */
    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "segment with null byte", 4000 };
      const string text = "Here's a null byte:"s + '\0' + "and it's gone."s;
      test.execute( HasAckno { false } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( text ) );
      test.execute( ReadAll { string( text ) } );
      test.execute( ExpectAckno { Wrap32 { isn + 35 } } );
      test.execute( IsClosed { false } );
    }

    /* segment with data + FIN */
    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "segment with data + FIN", 4000 };
      test.execute( HasAckno { false } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_fin().with_data( "Goodbye, CS144!" ).with_seqno( isn + 1 ) );
      test.execute( IsClosed { true } );
      test.execute( ReadAll { "Goodbye, CS144!" } );
      test.execute( ExpectAckno { Wrap32 { isn + 17 } } );
      test.execute( IsFinished { true } );
    }

    /* segment with FIN (but can't be assembled yet) */
    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "segment with FIN (but can't be assembled yet)", 4000 };
      test.execute( HasAckno { false } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_fin().with_data( "oodbye, CS144!" ).with_seqno( isn + 2 ) );
      test.execute( ReadAll { "" } );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( IsClosed { false } );
      test.execute( SegmentArrives {}.with_data( "G" ).with_seqno( isn + 1 ) );
      test.execute( IsClosed { true } );
      test.execute( ReadAll { "Goodbye, CS144!" } );
      test.execute( ExpectAckno { Wrap32 { isn + 17 } } );
      test.execute( IsFinished { true } );
    }

    /* segment with SYN + data + FIN */
    {
      const uint32_t isn = uniform_int_distribution<uint32_t> { 0, UINT32_MAX }( rd );
      TCPReceiverTestHarness test { "segment with SYN + payload + FIN", 4000 };
      test.execute( HasAckno { false } );
      test.execute(
        SegmentArrives {}.with_syn().with_seqno( isn ).with_data( "Hello and goodbye, CS144!" ).with_fin() );
      test.execute( IsClosed { true } );
      test.execute( ExpectAckno { Wrap32 { isn + 27 } } );
      test.execute( BytesPending { 0 } );
      test.execute( ReadAll { "Hello and goodbye, CS144!" } );
      test.execute( IsFinished { true } );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
