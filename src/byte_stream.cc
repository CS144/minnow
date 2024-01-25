#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ),rcnt_(0),wcnt_(0),Q_(""),error_(false),EOF_(false){}

bool Writer::is_closed() const
{
  return EOF_;
}

void Writer::push( string data )
{
  if (EOF_||Q_.size()>=capacity_) return;
  uint64_t pushsize=min(data.size(),capacity_-Q_.size());
  wcnt_+=pushsize;
  Q_+=data.substr(0,pushsize);
  return;
}

void Writer::close()
{
  EOF_=true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_-Q_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return wcnt_;
}

bool Reader::is_finished() const
{
  return EOF_&&Q_.empty();
}

uint64_t Reader::bytes_popped() const
{
  return rcnt_;
}

string_view Reader::peek() const
{
  return std::string_view(Q_);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t popsize=min(Q_.size(),len);
  rcnt_+=popsize;
  Q_=Q_.substr(popsize);
}

uint64_t Reader::bytes_buffered() const
{
  return Q_.size();
}
