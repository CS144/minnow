#pragma once

#include "buffer.hh"
#include "wrapping_integers.hh"

#include <string>

/*
 * The TCPSenderMessage structure contains the information sent from a TCP sender to its receiver.
 *
 * It contains four fields:
 *
 * 1) The sequence number (seqno) of the beginning of the segment. If the SYN flag is set, this is the
 *    sequence number of the SYN flag. Otherwise, it's the sequence number of the beginning of the payload.
 *
 * 2) The SYN flag. If set, it means this segment is the beginning of the byte stream, and that
 *    the seqno field contains the Initial Sequence Number (ISN) -- the zero point.
 *
 * 3) The payload: a substring (possibly empty) of the byte stream.
 *
 * 4) The FIN flag. If set, it means the payload represents the ending of the byte stream.
 */

struct TCPSenderMessage
{
  Wrap32 seqno { 0 };
  bool SYN { false };
  Buffer payload {};
  bool FIN { false };

  // How many sequence numbers does this segment use?
  size_t sequence_length() const { return SYN + payload.size() + FIN; }
};
