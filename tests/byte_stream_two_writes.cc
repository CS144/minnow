#include "byte_stream.hh"
#include "byte_stream_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    {
      ByteStreamTestHarness test { "write-write-end-pop-pop", 15 };

      test.execute( Push { "cat" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 12 } );
      test.execute( BytesBuffered { 3 } );
      test.execute( Peek { "cat" } );

      test.execute( Push { "tac" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 6 } );
      test.execute( AvailableCapacity { 9 } );
      test.execute( BytesBuffered { 6 } );
      test.execute( Peek { "cattac" } );

      test.execute( Close {} );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 6 } );
      test.execute( AvailableCapacity { 9 } );
      test.execute( BytesBuffered { 6 } );
      test.execute( Peek { "cattac" } );

      test.execute( Pop { 2 } );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 2 } );
      test.execute( BytesPushed { 6 } );
      test.execute( AvailableCapacity { 11 } );
      test.execute( BytesBuffered { 4 } );
      test.execute( Peek { "ttac" } );

      test.execute( Pop { 4 } );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { true } );
      test.execute( IsFinished { true } );
      test.execute( BytesPopped { 6 } );
      test.execute( BytesPushed { 6 } );
      test.execute( AvailableCapacity { 15 } );
      test.execute( BytesBuffered { 0 } );
    }

    {
      ByteStreamTestHarness test { "write-pop-write-end-pop", 15 };

      test.execute( Push { "cat" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 12 } );
      test.execute( BytesBuffered { 3 } );
      test.execute( Peek { "cat" } );

      test.execute( Pop { 2 } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 2 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 14 } );
      test.execute( BytesBuffered { 1 } );
      test.execute( Peek { "t" } );

      test.execute( Push { "tac" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 2 } );
      test.execute( BytesPushed { 6 } );
      test.execute( AvailableCapacity { 11 } );
      test.execute( BytesBuffered { 4 } );
      test.execute( Peek { "ttac" } );

      test.execute( Close {} );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 2 } );
      test.execute( BytesPushed { 6 } );
      test.execute( AvailableCapacity { 11 } );
      test.execute( BytesBuffered { 4 } );
      test.execute( Peek { "ttac" } );

      test.execute( Pop { 4 } );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { true } );
      test.execute( IsFinished { true } );
      test.execute( BytesPopped { 6 } );
      test.execute( BytesPushed { 6 } );
      test.execute( AvailableCapacity { 15 } );
      test.execute( BytesBuffered { 0 } );
    }

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
