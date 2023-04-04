#pragma once

#include <cxxabi.h>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

class tagged_error : public std::system_error
{
private:
  std::string attempt_and_error_;
  int error_code_;

public:
  tagged_error( const std::error_category& category, const std::string_view s_attempt, const int error_code )
    : system_error( error_code, category )
    , attempt_and_error_( std::string( s_attempt ) + ": " + std::system_error::what() )
    , error_code_( error_code )
  {}

  const char* what() const noexcept override { return attempt_and_error_.c_str(); }

  int error_code() const { return error_code_; }
};

class unix_error : public tagged_error
{
public:
  explicit unix_error( const std::string_view s_attempt, const int s_errno = errno )
    : tagged_error( std::system_category(), s_attempt, s_errno )
  {}
};

inline int CheckSystemCall( const std::string_view s_attempt, const int return_value )
{
  if ( return_value >= 0 ) {
    return return_value;
  }

  throw unix_error { s_attempt };
}

template<typename T>
inline T* notnull( const std::string_view context, T* const x )
{
  return x ? x : throw std::runtime_error( std::string( context ) + ": returned null pointer" );
}

inline std::string demangle( const char* name )
{
  int status {};
  const std::unique_ptr<char, decltype( &free )> res { abi::__cxa_demangle( name, nullptr, nullptr, &status ),
                                                       free };
  if ( status ) {
    throw std::runtime_error( "cxa_demangle" );
  }
  return res.get();
}
