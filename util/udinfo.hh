#pragma once

#include <cstdint>

// Small struct to represent "user datagram" information (UDP, or the "UDP-like" portion of a TCP header)

struct UserDatagramInfo
{
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t cksum;
};
