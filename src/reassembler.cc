/*
  Comments may include Chinese characters. Use UTF-8 to open this file.
*/
#include "reassembler.hh"
#include <algorithm>
#include <cstdint>
#include <utility>
using namespace std;
void Reassembler::merge(block_node& bn,set<block_node>::iterator bo)
{
  if(bo==blocks_.end()){
    blocks_.insert(bn);
    return;
  }
  //1
  if(bn.begin<bo->begin&&bn.end<=bo->end){
    bn.length=bo->begin-bn.begin;
    bn.data=bn.data.substr(0,bn.length);
    bn.end=bn.begin+bn.length;
    blocks_.insert(bn);
    return;
  }
  //2
  if(bn.begin>=bo->begin&&bn.end<=bo->end){
    return;
  }
  //5
  if(bn.end<=bo->begin){
    blocks_.insert(bn);
    return;
  }
  //6
  if(bn.begin>=bo->end){
    merge(bn,next(bo));
    return;
  }
  //3
  if(bn.begin>=bo->begin&&bn.end>bo->end){
    bn.length=bn.end-bo->end;
    bn.data=bn.data.substr(bo->end-bn.begin,bn.length);
    bn.begin=bo->end;
    bn.end=bn.begin+bn.length;
    merge(bn,next(bo));
    return;
  }
  //4
  if(bn.begin<bo->begin&&bn.end>bo->end){
    block_node bnn{bn.begin,bo->begin-bn.begin,bnn.begin+bnn.length,bn.data.substr(0,bnn.length),false};
    blocks_.insert(bnn);
    bn.length=bn.end-bo->end;
    bn.data=bn.data.substr(bo->end-bn.begin,bn.length);
    bn.begin=bo->end;
    bn.end=bn.begin+bn.length;
    merge(bn,next(bo));
    return;
  }
}

void Reassembler::push_substring(block_node& newnode,uint64_t first_unacceptable_index)
{
  if(newnode.end<=first_unassembled_index){
    return;
  }
  if(newnode.begin>=first_unacceptable_index){
    return;
  }
  if(newnode.begin<first_unassembled_index){
    newnode.length=first_unassembled_index-newnode.begin;
    newnode.data=newnode.data.substr(first_unassembled_index-newnode.begin,newnode.length);
    newnode.begin=first_unassembled_index;
  }
  if(newnode.end>first_unacceptable_index){
    newnode.data=newnode.data.substr(0,first_unacceptable_index-newnode.begin);
    newnode.length=first_unacceptable_index-newnode.begin;
    newnode.end=first_unacceptable_index;
  }

  set<block_node>::iterator blk;
  if(blocks_.empty()){
    blk=blocks_.end();
  }
  else{
    blk=blocks_.lower_bound(newnode);
    if(blk!=blocks_.begin()){
      blk=prev(blk);
    }
  }
  merge(newnode,blk);
  
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  
  if ( output.is_closed() ) {
    return;
  }

  if ( data.empty() ) {
    if ( is_last_substring ) {
      output.close();
    }
    return;
  }

  uint64_t first_unacceptable_index = first_unassembled_index + output.available_capacity();

  block_node newnode{first_index,data.length(),first_index+data.length(),move(data),is_last_substring};
  push_substring(newnode,first_unacceptable_index);

  while( blocks_.begin()->begin == first_unassembled_index ) {
    output.push( blocks_.begin()->data );
    first_unassembled_index = blocks_.begin()->end;
    if ( blocks_.begin()->is_last ) {
      output.close();
    }
    blocks_.erase( blocks_.begin() );
  }
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t res = 0;
  for ( const auto& subs : blocks_ ) {
    res += subs.length;
  }
  return res;
}
