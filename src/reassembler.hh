#pragma once
#include "byte_stream.hh"
#include <cstdint>
#include <queue>
#include <set>
#include <string>
#include <vector>
struct block_node
{
  uint64_t begin {};
  uint64_t length { 0 };
  uint64_t end{};
  std::string data {};
  bool is_last {};
  inline bool operator<( const block_node& t ) const
  {
    bool res = begin < t.begin;
    return res;
  }
};
class Reassembler
{
public:
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring, Writer& output );
  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

private:
  std::set<block_node> blocks_ {};
  uint64_t first_unassembled_index { 0 };
  void push_substring(block_node&,uint64_t);
  void merge(block_node&,std::set<block_node>::iterator);
  // uint64_t first_unacceptable_index {};
};
