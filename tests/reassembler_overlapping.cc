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

    {
      // Hole filled progressively and with overlap. Credit: Sarah McCarthy

      ReassemblerTestHarness test { "hole filled with overlap", 20 };

      test.execute( Insert { "fgh", 5 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( ReadAll( "" ) );
      test.execute( IsFinished { false } );

      test.execute( Insert { "abc", 0 } );
      test.execute( BytesPushed( 3 ) );

      test.execute( Insert { "abcdef", 0 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( BytesPending( 0 ) );
      test.execute( ReadAll( "abcdefgh" ) );
    }

    {
      // Multiple overlap. Credit: Sebastian Ingino
      ReassemblerTestHarness test { "multiple overlaps", 1000 };

      test.execute( Insert { "c", 2 } );
      test.execute( Insert { "e", 4 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 2 ) );

      test.execute( Insert { "bcdef", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 5 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "abcdef" ) );
      test.execute( BytesPushed( 6 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      // Overlap between two pending. Credit: Sebastian Ingino.
      ReassemblerTestHarness test { "overlap between two pending", 1000 };

      test.execute( Insert { "bc", 1 } );
      test.execute( Insert { "ef", 4 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 4 ) );

      test.execute( Insert { "cde", 2 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 5 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "abcdef" ) );
      test.execute( BytesPushed( 6 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      // Add exact copy. Credit: Sebastian Ingino.
      ReassemblerTestHarness test { "exact copy", 1000 };

      test.execute( Insert { "b", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 1 ) );

      test.execute( Insert { "b", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 1 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "ab" ) );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      // Credit: Anonymous (2023)
      ReassemblerTestHarness test { "yet another overlap test", 150 };

      test.execute( Insert { "efgh", 4 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 4 ) );

      test.execute( Insert { "op", 14 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 6 ) );

      test.execute( Insert { "s", 18 } );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 7 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( BytesPushed( 1 ) );
      test.execute( BytesPending( 7 ) );

      test.execute( Insert { "abcde", 0 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( BytesPending( 3 ) );

      test.execute( Insert { "opqrst", 14 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( BytesPending( 6 ) );

      test.execute( Insert { "op", 14 } );
      test.execute( BytesPushed( 8 ) );
      test.execute( BytesPending( 6 ) );

      test.execute( Insert { "ijklmn", 8 } );
      test.execute( BytesPushed( 20 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      // Credit: Eli Wald
      ReassemblerTestHarness test { "small capacity with overlapping insert", 2 };
      test.execute( Insert { "bc", 1 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 1 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "ab" ) );
      test.execute( BytesPushed( 2 ) );
      test.execute( BytesPending( 0 ) );
    }

    {
      // Credit: Chenhao Li
      const size_t cap = { 1000 };
      ReassemblerTestHarness test { "overlapping multiple unassembled sections 2", cap };

      test.execute( Insert { "bcd", 1 } );
      test.execute( Insert { "cde", 2 } );
      test.execute( ReadAll( "" ) );
      test.execute( BytesPushed( 0 ) );
      test.execute( BytesPending( 4 ) );

      test.execute( Insert { "a", 0 } );
      test.execute( ReadAll( "abcde" ) );
      test.execute( BytesPushed( 5 ) );
      test.execute( BytesPending( 0 ) );
    }
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
