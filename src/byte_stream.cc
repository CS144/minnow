#include <stdexcept>

#include "byte_stream.hh"


using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ),_is_closed(false),_error(false),_bytestream(),_bytes_pushed(0),_bytes_poped(0) {}

void Writer::push( string data )
{
  // Your code here.
  if (_is_closed || _error){
    return;
  }
  uint64_t remain = available_capacity();
  uint64_t to_write;

  if (remain > data.length()){
    to_write = data.length();
  }else to_write=remain;

  for (uint64_t i=0; i <to_write;i++){
    _bytestream.push_back(data[i]);
    _bytes_pushed++;
  }
}

void Writer::close()
{
  // Your code here.
  _is_closed=true;
  return;
}

void Writer::set_error()
{
  // Your code here.
  _error=true;
  return;
}

bool Writer::is_closed() const
{
  // Your code here.
  return _is_closed;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - _bytestream.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return _bytes_pushed;
}

string_view Reader::peek() const
{
  // Your code here.
  
  string_view strView(&_bytestream.front(),1);
  return strView;
}

bool Reader::is_finished() const
{
  // Your code here.
  if (_is_closed && _bytestream.size()==0){
    return true;
  }else return false;
}

bool Reader::has_error() const
{
  // Your code here.
  if (_error){
    return true;
  }else{
    return false;
  }
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  for (uint64_t i=0; i<len; i++){
    _bytestream.pop_front();
    _bytes_poped++;
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return _bytestream.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return _bytes_poped;
}
