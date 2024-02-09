#include "tuntap_adapter.hh"
#include "parser.hh"

using namespace std;

optional<TCPMessage> TCPOverIPv4OverTunFdAdapter::read()
{
  vector<string> strs( 2 );
  strs.front().resize( IPv4Header::LENGTH );
  _tun.read( strs );

  InternetDatagram ip_dgram;
  const vector<string> buffers = { strs.at( 0 ), strs.at( 1 ) };
  if ( parse( ip_dgram, buffers ) ) {
    return unwrap_tcp_in_ip( ip_dgram );
  }
  return {};
}

//! Specialize LossyFdAdapter to TCPOverIPv4OverTunFdAdapter
template class LossyFdAdapter<TCPOverIPv4OverTunFdAdapter>;
