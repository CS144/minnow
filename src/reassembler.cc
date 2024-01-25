#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( is_last_substring )
    EOF_index_ = first_index + data.size();
  if ( EOF_index_ <= next_assembled_index_ )
    output_.writer().close();
  if ( first_index + data.size() <= next_assembled_index_ )
    return;

  uint64_t len = data.size();
  auto pre_it = unassembled_str_.upper_bound( first_index );
  if ( pre_it != unassembled_str_.begin() )
    --pre_it;
  uint64_t now_index = first_index;
  if ( pre_it != unassembled_str_.end() && pre_it->first <= first_index ) {
    uint64_t pre_index = pre_it->first;
    if ( first_index < pre_index + pre_it->second.size() )
      now_index = pre_index + pre_it->second.size();
  } else if ( first_index < next_assembled_index_ ) {
    now_index = next_assembled_index_;
  }

  if ( first_index + data.size() <= now_index )
    return;
  uint64_t unacceptable_index = next_assembled_index_ + output_.writer().available_capacity();
  if ( now_index >= unacceptable_index )
    return;

  uint64_t data_start_pos = now_index - first_index;
  len = data.size() - data_start_pos;
  auto nxt_it = unassembled_str_.lower_bound( now_index );
  while ( nxt_it != unassembled_str_.end() && nxt_it->first < now_index + len && now_index <= nxt_it->first ) {
    if ( now_index + len < nxt_it->first + nxt_it->second.size() ) {
      len = nxt_it->first - now_index;
      break;
    } else {
      unassembled_bytes_num_ -= nxt_it->second.size();
      nxt_it = unassembled_str_.erase( nxt_it );
    }
  }

  if ( now_index + len > unacceptable_index )
    len -= unacceptable_index - now_index;
  unassembled_bytes_num_ += len;
  unassembled_str_.insert( make_pair( now_index, data.substr( data_start_pos, len ) ) );

  for ( auto it = unassembled_str_.begin(); it != unassembled_str_.end() && it->first == next_assembled_index_; ) {
    output_.writer().push( it->second );
    unassembled_bytes_num_ -= it->second.size();
    next_assembled_index_ += it->second.size();
    it = unassembled_str_.erase( it );
  }

  if ( EOF_index_ <= next_assembled_index_ )
    output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  return unassembled_bytes_num_;
}
