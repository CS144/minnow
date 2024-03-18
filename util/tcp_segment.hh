#pragma once

#include "parser.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "udinfo.hh"

struct TCPMessage
{
  TCPSenderMessage sender {};
  TCPReceiverMessage receiver {};
};

struct TCPSegment
{
  TCPMessage message {};
  UserDatagramInfo udinfo {};

  void parse( Parser& parser, uint32_t datagram_layer_pseudo_checksum );
  void serialize( Serializer& serializer ) const;

  void compute_checksum( uint32_t datagram_layer_pseudo_checksum );
};
