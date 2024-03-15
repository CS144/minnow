#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point+n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{ 
  Wrap32 temp=wrap(checkpoint,zero_point);
  uint64_t ret_right,ret_left=0;
  if ( checkpoint < raw_value_ - zero_point.raw_value_ ) {
    return raw_value_ - zero_point.raw_value_;
  }
  if(raw_value_<temp.raw_value_){//checkpoint 大于
    ret_right = checkpoint+ (1UL << 32)-temp.raw_value_+raw_value_;
    ret_left= checkpoint+raw_value_-temp.raw_value_;
  }else{
    ret_left = checkpoint - (1UL << 32) +(raw_value_-temp.raw_value_);
    ret_right= checkpoint+raw_value_-temp.raw_value_;
  }
  return ret_right-checkpoint<checkpoint-ret_left?ret_right:ret_left;




  // uint64_t res = raw_value_ - zero_point.raw_value_;
  // if ( checkpoint < res ) {
  //   return res;
  // }
  // uint64_t l = ( ( ( checkpoint - res ) >> 32 ) << 32 ) + res;
  // uint64_t r = ( ( ( ( checkpoint - res ) >> 32 ) + 1 ) << 32 ) + res;
  // uint64_t ans = checkpoint - l < r - checkpoint ? l : r;
  // return { ans };
}