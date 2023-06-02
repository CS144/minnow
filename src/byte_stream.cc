#include "byte_stream.hh"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <version>
using namespace std;
ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), data_queue_ {}, data_view_ {} {}
void Writer::push( string data )
{
  if ( has_error_ ) {
    throw runtime_error( "The stream has error. Push failed.\n" );
  }
  if ( is_closed_ ) {
    return;
  }
  if ( data.empty() || available_capacity() == 0 ) {
    return;
  }
  auto const topush=min(available_capacity(),data.length());
  if(topush<data.length()){
    data=data.substr(0,topush);
  }
  //const string s;
  data_queue_.push_back(std::move(data));
  data_view_.emplace_back(data_queue_.back().data(),topush);
  bytes_pushed_+=topush;
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
  if(data_view_.empty()) { 
    return {};
  }
  return data_view_.front();
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
  if(len==0||data_view_.empty()){
    return;
  }
  auto topop=min(len,bytes_buffered());
  //auto n = min( len, num_bytes_buffered_ );
  while ( topop > 0 ) {
    auto sz = data_view_.front().length();
    if ( topop < sz ) {
      data_view_.front().remove_prefix( topop );
      //num_bytes_buffered_ -= n;
      bytes_popped_ += topop;
      return;
    }
    data_view_.pop_front();
    data_queue_.pop_front();
    topop -= sz;
    //num_bytes_buffered_ -= sz;
    bytes_popped_ += sz;
  }

// 作者：haha
// 链接：https://zhuanlan.zhihu.com/p/630739394
// 来源：知乎
// 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
}
uint64_t Reader::bytes_buffered() const
{
  return bytes_pushed_ - bytes_popped_;
}
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
