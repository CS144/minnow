#pragma once

#include "buffer.hh"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <deque>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class Serializer;

class Parser
{
  class BufferList
  {
    uint64_t size_ {};
    std::deque<Buffer> buffer_ {};
    uint64_t skip_ {};

  public:
    // NOLINTNEXTLINE(*-explicit-*)
    BufferList( const std::vector<Buffer>& buffers )
    {
      for ( const auto& x : buffers ) {
        append( x );
      }
    }

    uint64_t size() const { return size_; }
    uint64_t serialized_length() const { return size(); }
    bool empty() const { return size_ == 0; }

    std::string_view peek() const
    {
      if ( buffer_.empty() ) {
        throw std::runtime_error( "peek on empty BufferList" );
      }
      return std::string_view { buffer_.front() }.substr( skip_ );
    }

    void remove_prefix( uint64_t len )
    {
      while ( len and not buffer_.empty() ) {
        const uint64_t to_pop_now = std::min( len, peek().size() );
        skip_ += to_pop_now;
        len -= to_pop_now;
        size_ -= to_pop_now;
        if ( skip_ == buffer_.front().size() ) {
          buffer_.pop_front();
          skip_ = 0;
        }
      }
    }

    void dump_all( std::vector<Buffer>& out )
    {
      out.clear();
      if ( empty() ) {
        return;
      }
      std::string first_str = std::move( buffer_.front() );
      if ( skip_ ) {
        first_str = first_str.substr( skip_ );
      }
      out.emplace_back( std::move( first_str ) );
      buffer_.pop_front();
      for ( auto&& x : buffer_ ) {
        out.emplace_back( std::move( x ) );
      }
    }

    void dump_all( Buffer& out )
    {
      std::vector<Buffer> concat;
      dump_all( concat );
      if ( concat.size() == 1 ) {
        out = concat.front();
        return;
      }

      out.release().clear();
      for ( const auto& s : concat ) {
        out.release().append( s );
      }
    }

    void append( Buffer str )
    {
      size_ += str.size();
      buffer_.push_back( std::move( str ) );
    }
  };

  BufferList input_;
  bool error_ {};

  void check_size( const size_t size )
  {
    if ( size > input_.size() ) {
      error_ = true;
    }
  }

public:
  explicit Parser( const std::vector<Buffer>& input ) : input_( input ) {}

  const BufferList& input() const { return input_; }

  bool has_error() const { return error_; }
  void set_error() { error_ = true; }
  void remove_prefix( size_t n ) { input_.remove_prefix( n ); }

  template<std::unsigned_integral T>
  void integer( T& out )
  {
    check_size( sizeof( T ) );
    if ( has_error() ) {
      return;
    }

    out = static_cast<T>( 0 );
    for ( size_t i = 0; i < sizeof( T ); i++ ) {
      out <<= 8;
      out += static_cast<uint8_t>( input_.peek().front() );
      input_.remove_prefix( 1 );
    }
  }

  void string( std::span<char> out )
  {
    check_size( out.size() );
    if ( has_error() ) {
      return;
    }

    auto next = out.begin();
    while ( next != out.end() ) {
      const auto view = input_.peek().substr( 0, out.end() - next );
      next = std::copy( view.begin(), view.end(), next );
      input_.remove_prefix( view.size() );
    }
  }

  void all_remaining( std::vector<Buffer>& out ) { input_.dump_all( out ); }
  void all_remaining( Buffer& out ) { input_.dump_all( out ); }
};

class Serializer
{
  std::vector<Buffer> output_ {};
  std::string buffer_ {};

public:
  Serializer() = default;
  explicit Serializer( std::string&& buffer ) : buffer_( std::move( buffer ) ) {}

  template<std::unsigned_integral T>
  void integer( const T& val )
  {
    constexpr uint64_t len = sizeof( T );

    for ( uint64_t i = 0; i < len; ++i ) {
      const uint8_t byte_val = val >> ( ( len - i - 1 ) * 8 );
      buffer_.push_back( byte_val );
    }
  }

  void buffer( const Buffer& buf )
  {
    flush();
    output_.push_back( buf );
  }

  void buffer( const std::vector<Buffer>& bufs )
  {
    for ( const auto& b : bufs ) {
      buffer( b );
    }
  }

  void flush()
  {
    output_.emplace_back( std::move( buffer_ ) );
    buffer_.clear();
  }

  std::vector<Buffer> output()
  {
    flush();
    return output_;
  }
};

// Helper to serialize any object (without constructing a Serializer of the caller's own)
template<class T>
std::vector<Buffer> serialize( const T& obj )
{
  Serializer s;
  obj.serialize( s );
  return s.output();
}

// Helper to parse any object (without constructing a Parser of the caller's own). Returns true if successful.
template<class T>
bool parse( T& obj, const std::vector<Buffer>& buffers )
{
  Parser p { buffers };
  obj.parse( p );
  return not p.has_error();
}
