#pragma once

#include "tcp_over_ip.hh"
#include "tcp_segment.hh"
#include "tun.hh"

#include <optional>
#include <unordered_map>
#include <utility>

template<class T>
concept TCPDatagramAdapter = requires( T a, TCPMessage seg ) {
  {
    a.write( seg )
  } -> std::same_as<void>;

  {
    a.read()
  } -> std::same_as<std::optional<TCPMessage>>;
};

//! \brief A FD adapter for IPv4 datagrams read from and written to a TUN device
class TCPOverIPv4OverTunFdAdapter : public TCPOverIPv4Adapter
{
private:
  TunFD _tun;

public:
  //! Construct from a TunFD
  explicit TCPOverIPv4OverTunFdAdapter( TunFD&& tun ) : _tun( std::move( tun ) ) {}

  //! Attempts to read and parse an IPv4 datagram containing a TCP segment related to the current connection
  std::optional<TCPMessage> read();

  //! Creates an IPv4 datagram from a TCP segment and writes it to the TUN device
  void write( const TCPMessage& seg ) { _tun.write( serialize( wrap_tcp_in_ip( seg ) ) ); }

  //! Access the underlying TUN device
  explicit operator TunFD&() { return _tun; }

  //! Access the underlying TUN device
  explicit operator const TunFD&() const { return _tun; }

  //! Access underlying file descriptor
  FileDescriptor& fd() { return _tun; }
};

static_assert( TCPDatagramAdapter<TCPOverIPv4OverTunFdAdapter> );
static_assert( TCPDatagramAdapter<LossyFdAdapter<TCPOverIPv4OverTunFdAdapter>> );
