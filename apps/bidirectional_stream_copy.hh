#pragma once

#include "socket.hh"

//! Copy socket input/output to stdin/stdout until finished
void bidirectional_stream_copy( Socket& socket, std::string_view peer_name );
