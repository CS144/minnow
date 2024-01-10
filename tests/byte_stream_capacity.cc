#include "byte_stream.hh"
#include "byte_stream_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    {
      ByteStreamTestHarness test { "overwrite", 2 };

      test.execute( Push { "cat" } );
      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 2 } );
      test.execute( AvailableCapacity { 0 } );
      test.execute( BytesBuffered { 2 } );
      test.execute( Peek { "ca" } );

      test.execute( Push { "t" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 0 } );
      test.execute( BytesPushed { 2 } );
      test.execute( AvailableCapacity { 0 } );
      test.execute( BytesBuffered { 2 } );
      test.execute( Peek { "ca" } );
    }

    {
      ByteStreamTestHarness test { "overwrite-clear-overwrite", 2 };

      test.execute( Push { "cat" } );
      test.execute( BytesPushed { 2 } );
      test.execute( Pop { 2 } );
      test.execute( Push { "tac" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 2 } );
      test.execute( BytesPushed { 4 } );
      test.execute( AvailableCapacity { 0 } );
      test.execute( BytesBuffered { 2 } );
      test.execute( Peek { "ta" } );
    }

    {
      ByteStreamTestHarness test { "overwrite-pop-overwrite", 2 };

      test.execute( Push { "cat" } );
      test.execute( BytesPushed { 2 } );
      test.execute( Pop { 1 } );
      test.execute( Push { "tac" } );

      test.execute( IsClosed { false } );
      test.execute( BufferEmpty { false } );
      test.execute( IsFinished { false } );
      test.execute( BytesPopped { 1 } );
      test.execute( BytesPushed { 3 } );
      test.execute( AvailableCapacity { 0 } );
      test.execute( BytesBuffered { 2 } );
      test.execute( Peek { "at" } );
    }

    {
      ByteStreamTestHarness test { "peeks", 2 };
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "cat" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Peek { "ca" } );
      test.execute( Peek { "ca" } );
      test.execute( BytesBuffered { 2 } );
      test.execute( Peek { "ca" } );
      test.execute( Peek { "ca" } );
      test.execute( Pop { 1 } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Push { "" } );
      test.execute( Peek { "a" } );
      test.execute( Peek { "a" } );
      test.execute( BytesBuffered { 1 } );
    }

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
