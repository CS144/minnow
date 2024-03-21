#include "reassembler_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    {
      ReassemblerTestHarness test { "holes 1", 65000 };

      test.execute( Insert { "b", 1 } );

      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "holes 2", 65000 };

      test.execute( Insert { "b", 1 } );
      test.execute( Insert { "a", 0 } );

      test.execute( BytesPushed( 2 ) );
      test.execute( ReadAll( "ab" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "holes 3", 65000 };

      test.execute( Insert { "b", 1 }.is_last() );

      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "a", 0 } );

      test.execute( BytesPushed( 2 ) );
      test.execute( ReadAll( "ab" ) );
      test.execute( IsFinished { true } );
    }

    {
      ReassemblerTestHarness test { "holes 4", 65000 };

      test.execute( Insert { "b", 1 } );
      test.execute( Insert { "ab", 0 } );

      test.execute( BytesPushed( 2 ) );
      test.execute( ReadAll( "ab" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "holes 5", 65000 };

      test.execute( Insert { "b", 1 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "d", 3 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "c", 2 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "a", 0 } );

      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "abcd" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "holes 6", 65000 };

      test.execute( Insert { "b", 1 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "d", 3 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "abc", 0 } );

      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "abcd" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "holes 7", 65000 };

      test.execute( Insert { "b", 1 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "d", 3 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "a", 0 } );
      test.execute( BytesPushed( 2 ) );
      test.execute( ReadAll( "ab" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "c", 2 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "cd" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "", 4 }.is_last() );
      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { true } );
    }
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
