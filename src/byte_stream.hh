#pragma once
#include <cstdint>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
class Reader;
class Writer;
class ByteStream
{
protected:
  uint64_t capacity_;
  // std::deque<std::string> data_queue_;
  // std::deque<std::string_view> data_view_;
  std::deque<char> buf_;
  bool is_closed_ = false;
  bool has_error_ = false;
  // uint64_t front_;
  // uint64_t back_;
  uint64_t bytes_popped_ = 0;
  uint64_t bytes_pushed_ = 0;

public:
  explicit ByteStream( uint64_t capacity );
  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;
};
class Writer : public ByteStream
{
public:
  // void push( const std::string& data ); // Push data to stream, but only as much as available capacity allows.
  void push( std::string data );
  void close();           // Signal that the stream has reached its ending. Nothing more will be written.
  void set_error();       // Signal that the stream suffered an error.
  bool is_closed() const; // Has the stream been closed?
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now?
  uint64_t bytes_pushed() const;       // Total number of bytes cumulatively pushed to the stream
};
class Reader : public ByteStream
{
public:
  std::string_view peek() const;   // Peek at the next bytes in the buffer
  void pop( uint64_t len );        // Remove `len` bytes from the buffer
  bool is_finished() const;        // Is the stream finished (closed and fully popped)?
  bool has_error() const;          // Has the stream had an error?
  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream
};
/*
 * read: A (provided) helper function thats peeks and pops up to `len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t len, std::string& out );
