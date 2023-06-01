#include "byte_stream.hh"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
using namespace std;
ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buf_() {}
void Writer::push( string data )
{
  if ( has_error_ ) {
    throw runtime_error( "The stream has error. Push failed.\n" );
  }
  if ( is_closed_ ) {
    return;
  }
  auto size = min( data.length(), capacity_ - buf_.size() );
  data = data.substr( 0, size );
  for ( const auto& c : data ) {
    buf_.push_back( c );
  }
  bytes_pushed_ += size;
}

void Writer::close()
{
  is_closed_ = true;
}
void Writer::set_error()
{
  has_error_ = true;
}
bool Writer::is_closed() const
{
  return is_closed_;
}
uint64_t Writer::available_capacity() const
{
  return capacity_ - ( bytes_pushed_ - bytes_popped_ );
}
uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}
string_view Reader::peek() const
{
  // string t{"z"};
  return string_view { &buf_.front(), 1ul };
}
bool Reader::is_finished() const
{
  return is_closed_ && bytes_pushed_ == bytes_popped_;
}
bool Reader::has_error() const
{
  return has_error_;
}
void Reader::pop( uint64_t len )
{
  if ( has_error() ) {
    throw runtime_error( "The stream has error. Read failed.\n" );
  }
  if ( is_finished() ) {
    return;
  }
  auto size = min( len, buf_.size() );
  buf_.erase( buf_.begin(), buf_.begin() + size );
  bytes_popped_ += size;
}
uint64_t Reader::bytes_buffered() const
{
  return bytes_pushed_ - bytes_popped_;
}
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
