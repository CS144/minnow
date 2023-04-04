#pragma once

#include <memory>
#include <string>

class Buffer
{
  std::shared_ptr<std::string> buffer_;

public:
  // NOLINTBEGIN(*-explicit-*)

  Buffer( std::string str = {} ) : buffer_( make_shared<std::string>( std::move( str ) ) ) {}
  operator std::string_view() const { return *buffer_; }
  operator std::string&() { return *buffer_; }

  // NOLINTEND(*-explicit-*)

  std::string&& release() { return std::move( *buffer_ ); }
  size_t size() const { return buffer_->size(); }
  size_t length() const { return buffer_->length(); }
  bool empty() const { return buffer_->empty(); }
};
