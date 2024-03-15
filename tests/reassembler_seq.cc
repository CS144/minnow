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

    {
      ReassemblerTestHarness test { "zero-valued byte in substring", 16 };

      test.execute( Insert { { 0x30, 0x0d, 0x62, 0x00, 0x61, 0x00, 0x00 }, 9 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { { 0x0d, 0x0a, 0x63, 0x61, 0x0a, 0x66 }, 0 } );
      test.execute( BytesPushed( 6 ) );

      test.execute( Insert { { 0x0d, 0x0a, 0x63, 0x61, 0x0a, 0x66, 0x65, 0x20, 0x62, 0x30 }, 0 } );
      test.execute( BytesPushed( 16 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( ReadAll(
        { 0x0d, 0x0a, 0x63, 0x61, 0x0a, 0x66, 0x65, 0x20, 0x62, 0x30, 0x0d, 0x62, 0x00, 0x61, 0x00, 0x00 } ) );
    }
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
