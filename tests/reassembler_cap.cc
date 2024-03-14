#include "reassembler_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    {
      ReassemblerTestHarness test { "all within capacity", 2 };

      test.execute( Insert { "ab", 0 } );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( ReadAll( "ab" ) );

      test.execute( Insert { "cd", 2 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( ReadAll( "cd" ) );

      test.execute( Insert { "ef", 4 } );
      test.execute( BytesPushed( 6 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( ReadAll( "ef" ) );
    }

    {
      ReassemblerTestHarness test { "insert beyond capacity", 2 };

      test.execute( Insert { "ab", 0 } );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( Insert { "cd", 2 } );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( ReadAll( "ab" ) );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( Insert { "cd", 2 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( ReadAll( "cd" ) );
    }

    {
      ReassemblerTestHarness test { "overlapping inserts", 1 };

      test.execute( Insert { "ab", 0 } );
      test.execute( BytesPushed( 1 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( Insert { "ab", 0 } );
      test.execute( BytesPushed( 1 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( ReadAll( "a" ) );
      test.execute( BytesPushed( 1 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( Insert { "abc", 0 } );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( ReadAll( "b" ) );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      ReassemblerTestHarness test { "insert beyond capacity repeated with different data", 2 };

      test.execute( Insert { "b", 1 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 1 ) );

      test.execute( Insert { "bX", 2 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 1 ) );

      test.execute( Insert { "a", 0 } );

      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( ReadAll( "ab" ) );

      test.execute( Insert { "bc", 1 } );
      test.execute( BytesPushed( 3 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( ReadAll( "c" ) );
    }

    // test credit: Cooper de Nicola
    {
      ReassemblerTestHarness test { "insert last beyond capacity", 2 };

      test.execute( Insert { "bc", 1 }.is_last() );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 1 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( ReadAll( "ab" ) );

      test.execute( IsFinished { false } );

      test.execute( Insert { "bc", 1 }.is_last() );
      test.execute( BytesPushed( 3 ) );
      test.execute( BytesPending( 0 ) );

      test.execute( ReadAll( "c" ) );

      test.execute( IsFinished { true } );
    }
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
