//
// basic_dgram_socket.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003 Christopher M. Kohlhoff (chris@kohlhoff.com)
//
// Permission to use, copy, modify, distribute and sell this software and its
// documentation for any purpose is hereby granted without fee, provided that
// the above copyright notice appears in all copies and that both the copyright
// notice and this permission notice appear in supporting documentation. This
// software is provided "as is" without express or implied warranty, and with
// no claim as to its suitability for any purpose.
//

#ifndef ASIO_BASIC_DGRAM_SOCKET_HPP
#define ASIO_BASIC_DGRAM_SOCKET_HPP

#include "asio/detail/push_options.hpp"

#include "asio/detail/push_options.hpp"
#include <boost/noncopyable.hpp>
#include "asio/detail/pop_options.hpp"

#include "asio/completion_context.hpp"
#include "asio/demuxer.hpp"

namespace asio {

class socket_address;
class socket_error;

/// The basic_dgram_socket class template provides asynchronous and blocking
/// datagram-oriented socket functionality. Most applications will simply use
/// the dgram_socket typedef.
template <typename Service>
class basic_dgram_socket
  : private boost::noncopyable
{
public:
  /// The native implementation type of the dgram socket.
  typedef typename Service::impl_type impl_type;

  /// Construct a dgram_socket without opening it. The socket needs to be
  /// opened before data can be sent or received on it.
  explicit basic_dgram_socket(demuxer& d)
    : service_(dynamic_cast<Service&>(d.get_service(Service::id)))
  {
    service_.nullify(impl_);
  }

  /// Construct a dgram_socket opened on the given address.
  basic_dgram_socket(demuxer& d, const socket_address& address)
    : service_(dynamic_cast<Service&>(d.get_service(Service::id)))
  {
    service_.create(impl_, address);
  }

  /// Destructor.
  ~basic_dgram_socket()
  {
    service_.destroy(impl_);
  }

  /// Open the socket on the given address.
  void open(const socket_address& address)
  {
    service_.create(impl_, address);
  }

  /// Close the socket.
  void close()
  {
    service_.destroy(impl_);
  }

  /// Get the underlying implementation in the native type.
  impl_type impl() const
  {
    return impl_;
  }

  /// Attach an existing implementation to the dgram socket. The dgram_socket
  /// object takes ownership of the implementation.
  void attach_impl(impl_type impl)
  {
    service_.attach(impl_, impl);
  }

  /// Send a datagram to the specified address. Returns the number of bytes
  /// sent. Throws a socket_error exception on failure.
  size_t sendto(const void* data, size_t length,
      const socket_address& destination)
  {
    return service_.sendto(impl_, data, length, destination);
  }

  /// The handler when a sendto operation is completed. The first argument is
  /// the error code, the second is the number of bytes sent.
  typedef typename Service::sendto_handler sendto_handler;

  /// Start an asynchronous send. The data being sent must be valid for the
  /// lifetime of the asynchronous operation.
  void async_sendto(const void* data, size_t length,
      const socket_address& destination, const sendto_handler& handler,
      completion_context& context = completion_context::null())
  {
    service_.async_sendto(impl_, data, length, destination, handler, context);
  }

  /// Receive a datagram with the address of the sender. Returns the number of
  /// bytes received. Throws a socket_error exception on failure.
  size_t recvfrom(void* data, size_t max_length,
      socket_address& sender_address)
  {
    return service_.recvfrom(impl_, data, max_length, sender_address);
  }
  
  /// The handler when a recvfrom operation is completed. The first argument is
  /// the error code, the second is the number of bytes received.
  typedef typename Service::recvfrom_handler recvfrom_handler;

  /// Start an asynchronous receive. The buffer for the data being received and
  /// the sender_address obejct must both be valid for the lifetime of the
  /// asynchronous operation.
  void async_recvfrom(void* data, size_t max_length,
      socket_address& sender_address, const recvfrom_handler& handler,
      completion_context& context = completion_context::null())
  {
    service_.async_recvfrom(impl_, data, max_length, sender_address, handler,
        context);
  }

private:
  /// The backend service implementation.
  Service& service_;

  /// The underlying native implementation.
  impl_type impl_;
};

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_BASIC_DGRAM_SOCKET_HPP
