#include "byte_stream.hh"
#include "byte_stream_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

void all_zeroes( ByteStreamTestHarness& test )
{
  test.execute( BytesBuffered { 0 } );
  test.execute( AvailableCapacity { 15 } );
  test.execute( BytesPushed { 0 } );
  test.execute( BytesPopped { 0 } );
}

int main()
{
  try {
    {
      ByteStreamTestHarness test { "construction", 15 };
      test.execute( IsClosed { false } );
      test.execute( IsFinished { false } );
      test.execute( HasError { false } );
      all_zeroes( test );
    }

    {
      ByteStreamTestHarness test { "close", 15 };
      test.execute( Close {} );
      test.execute( IsClosed { true } );
      test.execute( IsFinished { true } );
      test.execute( HasError { false } );
      all_zeroes( test );
    }

    {
      ByteStreamTestHarness test { "set-error", 15 };
      test.execute( SetError {} );
      test.execute( IsClosed { false } );
      test.execute( IsFinished { false } );
      test.execute( HasError { true } );
      all_zeroes( test );
    }

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
