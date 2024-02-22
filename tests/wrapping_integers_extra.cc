#include "test_should_be.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>

using namespace std;

int main()
{
  try {
    for ( uint64_t checkpoint = 0; checkpoint < 100000; ++checkpoint ) {
      test_should_be( Wrap32::wrap( 0UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ), 0UL );
    }

    for ( uint64_t checkpoint = 0; checkpoint < 100000; ++checkpoint ) {
      test_should_be( Wrap32::wrap( 1UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ), 1UL );
    }

    for ( uint64_t checkpoint = UINT32_MAX - 100000UL; checkpoint < UINT32_MAX + 100000UL; ++checkpoint ) {
      test_should_be( Wrap32::wrap( UINT32_MAX - 1UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      UINT32_MAX - 1UL );
    }

    for ( uint64_t checkpoint = UINT32_MAX - 100000UL; checkpoint < UINT32_MAX + 100000UL; ++checkpoint ) {
      test_should_be( Wrap32::wrap( UINT32_MAX, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      uint64_t { UINT32_MAX } );
    }

    for ( uint64_t checkpoint = UINT32_MAX - 100000UL; checkpoint < UINT32_MAX + 100000UL; ++checkpoint ) {
      test_should_be( Wrap32::wrap( UINT32_MAX + 1UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      UINT32_MAX + 1UL );
    }

    for ( uint64_t checkpoint = UINT32_MAX - 100000UL; checkpoint < UINT32_MAX + 100000UL; ++checkpoint ) {
      test_should_be( Wrap32::wrap( UINT32_MAX + 2UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      UINT32_MAX + 2UL );
    }

    for ( uint64_t checkpoint = 2UL * UINT32_MAX - 100000UL; checkpoint < 2UL * UINT32_MAX + 100000UL;
          ++checkpoint ) {
      test_should_be( Wrap32::wrap( 2UL * UINT32_MAX - 1UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      2UL * UINT32_MAX - 1UL );
    }

    for ( uint64_t checkpoint = 2UL * UINT32_MAX - 100000UL; checkpoint < 2UL * UINT32_MAX + 100000UL;
          ++checkpoint ) {
      test_should_be( Wrap32::wrap( 2UL * UINT32_MAX, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      2UL * UINT32_MAX );
    }

    for ( uint64_t checkpoint = 2UL * UINT32_MAX - 100000UL; checkpoint < 2UL * UINT32_MAX + 100000UL;
          ++checkpoint ) {
      test_should_be( Wrap32::wrap( 2UL * UINT32_MAX + 1UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      2UL * UINT32_MAX + 1UL );
    }

    for ( uint64_t checkpoint = 2UL * UINT32_MAX - 100000UL; checkpoint < 2UL * UINT32_MAX + 100000UL;
          ++checkpoint ) {
      test_should_be( Wrap32::wrap( 2UL * UINT32_MAX + 2UL, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, checkpoint ),
                      2UL * UINT32_MAX + 2UL );
    }

    for ( int64_t i = -100000; i < 100000; ++i ) {
      test_should_be( Wrap32::wrap( UINT32_MAX + i, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, UINT32_MAX ),
                      uint64_t( UINT32_MAX + i ) );
    }

    for ( int64_t i = -100000; i < 100000; ++i ) {
      test_should_be( Wrap32::wrap( 2UL * UINT32_MAX + i, Wrap32 { 19 } ).unwrap( Wrap32 { 19 }, 2UL * UINT32_MAX ),
                      2UL * UINT32_MAX + i );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
