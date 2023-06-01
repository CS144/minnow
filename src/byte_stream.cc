#include "byte_stream.hh"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
using namespace std;
ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , data_queue_ {}
  , data_view_ {}
  , is_closed_( false )
  , has_error_( false )
  //, front_( 0 )
  //, back_( 0 )
  , bytes_popped_( 0 )
  , bytes_pushed_( 0 )
{}
void Writer::push( string data )
{
  if ( has_error_ ) {
    throw runtime_error( "The stream has error. Push failed.\n" );
  }
  if ( is_closed_ ) {
    return;
  }
  uint64_t topush = 0;
  if ( data.size() <= available_capacity() ) {
    topush = data.size();
  } else {
    topush = available_capacity();
  }
  data_queue_.push_back( data.substr( 0, topush ) );
  string_view now(data_queue_.back().data(),1);
  data_view_.push_back(now);
  bytes_pushed_ += topush;
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
  static string sssss="z\0";
  return string_view(&sssss[0],1ul);
}
bool Reader::is_finished() const
{
  return is_closed_ &&  bytes_pushed_ == bytes_popped_;
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
  if ( len <= bytes_buffered() ) {
    uint64_t cur = len;
    while ( data_view_.front().size() <= cur ) {
      cur -= data_view_.front().size();
      data_queue_.pop_front();
      data_view_.pop_front();
    } // cur<front.sz(maybe cur == 0)
    if ( cur == 0 ) {
      return;
    }
    // data_queue_.front();
    data_view_.front().remove_prefix( cur );
    bytes_popped_ += len;
  } else {
    data_view_.clear();
    data_queue_.clear();
    bytes_popped_ += bytes_buffered();
  }
}
uint64_t Reader::bytes_buffered() const
{
  return bytes_pushed_ - bytes_popped_;
}
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
