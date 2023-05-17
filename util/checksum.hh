#pragma once

#include "buffer.hh"

#include <cstdint>
#include <string>
#include <vector>

//! The internet checksum algorithm
class InternetChecksum
{
private:
  uint32_t sum_;
  bool parity_ {};

public:
  explicit InternetChecksum( const uint32_t sum = 0 ) : sum_( sum ) {}
  void add( std::string_view data )
  {
    for ( const uint8_t i : data ) {
      uint16_t val = i;
      if ( not parity_ ) {
        val <<= 8;
      }
      sum_ += val;
      parity_ = !parity_;
    }
  }

  uint16_t value() const
  {
    uint32_t ret = sum_;

    while ( ret > 0xffff ) {
      ret = ( ret >> 16 ) + static_cast<uint16_t>( ret );
    }

    return ~ret;
  }

  void add( const std::vector<Buffer>& data )
  {
    for ( const auto& x : data ) {
      add( x );
    }
  }
};
