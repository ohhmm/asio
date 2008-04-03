//
// connect_pair.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_LOCAL_CONNECT_PAIR_HPP
#define ASIO_LOCAL_CONNECT_PAIR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/push_options.hpp"

#include "asio/basic_socket.hpp"
#include "asio/error.hpp"
#include "asio/local/basic_endpoint.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/detail/throw_error.hpp"

#if defined(ASIO_HAS_LOCAL_SOCKETS)

namespace asio {
namespace local {

/// Create a pair of connected sockets.
template <typename Protocol, typename SocketService1, typename SocketService2>
void connect_pair(
    basic_socket<Protocol, SocketService1>& socket1,
    basic_socket<Protocol, SocketService2>& socket2);

/// Create a pair of connected sockets.
template <typename Protocol, typename SocketService1, typename SocketService2>
asio::error_code connect_pair(
    basic_socket<Protocol, SocketService1>& socket1,
    basic_socket<Protocol, SocketService2>& socket2,
    asio::error_code& ec);

template <typename Protocol, typename SocketService1, typename SocketService2>
inline void connect_pair(
    basic_socket<Protocol, SocketService1>& socket1,
    basic_socket<Protocol, SocketService2>& socket2)
{
  asio::error_code ec;
  connect_pair(socket1, socket2, ec);
  asio::detail::throw_error(ec);
}

template <typename Protocol, typename SocketService1, typename SocketService2>
inline asio::error_code connect_pair(
    basic_socket<Protocol, SocketService1>& socket1,
    basic_socket<Protocol, SocketService2>& socket2,
    asio::error_code& ec)
{
  // Check that this function is only being used with a UNIX domain socket.
  asio::local::basic_endpoint<Protocol>* tmp
    = static_cast<typename Protocol::endpoint*>(0);
  (void)tmp;

  Protocol protocol;
  asio::detail::socket_type sv[2];
  if (asio::detail::socket_ops::socketpair(protocol.family(),
        protocol.type(), protocol.protocol(), sv, ec)
      == asio::detail::socket_error_retval)
    return ec;

  if (socket1.assign(protocol, sv[0], ec))
  {
    asio::error_code temp_ec;
    asio::detail::socket_ops::close(sv[0], temp_ec);
    asio::detail::socket_ops::close(sv[1], temp_ec);
    return ec;
  }

  if (socket2.assign(protocol, sv[1], ec))
  {
    asio::error_code temp_ec;
    socket1.close(temp_ec);
    asio::detail::socket_ops::close(sv[1], temp_ec);
    return ec;
  }

  return ec;
}

} // namespace local
} // namespace asio

#endif // defined(ASIO_HAS_LOCAL_SOCKETS)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_LOCAL_CONNECT_PAIR_HPP
