#include "bidirectional_stream_copy.hh"

#include "byte_stream.hh"
#include "eventloop.hh"

#include <algorithm>
#include <iostream>
#include <unistd.h>

using namespace std;

void bidirectional_stream_copy( Socket& socket, string_view peer_name )
{
  constexpr size_t buffer_size = 1048576;

  EventLoop _eventloop {};
  FileDescriptor _input { STDIN_FILENO };
  FileDescriptor _output { STDOUT_FILENO };
  ByteStream _outbound { buffer_size };
  ByteStream _inbound { buffer_size };
  bool _outbound_shutdown { false };
  bool _inbound_shutdown { false };

  socket.set_blocking( false );
  _input.set_blocking( false );
  _output.set_blocking( false );

  // rule 1: read from stdin into outbound byte stream
  _eventloop.add_rule(
    "read from stdin into outbound byte stream",
    _input,
    Direction::In,
    [&] {
      string data;
      data.resize( _outbound.writer().available_capacity() );
      _input.read( data );
      _outbound.writer().push( move( data ) );
      if ( _input.eof() ) {
        _outbound.writer().close();
      }
    },
    [&] {
      return !_outbound.has_error() and !_inbound.has_error() and ( _outbound.writer().available_capacity() > 0 )
             and !_outbound.writer().is_closed();
    },
    [&] { _outbound.writer().close(); },
    [&] {
      cerr << "DEBUG: Outbound stream had error from source.\n";
      _outbound.set_error();
      _inbound.set_error();
    } );

  // rule 2: read from outbound byte stream into socket
  _eventloop.add_rule(
    "read from outbound byte stream into socket",
    socket,
    Direction::Out,
    [&] {
      if ( _outbound.reader().bytes_buffered() ) {
        _outbound.reader().pop( socket.write( _outbound.reader().peek() ) );
      }
      if ( _outbound.reader().is_finished() ) {
        socket.shutdown( SHUT_WR );
        _outbound_shutdown = true;
        cerr << "DEBUG: Outbound stream to " << peer_name << " finished.\n";
      }
    },
    [&] {
      return _outbound.reader().bytes_buffered() or ( _outbound.reader().is_finished() and not _outbound_shutdown );
    },
    [&] { _outbound.writer().close(); },
    [&] {
      cerr << "DEBUG: Outbound stream had error from destination.\n";
      _outbound.set_error();
      _inbound.set_error();
    } );

  // rule 3: read from socket into inbound byte stream
  _eventloop.add_rule(
    "read from socket into inbound byte stream",
    socket,
    Direction::In,
    [&] {
      string data;
      data.resize( _inbound.writer().available_capacity() );
      socket.read( data );
      _inbound.writer().push( move( data ) );
      if ( socket.eof() ) {
        _inbound.writer().close();
      }
    },
    [&] {
      return !_inbound.has_error() and !_outbound.has_error() and ( _inbound.writer().available_capacity() > 0 )
             and !_inbound.writer().is_closed();
    },
    [&] { _inbound.writer().close(); },
    [&] {
      cerr << "DEBUG: Inbound stream had error from source.\n";
      _outbound.set_error();
      _inbound.set_error();
    } );

  // rule 4: read from inbound byte stream into stdout
  _eventloop.add_rule(
    "read from inbound byte stream into stdout",
    _output,
    Direction::Out,
    [&] {
      if ( _inbound.reader().bytes_buffered() ) {
        _inbound.reader().pop( _output.write( _inbound.reader().peek() ) );
      }
      if ( _inbound.reader().is_finished() ) {
        _output.close();
        _inbound_shutdown = true;
        cerr << "DEBUG: Inbound stream from " << peer_name << " finished"
             << ( _inbound.has_error() ? " uncleanly.\n" : ".\n" );
      }
    },
    [&] {
      return _inbound.reader().bytes_buffered() or ( _inbound.reader().is_finished() and not _inbound_shutdown );
    },
    [&] { _inbound.writer().close(); },
    [&] {
      cerr << "DEBUG: Inbound stream had error from destination.\n";
      _outbound.set_error();
      _inbound.set_error();
    } );

  // loop until completion
  while ( true ) {
    if ( EventLoop::Result::Exit == _eventloop.wait_next_event( -1 ) ) {
      return;
    }
  }
}
