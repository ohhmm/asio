//
// udp.hpp
// ~~~~~~~
//
// Copyright (c) 2003, 2004 Christopher M. Kohlhoff (chris@kohlhoff.com)
//
// Permission to use, copy, modify, distribute and sell this software and its
// documentation for any purpose is hereby granted without fee, provided that
// the above copyright notice appears in all copies and that both the copyright
// notice and this permission notice appear in supporting documentation. This
// software is provided "as is" without express or implied warranty, and with
// no claim as to its suitability for any purpose.
//

#ifndef ASIO_IPV4_UDP_HPP
#define ASIO_IPV4_UDP_HPP

#include "asio/detail/push_options.hpp"

#include "asio/detail/socket_types.hpp"

namespace asio {
namespace ipv4 {

/// Encapsulates the flags needed for UDP.
/**
 * The asio::ipv4::udp class contains the flags necessary to use UDP sockets.
 *
 * @par Thread Safety:
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Safe.
 *
 * @par Concepts:
 * Protocol.
 */
class udp
{
public:
  class endpoint;

  /// Obtain an identifier for the type of the protocol.
  int type() const
  {
    return SOCK_DGRAM;
  }

  /// Obtain an identifier for the protocol.
  int protocol() const
  {
    return IPPROTO_UDP;
  }

  /// Obtain an identifier for the protocol family.
  int family() const
  {
    return PF_INET;
  }
};

/// Describes an endpoint for a UDP socket.
/**
 * The asio::ipv4::udp::endpoint class describes an endpoint that may be
 * associated with a particular socket.
 *
 * @par Thread Safety:
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 *
 * @par Concepts:
 * Endpoint.
 */
class udp::endpoint
{
public:
  /// The protocol type associated with the endpoint.
  typedef udp protocol_type;

  /// The native type of the endpoint structure. This type is dependent on the
  /// underlying implementation of the socket layer.
  typedef detail::socket_addr_type native_data_type;

  /// The native type for the size of the endpoint structure. This type is
  /// dependent on the underlying implementation of the socket layer.
  typedef detail::socket_addr_len_type native_size_type;

  /// Default constructor.
  endpoint()
  {
    addr_.sin_family = AF_INET;
    addr_.sin_port = 0;
    addr_.sin_addr.s_addr = INADDR_ANY;
  }

  /// Construct an endpoint using a port number, specified in the host's byte
  /// order. The IP address will be the any address (i.e. INADDR_ANY). This
  /// constructor would typically be used for accepting new connections.
  endpoint(short port_num)
  {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_num);
    addr_.sin_addr.s_addr = INADDR_ANY;
  }

  /// Construct an endpoint using a port number and an IP address. This
  /// constructor may be used for accepting connections on a specific interface
  /// or for making a connection to a remote endpoint.
  endpoint(short port_num, const asio::ipv4::address& addr)
  {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_num);
    addr_.sin_addr.s_addr = htonl(addr.to_ulong());
  }

  /// Copy constructor.
  endpoint(const endpoint& other)
    : addr_(other.addr_)
  {
  }

  /// Assign from another endpoint.
  endpoint& operator=(const endpoint& other)
  {
    addr_ = other.addr_;
    return *this;
  }

  /// The protocol associated with the endpoint.
  protocol_type protocol() const
  {
    return protocol_type();
  }

  /// Get the underlying endpoint in the native type.
  native_data_type* native_data()
  {
    return reinterpret_cast<native_data_type*>(&addr_);
  }

  /// Get the underlying endpoint in the native type.
  const native_data_type* native_data() const
  {
    return reinterpret_cast<const native_data_type*>(&addr_);
  }

  /// Get the underlying size of the endpoint in the native type.
  native_size_type native_size() const
  {
    return sizeof(addr_);
  }

  /// Set the underlying size of the endpoint in the native type.
  void native_size(native_size_type size)
  {
    if (size != sizeof(addr_))
      throw socket_error(socket_error::invalid_argument);
  }

  /// Get the port associated with the endpoint. The port number is always in
  /// the host's byte order.
  short port() const
  {
    return ntohs(addr_.sin_port);
  }

  /// Set the port associated with the endpoint. The port number is always in
  /// the host's byte order.
  void port(short port_num)
  {
    addr_.sin_port = htons(port_num);
  }

  /// Get the IP address associated with the endpoint.
  asio::ipv4::address address() const
  {
    return addr_.sin_addr.s_addr;
  }

  /// Set the IP address associated with the endpoint.
  void address(const asio::ipv4::address& addr)
  {
    addr_.sin_addr.s_addr = htonl(addr.to_ulong());
  }

private:
  // The underlying IPv4 socket address.
  detail::inet_addr_v4_type addr_;
};

} // namespace ipv4
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IPV4_UDP_HPP
