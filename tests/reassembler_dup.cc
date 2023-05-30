#include "random.hh"
#include "reassembler_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    auto rd = get_random_engine();

    {
      ReassemblerTestHarness test { "dup 1", 65000 };

      test.execute( Insert { "abcd", 0 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "abcd" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "abcd", 0 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "dup 2", 65000 };

      test.execute( Insert { "abcd", 0 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "abcd" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "abcd", 4 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( ReadAll( "abcd" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "abcd", 0 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "abcd", 4 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );
    }

    {
      ReassemblerTestHarness test { "dup 3", 65000 };

      test.execute( Insert { "abcdefgh", 0 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( ReadAll( "abcdefgh" ) );
      test.execute( IsFinished { false } );
      string data = "abcdefgh";

      for ( size_t i = 0; i < 1000; ++i ) {
        const size_t start_i = uniform_int_distribution<size_t> { 0, 8 }( rd );
        auto start = data.begin();
        std::advance( start, start_i );

        const size_t end_i = uniform_int_distribution<size_t> { start_i, 8 }( rd );
        auto end = data.begin();
        std::advance( end, end_i );

        test.execute( Insert { string { start, end }, start_i } );
        test.execute( BytesPushed( 8 ) );
        test.execute( ReadAll( "" ) );
        test.execute( IsFinished { false } );
      }
    }

    {
      ReassemblerTestHarness test { "dup 4", 65000 };

      test.execute( Insert { "abcd", 0 } );
      test.execute( BytesPushed( 4 ) );
      test.execute( ReadAll( "abcd" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "abcdef", 0 } );
      test.execute( BytesPushed( 6 ) );
      test.execute( ReadAll( "ef" ) );
      test.execute( IsFinished { false } );
    }

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
