//
// reactive_stream_socket_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

#ifndef ASIO_DETAIL_REACTIVE_STREAM_SOCKET_SERVICE_HPP
#define ASIO_DETAIL_REACTIVE_STREAM_SOCKET_SERVICE_HPP

#include "asio/detail/push_options.hpp"

#include "asio/service_factory.hpp"
#include "asio/socket_error.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/detail/socket_types.hpp"

namespace asio {
namespace detail {

template <typename Demuxer, typename Reactor>
class reactive_stream_socket_service
{
public:
  // The native type of the stream socket. This type is dependent on the
  // underlying implementation of the socket layer.
  typedef socket_type impl_type;

  // Return a null stream socket implementation.
  static impl_type null()
  {
    return invalid_socket;
  }

  // Constructor.
  reactive_stream_socket_service(Demuxer& d)
    : demuxer_(d),
      reactor_(d.get_service(service_factory<Reactor>()))
  {
  }

  // The demuxer type for this service.
  typedef Demuxer demuxer_type;

  // Get the demuxer associated with the service.
  demuxer_type& demuxer()
  {
    return demuxer_;
  }

  // Open a new stream socket implementation.
  void open(impl_type& impl, impl_type new_impl)
  {
    impl = new_impl;
  }

  // Destroy a stream socket implementation.
  void close(impl_type& impl)
  {
    if (impl != null())
    {
      reactor_.close_descriptor(impl, socket_ops::close);
      impl = null();
    }
  }

  // Set a socket option.
  template <typename Option, typename Error_Handler>
  void set_option(impl_type& impl, const Option& option,
      Error_Handler error_handler)
  {
    if (socket_ops::setsockopt(impl, option.level(), option.name(),
          option.data(), option.size()))
      error_handler(socket_error(socket_ops::get_error()));
  }

  // Set a socket option.
  template <typename Option, typename Error_Handler>
  void get_option(impl_type& impl, Option& option, Error_Handler error_handler)
  {
    socket_len_type size = option.size();
    if (socket_ops::getsockopt(impl, option.level(), option.name(),
          option.data(), &size))
      error_handler(socket_error(socket_ops::get_error()));
  }

  // Get the local socket address.
  template <typename Address, typename Error_Handler>
  void get_local_address(impl_type& impl, Address& address,
      Error_Handler error_handler)
  {
    socket_addr_len_type addr_len = address.native_size();
    if (socket_ops::getsockname(impl, address.native_address(), &addr_len))
      error_handler(socket_error(socket_ops::get_error()));
    address.native_size(addr_len);
  }

  // Get the remote socket address.
  template <typename Address, typename Error_Handler>
  void get_remote_address(impl_type& impl, Address& address,
      Error_Handler error_handler)
  {
    socket_addr_len_type addr_len = address.native_size();
    if (socket_ops::getpeername(impl, address.native_address(), &addr_len))
      error_handler(socket_error(socket_ops::get_error()));
    address.native_size(addr_len);
  }

  // Send the given data to the peer. Returns the number of bytes sent or
  // 0 if the connection was closed cleanly.
  template <typename Error_Handler>
  size_t send(impl_type& impl, const void* data, size_t length,
      Error_Handler error_handler)
  {
    int bytes_sent = socket_ops::send(impl, data, length, 0);
    if (bytes_sent < 0)
    {
      error_handler(socket_error(socket_ops::get_error()));
      return 0;
    }
    return bytes_sent;
  }

  template <typename Handler>
  class send_handler
  {
  public:
    send_handler(impl_type impl, Demuxer& demuxer, const void* data,
        size_t length, Handler handler)
      : impl_(impl),
        demuxer_(demuxer),
        data_(data),
        length_(length),
        handler_(handler)
    {
    }

    void do_operation()
    {
      int bytes = socket_ops::send(impl_, data_, length_, 0);
      socket_error error(bytes < 0
          ? socket_ops::get_error() : socket_error::success);
      demuxer_.post(bind_handler(handler_, error, bytes < 0 ? 0 : bytes));
      demuxer_.work_finished();
    }

    void do_cancel()
    {
      socket_error error(socket_error::operation_aborted);
      demuxer_.post(bind_handler(handler_, error, 0));
      demuxer_.work_finished();
    }

  private:
    impl_type impl_;
    Demuxer& demuxer_;
    const void* data_;
    size_t length_;
    Handler handler_;
  };

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename Handler>
  void async_send(impl_type& impl, const void* data, size_t length,
      Handler handler)
  {
    if (impl == null())
    {
      socket_error error(socket_error::bad_descriptor);
      demuxer_.post(bind_handler(handler, error, 0));
    }
    else
    {
      demuxer_.work_started();
      reactor_.start_write_op(impl,
          send_handler<Handler>(impl, demuxer_, data, length, handler));
    }
  }

  // Receive some data from the peer. Returns the number of bytes received or
  // 0 if the connection was closed cleanly.
  template <typename Error_Handler>
  size_t recv(impl_type& impl, void* data, size_t max_length,
      Error_Handler error_handler)
  {
    int bytes_recvd = socket_ops::recv(impl, data, max_length, 0);
    if (bytes_recvd < 0)
    {
      error_handler(socket_error(socket_ops::get_error()));
      return 0;
    }
    return bytes_recvd;
  }

  template <typename Handler>
  class recv_handler
  {
  public:
    recv_handler(impl_type impl, Demuxer& demuxer, void* data,
        size_t max_length, Handler handler)
      : impl_(impl),
        demuxer_(demuxer),
        data_(data),
        max_length_(max_length),
        handler_(handler)
    {
    }

    void do_operation()
    {
      int bytes = socket_ops::recv(impl_, data_, max_length_, 0);
      socket_error error(bytes < 0
          ? socket_ops::get_error() : socket_error::success);
      demuxer_.post(bind_handler(handler_, error, bytes < 0 ? 0 : bytes));
      demuxer_.work_finished();
    }

    void do_cancel()
    {
      socket_error error(socket_error::operation_aborted);
      demuxer_.post(bind_handler(handler_, error, 0));
      demuxer_.work_finished();
    }

  private:
    impl_type impl_;
    Demuxer& demuxer_;
    void* data_;
    size_t max_length_;
    Handler handler_;
  };

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename Handler>
  void async_recv(impl_type& impl, void* data, size_t max_length,
      Handler handler)
  {
    if (impl == null())
    {
      socket_error error(socket_error::bad_descriptor);
      demuxer_.post(bind_handler(handler, error, 0));
    }
    else
    {
      demuxer_.work_started();
      reactor_.start_read_op(impl,
          recv_handler<Handler>( impl, demuxer_, data, max_length, handler));
    }
  }

private:
  // The demuxer used for dispatching handlers.
  Demuxer& demuxer_;

  // The reactor that performs event demultiplexing for the provider.
  Reactor& reactor_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_REACTIVE_STREAM_SOCKET_SERVICE_HPP
