#include "byte_stream.hh"
#include "byte_stream_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    {
      ByteStreamTestHarness test { "write-end-pop", 15 };

      test.execute( Push { "cat" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 12 } );
      test.execute( BytesBuffered { 3 } );
      test.execute( Peek { "cat" } );

      test.execute( Close {} );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 12 } );
      test.execute( BytesBuffered { 3 } );
      test.execute( Peek { "cat" } );

      test.execute( Pop { 3 } );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { true } );
      test.execute( IsFinished { true } );
      test.execute( BytesPopped { 3 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 15 } );
      test.execute( BytesBuffered { 0 } );
    }

    {
      ByteStreamTestHarness test { "write-pop-end", 15 };

      test.execute( Push { "cat" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 12 } );
      test.execute( BytesBuffered { 3 } );
      test.execute( Peek { "cat" } );

      test.execute( Pop { 3 } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { true } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 3 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 15 } );
      test.execute( BytesBuffered { 0 } );

      test.execute( Close {} );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { true } );
      test.execute( IsFinished { true } );
      test.execute( BytesPopped { 3 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 15 } );
      test.execute( BytesBuffered { 0 } );
    }

    {
      ByteStreamTestHarness test { "write-pop2-end", 15 };

      test.execute( Push { "cat" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 12 } );
      test.execute( BytesBuffered { 3 } );
      test.execute( Peek { "cat" } );

      test.execute( Pop { 1 } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 1 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 13 } );
      test.execute( BytesBuffered { 2 } );
      test.execute( Peek { "at" } );

      test.execute( Pop { 2 } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { true } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 3 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 15 } );
      test.execute( BytesBuffered { 0 } );

      test.execute( Close {} );

      test.execute( IsClosed { true } );
      test.execute( BufferEmpty { true } );
      test.execute( IsFinished { true } );
      test.execute( BytesPopped { 3 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 15 } );
      test.execute( BytesBuffered { 0 } );
    }

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
