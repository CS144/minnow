#include "address.hh"

#include "exception.hh"

#include <arpa/inet.h>
#include <array>
#include <cstring>
#include <linux/if_packet.h>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <system_error>

using namespace std;

//! Converts Raw to `sockaddr *`.
Address::Raw::operator sockaddr*()
{
  return reinterpret_cast<sockaddr*>( &storage ); // NOLINT(*-reinterpret-cast)
}

//! Converts Raw to `const sockaddr *`.
Address::Raw::operator const sockaddr*() const
{
  return reinterpret_cast<const sockaddr*>( &storage ); // NOLINT(*-reinterpret-cast)
}

//! \param[in] addr points to a raw socket address
//! \param[in] size is `addr`'s length
Address::Address( const sockaddr* addr, const size_t size ) : _size( size )
{
  // make sure proposed sockaddr can fit
  if ( size > sizeof( _address.storage ) ) {
    throw runtime_error( "invalid sockaddr size" );
  }

  memcpy( &_address.storage, addr, size );
}

//! Error category for getaddrinfo and getnameinfo failures.
class gai_error_category : public error_category
{
public:
  //! The name of the wrapped error
  const char* name() const noexcept override { return "gai_error_category"; }
  //! \brief An error message
  //! \param[in] return_value the error return value from [getaddrinfo(3)](\ref man3::getaddrinfo)
  //!                         or [getnameinfo(3)](\ref man3::getnameinfo)
  string message( const int return_value ) const noexcept override { return gai_strerror( return_value ); }
};

//! \param[in] node is the hostname or dotted-quad address
//! \param[in] service is the service name or numeric string
//! \param[in] hints are criteria for resolving the supplied name
Address::Address( const string& node, const string& service, const addrinfo& hints ) : _size()
{
  // prepare for the answer
  addrinfo* resolved_address = nullptr;

  // look up the name or names
  const int gai_ret = getaddrinfo( node.c_str(), service.c_str(), &hints, &resolved_address );
  if ( gai_ret != 0 ) {
    throw tagged_error( gai_error_category(), "getaddrinfo(" + node + ", " + service + ")", gai_ret );
  }

  // if success, should always have at least one entry
  if ( resolved_address == nullptr ) {
    throw runtime_error( "getaddrinfo returned successfully but with no results" );
  }

  // put resolved_address in a wrapper so it will get freed if we have to throw an exception
  auto addrinfo_deleter = []( addrinfo* const x ) { freeaddrinfo( x ); };
  unique_ptr<addrinfo, decltype( addrinfo_deleter )> wrapped_address( resolved_address, move( addrinfo_deleter ) );

  // assign to our private members (making sure size fits)
  *this = Address( wrapped_address->ai_addr, wrapped_address->ai_addrlen );
}

//! \brief Build a `struct addrinfo` containing hints for [getaddrinfo(3)](\ref man3::getaddrinfo)
//! \param[in] ai_flags is the value of the `ai_flags` field in the [struct addrinfo](\ref man3::getaddrinfo)
//! \param[in] ai_family is the value of the `ai_family` field in the [struct addrinfo](\ref
//! man3::getaddrinfo)
static inline addrinfo make_hints( int ai_flags, int ai_family ) // NOLINT(*-swappable-parameters)
{
  addrinfo hints {}; // value initialized to all zeros
  hints.ai_flags = ai_flags;
  hints.ai_family = ai_family;
  return hints;
}

//! \param[in] hostname to resolve
//! \param[in] service name (from `/etc/services`, e.g., "http" is port 80)
Address::Address( const string& hostname, const string& service )
  : Address( hostname, service, make_hints( AI_ALL, AF_INET ) )
{}

//! \param[in] ip address as a dotted quad ("1.1.1.1")
//! \param[in] port number
Address::Address( const string& ip, const uint16_t port )
  // tell getaddrinfo that we don't want to resolve anything
  : Address( ip, ::to_string( port ), make_hints( AI_NUMERICHOST | AI_NUMERICSERV, AF_INET ) )
{}

// accessors
pair<string, uint16_t> Address::ip_port() const
{
  array<char, NI_MAXHOST> ip {};
  array<char, NI_MAXSERV> port {};

  const int gni_ret = getnameinfo( static_cast<const sockaddr*>( _address ),
                                   _size,
                                   ip.data(),
                                   ip.size(),
                                   port.data(),
                                   port.size(),
                                   NI_NUMERICHOST | NI_NUMERICSERV );
  if ( gni_ret != 0 ) {
    throw tagged_error( gai_error_category(), "getnameinfo", gni_ret );
  }

  return { ip.data(), stoi( port.data() ) };
}

string Address::to_string() const
{
  const auto ip_and_port = ip_port();
  return ip_and_port.first + ":" + ::to_string( ip_and_port.second );
}

uint32_t Address::ipv4_numeric() const
{
  if ( _address.storage.ss_family != AF_INET or _size != sizeof( sockaddr_in ) ) {
    throw runtime_error( "ipv4_numeric called on non-IPV4 address" );
  }

  sockaddr_in ipv4_addr {};
  memcpy( &ipv4_addr, &_address.storage, _size );

  return be32toh( ipv4_addr.sin_addr.s_addr );
}

Address Address::from_ipv4_numeric( const uint32_t ip_address )
{
  sockaddr_in ipv4_addr {};
  ipv4_addr.sin_family = AF_INET;
  ipv4_addr.sin_addr.s_addr = htobe32( ip_address );

  return { reinterpret_cast<sockaddr*>( &ipv4_addr ), sizeof( ipv4_addr ) }; // NOLINT(*-reinterpret-cast)
}

// equality
bool Address::operator==( const Address& other ) const
{
  if ( _size != other._size ) {
    return false;
  }

  return 0 == memcmp( &_address, &other._address, _size );
}

// address families that correspond to each sockaddr type
template<typename sockaddr_type>
constexpr int sockaddr_family = -1;

template<>
constexpr int sockaddr_family<sockaddr_in> = AF_INET;

template<>
constexpr int sockaddr_family<sockaddr_in6> = AF_INET6;

template<>
constexpr int sockaddr_family<sockaddr_ll> = AF_PACKET;

// safely cast the address to its underlying sockaddr type
template<typename sockaddr_type>
const sockaddr_type* Address::as() const
{
  const sockaddr* raw { _address };
  if ( sizeof( sockaddr_type ) < size() or raw->sa_family != sockaddr_family<sockaddr_type> ) {
    throw std::runtime_error( "Address::as() conversion failure" );
  }

  return reinterpret_cast<const sockaddr_type*>( raw ); // NOLINT(*-reinterpret-cast)
}

template const sockaddr_in* Address::as<sockaddr_in>() const;
template const sockaddr_in6* Address::as<sockaddr_in6>() const;
template const sockaddr_ll* Address::as<sockaddr_ll>() const;
