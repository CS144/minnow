#include "reassembler_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    {
      ReassemblerTestHarness test { "seq 1", 65000 };

      test.execute( Insert { "abcd", 0 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "abcd" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "efgh", 4 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( ReadAll( "efgh" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "seq 2", 65000 };

      test.execute( Insert { "abcd", 0 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( IsFinished { false } );
      test.execute( Insert { "efgh", 4 } );
      test.execute( BytesPushed( 8 ) );

      test.execute( ReadAll( "abcdefgh" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "seq 3", 65000 };
      std::ostringstream ss;

      for ( size_t i = 0; i < 100; ++i ) {
        test.execute( BytesPushed( 4 * i ) );
        test.execute( Insert { "abcd", 4 * i } );
        test.execute( IsFinished { false } );

        ss << "abcd";
      }

      test.execute( ReadAll( ss.str() ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "seq 4", 65000 };
      for ( size_t i = 0; i < 100; ++i ) {
        test.execute( BytesPushed( 4 * i ) );
        test.execute( Insert { "abcd", 4 * i } );
        test.execute( IsFinished { false } );

        test.execute( ReadAll( "abcd" ) );
      }
    }

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
