//
// reactive_stream_socket_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

  // Create a new stream socket implementation.
  void create(impl_type& impl, impl_type new_impl)
  {
    impl = new_impl;
  }

  // Destroy a stream socket implementation.
  void destroy(impl_type& impl)
  {
    if (impl != null())
    {
      reactor_.close_descriptor(impl, socket_ops::close);
      impl = null();
    }
  }

  // Set a socket option. Throws a socket_error exception on failure.
  template <typename Option>
  void set_option(impl_type& impl, const Option& option)
  {
    if (socket_ops::setsockopt(impl, option.level(), option.name(),
          option.data(), option.size()))
        throw socket_error(socket_ops::get_error());
  }

  // Set a socket option. Throws a socket_error exception on failure.
  template <typename Option>
  void get_option(impl_type& impl, Option& option)
  {
    socket_len_type size = option.size();
    if (socket_ops::getsockopt(impl, option.level(), option.name(),
          option.data(), &size))
        throw socket_error(socket_ops::get_error());
  }

  // Send the given data to the peer. Returns the number of bytes sent or
  // 0 if the connection was closed cleanly. Throws a socket_error exception
  // on failure.
  size_t send(impl_type& impl, const void* data, size_t length)
  {
    int bytes_sent = socket_ops::send(impl, data, length, 0);
    if (bytes_sent < 0)
      throw socket_error(socket_ops::get_error());
    return bytes_sent;
  }

  template <typename Handler, typename Completion_Context>
  class send_handler
  {
  public:
    send_handler(impl_type impl, Demuxer& demuxer, const void* data,
        size_t length, Handler handler, Completion_Context& context)
      : impl_(impl),
        demuxer_(demuxer),
        data_(data),
        length_(length),
        handler_(handler),
        context_(context)
    {
    }

    void do_operation()
    {
      int bytes = socket_ops::send(impl_, data_, length_, 0);
      socket_error error(bytes < 0
          ? socket_ops::get_error() : socket_error::success);
      demuxer_.operation_completed(bind_handler(handler_, error, bytes),
          context_);
    }

    void do_cancel()
    {
      socket_error error(socket_error::operation_aborted);
      demuxer_.operation_completed(bind_handler(handler_, error, 0),
          context_);
    }

  private:
    impl_type impl_;
    Demuxer& demuxer_;
    const void* data_;
    size_t length_;
    Handler handler_;
    Completion_Context& context_;
  };

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename Handler, typename Completion_Context>
  void async_send(impl_type& impl, const void* data, size_t length,
      Handler handler, Completion_Context& context)
  {
    demuxer_.operation_started();
    reactor_.start_write_op(impl, send_handler<Handler, Completion_Context>(
          impl, demuxer_, data, length, handler, context));
  }

  // Receive some data from the peer. Returns the number of bytes received or
  // 0 if the connection was closed cleanly. Throws a socket_error exception
  // on failure.
  size_t recv(impl_type& impl, void* data, size_t max_length)
  {
    int bytes_recvd = socket_ops::recv(impl, data, max_length, 0);
    if (bytes_recvd < 0)
      throw socket_error(socket_ops::get_error());
    return bytes_recvd;
  }

  template <typename Handler, typename Completion_Context>
  class recv_handler
  {
  public:
    recv_handler(impl_type impl, Demuxer& demuxer, void* data,
        size_t max_length, Handler handler, Completion_Context& context)
      : impl_(impl),
        demuxer_(demuxer),
        data_(data),
        max_length_(max_length),
        handler_(handler),
        context_(context)
    {
    }

    void do_operation()
    {
      int bytes = socket_ops::recv(impl_, data_, max_length_, 0);
      socket_error error(bytes < 0
          ? socket_ops::get_error() : socket_error::success);
      demuxer_.operation_completed(bind_handler(handler_, error, bytes),
          context_);
    }

    void do_cancel()
    {
      socket_error error(socket_error::operation_aborted);
      demuxer_.operation_completed(bind_handler(handler_, error, 0), context_);
    }

  private:
    impl_type impl_;
    Demuxer& demuxer_;
    void* data_;
    size_t max_length_;
    Handler handler_;
    Completion_Context& context_;
  };

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename Handler, typename Completion_Context>
  void async_recv(impl_type& impl, void* data, size_t max_length,
      Handler handler, Completion_Context& context)
  {
    demuxer_.operation_started();
    reactor_.start_read_op(impl, recv_handler<Handler, Completion_Context>(
          impl, demuxer_, data, max_length, handler, context));
  }

private:
  // The demuxer used for delivering completion notifications.
  Demuxer& demuxer_;

  // The reactor that performs event demultiplexing for the provider.
  Reactor& reactor_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_REACTIVE_STREAM_SOCKET_SERVICE_HPP
