#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return write_have_been_closed;
}

void Writer::push( string data )
{
  uint64_t push_length = min(capacity_,data.size());
  if(!is_closed()) {
    data = data.substr( 0, push_length );
    stream_data.append( data );
    capacity_ -= push_length;
    total_bytes_pushed += push_length;
  }
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
  return write_have_been_closed && total_bytes_poped==total_bytes_pushed;
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
  stream_data.erase(0,len);
  total_bytes_poped+=len;
  capacity_+=len;
}

uint64_t Reader::bytes_buffered() const
{
  return total_bytes_pushed-total_bytes_poped;
}
