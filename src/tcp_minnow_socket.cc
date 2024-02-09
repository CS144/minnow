#include "tcp_minnow_socket_impl.hh"

//! Specializations of TCPMinnowSocket for TCPOverIPv4OverTunFdAdapter and its lossy version
template class TCPMinnowSocket<TCPOverIPv4OverTunFdAdapter>;
template class TCPMinnowSocket<LossyFdAdapter<TCPOverIPv4OverTunFdAdapter>>;
