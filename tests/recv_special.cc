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

    // credit: Mustafa Bayramov
    {
      const size_t cap = 10;
      const uint32_t isn = 23452;
      TCPReceiverTestHarness test { "buffer full, keep pushing", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
      test.execute( ExpectWindow { cap } );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "abcde" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 6 } } );
      test.execute( ExpectWindow { cap - 5 } );
      test.execute( BytesPushed { 5 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 6 ).with_data( "fghij" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 11 } } );
      test.execute( ExpectWindow { 0 } );
      test.execute( BytesPushed { 10 } );
      // buffer we keep pushing
      test.execute( SegmentArrives {}.with_seqno( isn + 11 ).with_data( "klmno" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 11 } } );
      test.execute( ExpectWindow { 0 } );
      test.execute( BytesPushed { 10 } );
      test.execute( SegmentArrives {}.with_seqno( isn + 16 ).with_data( "pqrst" ) );
      test.execute( ExpectAckno { Wrap32 { isn + 11 } } );
      test.execute( ExpectWindow { 0 } );
      test.execute( BytesPushed { 10 } );
      test.execute( ReadAll { "abcdefghij" } );
    }

    {
      const size_t cap = 10;
      const uint32_t isn = 12345;
      TCPReceiverTestHarness test { "missing first byte in the first segment", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( "" ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 2 ).with_data( "a" ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 4 ).with_data( "b" ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 6 ).with_data( "c" ) );
      test.execute( SegmentArrives {}.with_seqno( isn + 8 ).with_data( "d" ) );
      test.execute( BytesPushed { 0 } );
      test.execute( ExpectWindow { 10 } );
      test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
    }

    {
      const size_t cap = 10;
      const uint32_t isn = 123456;
      TCPReceiverTestHarness test { "pushing bytes in reverse order in initial SYN", cap };
      test.execute( SegmentArrives {}.with_syn().with_seqno( isn ) );
      string bytes = { 'j', 'i', 'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a' };
      for ( int i = cap - 1; i >= 1; --i ) {
        test.execute(
          SegmentArrives {}.with_seqno( isn + i + 1 ).with_data( std::string( 1, bytes[cap - i - 1] ) ) );
        test.execute( ExpectAckno { Wrap32 { isn + 1 } } );
        test.execute( BytesPushed { 0 } );
        test.execute( ExpectWindow { 10 } );
      }
      test.execute( SegmentArrives {}.with_seqno( isn + 1 ).with_data( std::string( 1, bytes[cap - 1] ) ) );
      test.execute( BytesPushed { bytes.size() } );
      test.execute( ExpectWindow { 0 } );
      test.execute( ExpectAckno { Wrap32 { static_cast<uint32_t>( isn + bytes.size() + 1 ) } } );
      test.execute( ReadAll { "abcdefghij" } );
    }

    {
      TCPReceiverTestHarness test { "Stream error -> RST flag", 10 };
      test.execute( SetError {} );
      test.execute( ExpectReset { true } );
    }

    {
      TCPReceiverTestHarness test { "No stream error -> no RST flag", 10 };
      test.execute( ExpectReset { false } );
    }

    {
      // The spec is more restrictive on RST acceptance; we use simpler logic in CS144.
      TCPReceiverTestHarness test { "RST flag set -> stream error", 10 };
      test.execute( SegmentArrives {}.with_seqno( rd() ).with_rst().without_ackno() );
      test.execute( HasError { true } );
    }

    {
      // test credit: Majd Nasra
      ReassemblerTestHarness test { "segment already seen in full", 3 };

      test.execute( Insert { "abc", 0 } );
      test.execute( ReadAll( "abc" ) );
      test.execute( BytesPushed( 3 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( Insert { "ab", 0 } );
      test.execute( ReadAll( "" ) ); // tests if the code double-pushes segments already seen in the stream.
      test.execute( BytesPushed( 3 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      // Credit: Max Jardetzky
      ReassemblerTestHarness test { "insert within capacity", 4 };

      test.execute( Insert { "bc", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 2 ) );

      test.execute( Insert { "bcd", 1 } );
      test.execute( BytesPending( 3 ) );
      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "abcd" ) );
      test.execute( BytesPushed( 4 ) );
      test.execute( BytesPending( 0 ) );
    }

    // test credit: Tanmay Garg and Agam Mohan Singh Bhatia
    {
      ReassemblerTestHarness test { "insert last fully beyond capacity + empty string is last", 2 };

      test.execute( Insert { "b", 1 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 1 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( Insert { "c", 2 }.is_last() );
      test.execute( IsFinished { false } );
      test.execute( Insert { "abc", 0 }.is_last() );
      test.execute( IsFinished { false } );
      test.execute( Insert { "", 3 }.is_last() );
      test.execute( IsFinished { false } );

      test.execute( ReadAll( "ab" ) );
      test.execute( Insert { "c", 2 }.is_last() );
      test.execute( ReadAll( "c" ) );
      test.execute( IsFinished { true } );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
