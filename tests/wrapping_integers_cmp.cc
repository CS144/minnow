#include "random.hh"
#include "test_should_be.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

int main()
{
  try {
    // Comparing low-number adjacent seqnos
    test_should_be( Wrap32( 3 ) != Wrap32( 1 ), true );
    test_should_be( Wrap32( 3 ) == Wrap32( 1 ), false );

    constexpr size_t N_REPS = 32768;

    auto rd = get_random_engine();

    for ( size_t i = 0; i < N_REPS; i++ ) {
      const uint32_t n = rd();
      const uint8_t diff = rd();
      const uint32_t m = n + diff;
      test_should_be( Wrap32( n ) == Wrap32( m ), n == m );
      test_should_be( Wrap32( n ) != Wrap32( m ), n != m );
    }

  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
