#pragma once

#include "wrapping_integers.hh"

#include <optional>
#include <string>
#include <utility>

// https://stackoverflow.com/questions/33399594/making-a-user-defined-class-stdto-stringable

namespace minnow_conversions {
using std::to_string;

class DebugWrap32 : public Wrap32
{
public:
  uint32_t debug_get_raw_value() { return raw_value_; }
};

inline std::string to_string( Wrap32 i )
{
  return "Wrap32<" + std::to_string( DebugWrap32 { i }.debug_get_raw_value() ) + ">";
}

template<typename T>
std::string to_string( const std::optional<T>& v )
{
  if ( v.has_value() ) {
    return "Some(" + to_string( v.value() ) + ")";
  }

  return "None";
}
} // namespace minnow_conversions

template<typename T>
concept MinnowStringable = requires( T t ) { minnow_conversions::to_string( t ); };

template<MinnowStringable T>
std::string to_string( T&& t )
{
  return minnow_conversions::to_string( std::forward<T>( t ) );
}

inline std::ostream& operator<<( std::ostream& os, Wrap32 a )
{
  return os << to_string( a );
}

inline bool operator!=( Wrap32 a, Wrap32 b )
{
  return not( a == b );
}

inline int64_t operator-( Wrap32 a, Wrap32 b )
{
  return static_cast<int64_t>( minnow_conversions::DebugWrap32 { a }.debug_get_raw_value() )
         - static_cast<int64_t>( minnow_conversions::DebugWrap32 { b }.debug_get_raw_value() );
}
