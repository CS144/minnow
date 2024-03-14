#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
if ( is_last_substring ) {
    lastpos = first_index + data.size();
  }
  // exceed window size
  if ( first_index >= need + output_.writer().available_capacity() || first_index + data.size() < need ) {
    return;
  }
  // limit left
  if ( first_index < need ) {
    data = data.substr( need - first_index );
    first_index = need;
  }
  // limit right
  data = data.substr( 0, min( need + output_.writer().available_capacity() - first_index, (uint64_t)data.size() ) );
  //  put unique byte into vector
  // while ( s.size() < first_index + data.size() ) {
  //   s.emplace_back( val );
  // }
  for(int i=s.size(),j=data.size()+first_index;i<j;i++){
    s.emplace_back(val);
  }
  for ( auto& ch : data ) {
    if ( s[first_index] == val ) {
      s[first_index] = ch;
      ++store;
    }
    ++first_index;
  }
  string buffer;
  while ( need < s.size() && s[need] != val ) {
    buffer += (char)s[need++];
    --store;
  }
  if ( !buffer.empty() ) {
    output_.writer().push( buffer );
  }

  if ( need >= lastpos ) {
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return store;
}
