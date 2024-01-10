#include "common.hh"

#include <iomanip>
#include <iostream>
#include <unistd.h>

using namespace std;

void Printer::diagnostic( std::string_view test_name,
                          const vector<pair<string, int>>& steps_executed,
                          const string& failing_step,
                          const exception& e ) const
{
  const string quote = Printer::with_color( Printer::def, "\"" );
  cerr << "\nThe test " << quote << Printer::with_color( Printer::def, test_name ) << quote
       << " failed after these steps:\n\n";
  unsigned int step_num = 0;
  for ( const auto& [str, col] : steps_executed ) {
    cerr << "  " << step_num++ << "."
         << "\t" << with_color( col, str ) << "\n";
  }
  cerr << with_color( red, "  ***** Unsuccessful " + failing_step + " *****\n\n" );

  cerr << with_color( red, demangle( typeid( e ).name() ) ) << ": " << with_color( def, e.what() ) << "\n\n";
}

string Printer::prettify( string_view str, size_t max_length )
{
  ostringstream ss;
  const string_view str_prefix = str.substr( 0, max_length );
  for ( const uint8_t ch : str_prefix ) {
    if ( isprint( ch ) ) {
      ss << ch;
    } else {
      ss << "\\x" << fixed << setw( 2 ) << setfill( '0' ) << hex << static_cast<size_t>( ch );
    }
  }
  if ( str.size() > str_prefix.size() ) {
    ss << "...";
  }
  return ss.str();
}

Printer::Printer() : is_terminal_( isatty( STDERR_FILENO ) or getenv( "MAKE_TERMOUT" ) ) {}

string Printer::with_color( int color_value, string_view str ) const
{
  string ret;
  if ( is_terminal_ ) {
    ret += "\033[1;" + to_string( color_value ) + "m";
  }

  ret += str;

  if ( is_terminal_ ) {
    ret += "\033[m";
  }

  return ret;
}
