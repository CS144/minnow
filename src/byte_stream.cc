#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  (void)data;
}

void Writer::close()
{
  // Your code here.
}

void Writer::set_error()
{
  // Your code here.
}

bool Writer::is_closed() const
{
  // Your code here.
  return {};
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return {};
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {};
}

string_view Reader::peek() const
{
  // Your code here.
  return {};
}

bool Reader::is_finished() const
{
  // Your code here.
  return {};
}

bool Reader::has_error() const
{
  // Your code here.
  return {};
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return {};
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return {};
}
