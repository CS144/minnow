#pragma once

#include "file_descriptor.hh"
#include "lossy_fd_adapter.hh"
#include "socket.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"

#include <optional>
#include <utility>

//! \brief Basic functionality for file descriptor adaptors
//! \details See TCPOverIPv4OverTunFdAdapter for more information.
class FdAdapterBase
{
private:
  FdAdapterConfig _cfg {}; //!< Configuration values
  bool _listen = false;    //!< Is the connected TCP FSM in listen state?

protected:
  FdAdapterConfig& config_mutable() { return _cfg; }

public:
  //! \brief Set the listening flag
  //! \param[in] l is the new value for the flag
  void set_listening( const bool l ) { _listen = l; }

  //! \brief Get the listening flag
  //! \returns whether the FdAdapter is listening for a new connection
  bool listening() const { return _listen; }

  //! \brief Get the current configuration
  //! \returns a const reference
  const FdAdapterConfig& config() const { return _cfg; }

  //! \brief Get the current configuration (mutable)
  //! \returns a mutable reference
  FdAdapterConfig& config_mut() { return _cfg; }

  //! Called periodically when time elapses
  void tick( const size_t unused [[maybe_unused]] ) {}
};
