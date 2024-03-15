#include "reassembler_test_harness.hh"
#include "receiver_test_harness.hh"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

int main()
{
  try {
    {
      TCPReceiverTestHarness test { "connect 1", 4000 };
      test.execute( ExpectWindow { 4000 } );
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( 0 ) );
      test.execute( ExpectAckno { Wrap32 { 1 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
    }

    {
      TCPReceiverTestHarness test { "connect 2", 5435 };
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( 89347598 ) );
      test.execute( ExpectAckno { Wrap32 { 89347599 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
    }

    {
      TCPReceiverTestHarness test { "connect 3", 5435 };
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_seqno( 893475 ).without_ackno() );
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
    }

    {
      TCPReceiverTestHarness test { "connect 4", 5435 };
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_fin().with_seqno( 893475 ).without_ackno() );
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
    }

    {
      TCPReceiverTestHarness test { "connect 5", 5435 };
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_fin().with_seqno( 893475 ).without_ackno() );
      test.execute( ExpectAckno { std::optional<Wrap32> {} } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
      test.execute( SegmentArrives {}.with_syn().with_seqno( 89347598 ) );
      test.execute( ExpectAckno { Wrap32 { 89347599 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
    }

    {
      TCPReceiverTestHarness test { "connect 6", 4000 };
      test.execute( SegmentArrives {}.with_syn().with_seqno( 5 ).with_fin() );
      test.execute( IsClosed { true } );
      test.execute( ExpectAckno { Wrap32 { 7 } } );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed { 0 } );
    }

    {
      TCPReceiverTestHarness test { "window size zero", 0 };
      test.execute( ExpectWindow { 0 } );
    }

    {
      TCPReceiverTestHarness test { "window size 50", 50 };
      test.execute( ExpectWindow { 50 } );
    }

    {
      TCPReceiverTestHarness test { "window size at max", UINT16_MAX };
      test.execute( ExpectWindow { UINT16_MAX } );
    }

    {
      TCPReceiverTestHarness test { "window size at max+1", UINT16_MAX + 1 };
      test.execute( ExpectWindow { UINT16_MAX } );
    }

    {
      TCPReceiverTestHarness test { "window size at max+5", UINT16_MAX + 5 };
      test.execute( ExpectWindow { UINT16_MAX } );
    }

    {
      TCPReceiverTestHarness test { "window size at 10M", 10'000'000 };
      test.execute( ExpectWindow { UINT16_MAX } );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
