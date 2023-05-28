#include "byte_stream.hh"
#include <cstdint>
#include <stdexcept>
#include <string_view>
using namespace std;
ByteStream::ByteStream( uint64_t capacity ) 
:capacity_(capacity),buf_(0),close_(false),error_(false),bytes_popped_(0),bytes_pushed_(0){
}
void Writer::push( string data )
{
  if(error_){
    throw runtime_error("The stream has error. Push failed.\n");
  }
  if(close_){
   // throw runtime_error("The stream is closed. Push failed.\n");
   return;
  }
  if(available_capacity()>=data.size()){
    for(const auto& d:data){
      buf_.push_back(d);
    }
    bytes_pushed_+=data.size();
  }
  else{//cap<data.size
    for(auto it=data.begin();it!=data.begin()+static_cast<int64_t>(available_capacity());++it){
      buf_.push_back(*it);
    }
    bytes_pushed_+=available_capacity();
  }
}
void Writer::close()
{
  close_=true; 
}
void Writer::set_error()
{
  error_=true;
}
bool Writer::is_closed() const
{
  return close_;
}
uint64_t Writer::available_capacity() const
{
  return capacity_-(bytes_pushed_-bytes_popped_);
}
uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}
string_view Reader::peek() const
{
  static string ss;
  ss.clear();
  for(auto it=buf_.begin();it!=buf_.begin()+static_cast<int64_t>(bytes_buffered());++it){
    ss+=*it;
  }
  return ss;
}
bool Reader::is_finished() const
{
  return close_&&!bytes_buffered();
}
bool Reader::has_error() const
{
  return error_;
}
void Reader::pop( uint64_t len )
{
  if(has_error()){
    throw runtime_error("The stream has error. Read failed.\n");
  }
  if(is_finished()){
    //throw runtime_error("The stream is finished. Read failed.\n");
    return;
  }
  if(bytes_buffered()>=len){
    for(uint64_t i=0;i<len;++i){
      buf_.pop_front();
    }
    bytes_popped_+=len;
  }
  else{//buf<len
    while(!buf_.empty()) { 
      buf_.pop_front();
    }
    bytes_popped_+=bytes_buffered();
  }
}
 uint64_t Reader::bytes_buffered() const
{
  return bytes_pushed_-bytes_popped_;
}
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
