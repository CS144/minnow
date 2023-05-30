#pragma once

#include "conversions.hh"

#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage))
#define test_should_be( act, exp ) test_should_be_helper( act, exp, #act, #exp, __LINE__ )

template<typename T>
static void test_should_be_helper( const T& actual,
                                   const T& expected,
                                   const char* actual_s,
                                   const char* expected_s,
                                   const int lineno )
{
  if ( actual != expected ) {
    std::ostringstream ss;
    ss << "`" << actual_s << "` should have been `" << expected_s << "`, but the former is\n\t"
       << to_string( actual ) << "\nand the latter is\n\t" << to_string( expected ) << " (difference of "
       << static_cast<int64_t>( expected - actual ) << ")\n"
       << " (at line " << lineno << ")\n";
    throw std::runtime_error( ss.str() );
  }
}
