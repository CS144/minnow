#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return write_have_been_closed;
}

void Writer::push( string data )
{
  uint64_t data_length =data.size();
  if(!is_closed() && data_length<=capacity_){
    stream_data.append(data);
    capacity_-=data_length;
    total_bytes_pushed+=data_length;
  }else if (!is_closed() && data_length>capacity_){
    data=data.substr(0,capacity_);
    stream_data.append(data);
    total_bytes_pushed+=capacity_;
    capacity_=0;
  }
  return;
}

void Writer::close()
{
    write_have_been_closed=true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  return total_bytes_pushed;
}

bool Reader::is_finished() const
{
  return write_have_been_closed && !stream_data.size();
}

uint64_t Reader::bytes_popped() const
{
  return total_bytes_poped;
}

string_view Reader::peek() const
{
  return stream_data;
}

void Reader::pop( uint64_t len )
{
  stream_data=stream_data.substr(len);
  total_bytes_poped+=len;
  capacity_+=len;
}

uint64_t Reader::bytes_buffered() const
{
  return total_bytes_pushed-total_bytes_poped;
}
