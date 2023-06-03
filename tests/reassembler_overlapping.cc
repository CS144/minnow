#include "reassembler_test_harness.hh"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
  try {
    {
      // Overlapping assembled (unread) section
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "overlapping assembled/unread section", cap };

      test.execute( Insert { "a", 0 } );
      test.execute( Insert { "ab", 0 } );

      test.execute( BytesPushed( 2 ) );
      test.execute( ReadAll( "ab" ) );
    }

    {
      // Overlapping assembled (read) section
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "overlapping assembled/read section", cap };

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "a" ) );

      test.execute( Insert { "ab", 0 } );
      test.execute( ReadAll( "b" ) );
      test.execute( BytesPushed( 2 ) );
    }

    {
      // Overlapping unassembled section, resulting in assembly
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "overlapping unassembled section to fill hole", cap };

      test.execute( Insert { "b", 1 } );
      test.execute( ReadAll( "" ) );

      test.execute( Insert { "ab", 0 } );
      test.execute( ReadAll( "ab" ) );
      test.execute( BytesPending { 0 } );
      test.execute( BytesPushed( 2 ) );
    }
    {
      // Overlapping unassembled section, not resulting in assembly
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "overlapping unassembled section", cap };

      test.execute( Insert { "b", 1 } );
      test.execute( ReadAll( "" ) );

      test.execute( Insert { "bc", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPending { 2 } );
      test.execute( BytesPushed( 0 ) );
    }
    {
      // Overlapping unassembled section, not resulting in assembly
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "overlapping unassembled section 2", cap };

      test.execute( Insert { "c", 2 } );
      test.execute( ReadAll( "" ) );

      test.execute( Insert { "bcd", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPending { 3 } );
      test.execute( BytesPushed( 0 ) );
    }

    {
      // Overlapping multiple unassembled sections
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "overlapping multiple unassembled sections", cap };

      test.execute( Insert { "b", 1 } );
      test.execute( Insert { "d", 3 } );
      test.execute( ReadAll( "" ) );

      test.execute( Insert { "bcde", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 4 ) );
    }

    {
      // Submission over existing
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "insert over existing section", cap };

      test.execute( Insert { "c", 2 } );
      test.execute( Insert { "bcd", 1 } );

      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 3 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "abcd" ) );
      test.execute( BytesPushed( 4 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      // Submission within existing
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "insert within existing section", cap };

      test.execute( Insert { "bcd", 1 } );
      test.execute( Insert { "c", 2 } );

      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 3 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "abcd" ) );
      test.execute( BytesPushed( 4 ) );
      test.execute( BytesPending( 0 ) );
    }

  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
