#pragma once

#include <optional>
#include <string>
#include <utility>

// https://stackoverflow.com/questions/33399594/making-a-user-defined-class-stdto-stringable

namespace minnow_conversions {
using std::to_string;

template<typename T>
std::string to_string( const std::optional<T>& v )
{
  if ( v.has_value() ) {
    return "Some(" + to_string( v.value() ) + ")";
  }

  return "None";
}

template<typename T>
std::string as_string( T&& t )
{
  return to_string( std::forward<T>( t ) );
}
} // namespace minnow_conversions

template<typename T>
std::string to_string( T&& t )
{
  return minnow_conversions::as_string( std::forward<T>( t ) );
}
