#include "random.hh"
#include "reassembler_test_harness.hh"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <tuple>
#include <vector>

using namespace std;

static constexpr size_t NREPS = 32;
static constexpr size_t NSEGS = 128;
static constexpr size_t MAX_SEG_LEN = 2048;

int main()
{
  try {
    auto rd = get_random_engine();

    // overlapping segments
    for ( unsigned rep_no = 0; rep_no < NREPS; ++rep_no ) {
      ReassemblerTestHarness sr { "win test " + to_string( rep_no ), NSEGS * MAX_SEG_LEN };

      vector<tuple<size_t, size_t>> seq_size;
      size_t offset = 0;
      for ( unsigned i = 0; i < NSEGS; ++i ) {
        const size_t size = 1 + ( rd() % ( MAX_SEG_LEN - 1 ) );
        const size_t offs = min( offset, 1 + ( static_cast<size_t>( rd() ) % 1023 ) );
        seq_size.emplace_back( offset - offs, size + offs );
        offset += size;
      }
      shuffle( seq_size.begin(), seq_size.end(), rd );

      string d( offset, 0 );
      generate( d.begin(), d.end(), [&] { return rd(); } );

      for ( auto [off, sz] : seq_size ) {
        sr.execute( Insert { d.substr( off, sz ), off }.is_last( off + sz == offset ) );
      }

      sr.execute( ReadAll { d } );
    }
  } catch ( const exception& e ) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
