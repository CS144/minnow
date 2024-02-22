#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { static_cast<uint32_t>( zero_point.raw_value_ + n ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t temp = raw_value_ - zero_point.raw_value_ - static_cast<uint32_t>( checkpoint );
  if ( temp <= ( 1U << 31 ) )
    return checkpoint + temp;
  else if ( checkpoint - ( 1ul << 32 ) + temp > checkpoint )
    return checkpoint + temp;
  else
    return checkpoint - ( 1ul << 32 ) + temp;
}
