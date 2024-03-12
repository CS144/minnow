#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return this->write_have_been_closed;
}

void Writer::push( string data )
{
  uint64_t data_length =data.size();
  if(!this->is_closed() && data_length<=this->capacity_){
    this->stream_data.append(data);
    this->capacity_-=data_length;
    this->total_bytes_pushed+=data_length;
  }else if (!this->is_closed() && data_length>this->capacity_){
    data=data.substr(0,this->capacity_);
    this->stream_data.append(data);
    this->total_bytes_pushed+=this->capacity_;
    this->capacity_=0;
  }
  return;
}

void Writer::close()
{
    this->write_have_been_closed=true;
}

uint64_t Writer::available_capacity() const
{
  return this->capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  return this->total_bytes_pushed;
}

bool Reader::is_finished() const
{
  return this->write_have_been_closed && !this->stream_data.size();
}

uint64_t Reader::bytes_popped() const
{
  return this->total_bytes_poped;
}

string_view Reader::peek() const
{
  return this->stream_data;
}

void Reader::pop( uint64_t len )
{
  this->stream_data=this->stream_data.substr(len);
  this->total_bytes_poped+=len;
  this->capacity_+=len;
}

uint64_t Reader::bytes_buffered() const
{
  return this->total_bytes_pushed-this->total_bytes_poped;
}
