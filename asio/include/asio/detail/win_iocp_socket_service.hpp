//
// win_iocp_socket_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2006 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_IOCP_SOCKET_SERVICE_HPP
#define ASIO_DETAIL_WIN_IOCP_SOCKET_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/push_options.hpp"

#include "asio/detail/push_options.hpp"
#include <boost/config.hpp>
#include "asio/detail/pop_options.hpp"

// This service is only supported on Win32 (NT4 and later).
#if defined(BOOST_WINDOWS)
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400)

#include "asio/detail/push_options.hpp"
#include <cstring>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "asio/detail/pop_options.hpp"

#include "asio/buffer.hpp"
#include "asio/error.hpp"
#include "asio/error_handler.hpp"
#include "asio/io_service.hpp"
#include "asio/socket_base.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/select_reactor.hpp"
#include "asio/detail/socket_holder.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/detail/win_iocp_io_service.hpp"

namespace asio {
namespace detail {

class win_iocp_socket_service
  : public asio::io_service::service
{
public:
  // Base class for all operations.
  typedef win_iocp_operation operation;

  struct noop_deleter { void operator()(void*) {} };
  typedef boost::shared_ptr<void> shared_cancel_token_type;
  typedef boost::weak_ptr<void> weak_cancel_token_type;

  // The native type of a socket.
  typedef socket_type native_type;

  // The implementation type of the socket.
  class implementation_type
  {
  private:
    // Only this service will have access to the internal values.
    friend class win_iocp_socket_service;

    // The native socket representation.
    socket_type socket_;

    // We use a shared pointer as a cancellation token here work around the
    // broken Windows support for cancellation. MSDN says that when you call
    // closesocket any outstanding WSARecv or WSASend operations will complete
    // with the error ERROR_OPERATION_ABORTED. In practice they complete with
    // ERROR_NETNAME_DELETED, which means you can't tell the difference between
    // a local cancellation and the socket being hard-closed by the peer.
    shared_cancel_token_type cancel_token_;
  };

  // The type of the reactor used for connect operations.
  typedef detail::select_reactor<true> reactor_type;

  // The maximum number of buffers to support in a single operation.
  enum { max_buffers = 16 };

  // Constructor.
  win_iocp_socket_service(asio::io_service& io_service)
    : asio::io_service::service(io_service),
      iocp_service_(asio::use_service<win_iocp_io_service>(io_service)),
      reactor_(0)
  {
  }

  // Construct a new socket implementation.
  void construct(implementation_type& impl)
  {
    impl.socket_ = invalid_socket;
    impl.cancel_token_.reset();
  }

  // Destroy a socket implementation.
  void destroy(implementation_type& impl)
  {
    close(impl, asio::ignore_error());
  }

  // Open a new socket implementation.
  template <typename Protocol, typename Error_Handler>
  void open(implementation_type& impl, const Protocol& protocol,
      Error_Handler error_handler)
  {
    close(impl, asio::ignore_error());

    socket_holder sock(socket_ops::socket(protocol.family(), protocol.type(),
          protocol.protocol()));
    if (sock.get() == invalid_socket)
    {
      error_handler(asio::error(socket_ops::get_error()));
      return;
    }

    iocp_service_.register_socket(sock.get());

    impl.socket_ = sock.release();
    impl.cancel_token_.reset(static_cast<void*>(0), noop_deleter());
  }

  // Open a new socket implementation from a native socket.
  template <typename Error_Handler>
  void open(implementation_type& impl, const native_type& native_socket,
      Error_Handler error_handler)
  {
    close(impl, asio::ignore_error());

    iocp_service_.register_socket(native_socket);

    impl.socket_ = native_socket;
    impl.cancel_token_.reset(static_cast<void*>(0), noop_deleter());
  }

  // Destroy a socket implementation.
  template <typename Error_Handler>
  void close(implementation_type& impl, Error_Handler error_handler)
  {
    if (impl.socket_ != invalid_socket)
    {
      // Check if the reactor was created, in which case we need to close the
      // socket on the reactor as well to cancel any operations that might be
      // running there.
      reactor_type* reactor = static_cast<reactor_type*>(
            InterlockedCompareExchangePointer(
            reinterpret_cast<void**>(&reactor_), 0, 0));
      if (reactor)
        reactor->close_descriptor(impl.socket_);

      if (socket_ops::close(impl.socket_) == socket_error_retval)
      {
        error_handler(asio::error(socket_ops::get_error()));
      }
      else
      {
        impl.socket_ = invalid_socket;
        impl.cancel_token_.reset();
      }
    }
  }

  // Get the native socket representation.
  native_type native(implementation_type& impl)
  {
    return impl.socket_;
  }

  // Bind the socket to the specified local endpoint.
  template <typename Endpoint, typename Error_Handler>
  void bind(implementation_type& impl, const Endpoint& endpoint,
      Error_Handler error_handler)
  {
    if (socket_ops::bind(impl.socket_, endpoint.data(),
          endpoint.size()) == socket_error_retval)
      error_handler(asio::error(socket_ops::get_error()));
  }

  // Place the socket into the state where it will listen for new connections.
  template <typename Error_Handler>
  void listen(implementation_type& impl, int backlog,
      Error_Handler error_handler)
  {
    if (backlog == 0)
      backlog = SOMAXCONN;

    if (socket_ops::listen(impl.socket_, backlog) == socket_error_retval)
      error_handler(asio::error(socket_ops::get_error()));
  }

  // Set a socket option.
  template <typename Option, typename Error_Handler>
  void set_option(implementation_type& impl, const Option& option,
      Error_Handler error_handler)
  {
    if (socket_ops::setsockopt(impl.socket_, option.level(), option.name(),
          option.data(), option.size()))
      error_handler(asio::error(socket_ops::get_error()));
  }

  // Set a socket option.
  template <typename Option, typename Error_Handler>
  void get_option(const implementation_type& impl, Option& option,
      Error_Handler error_handler) const
  {
    size_t size = option.size();
    if (socket_ops::getsockopt(impl.socket_, option.level(), option.name(),
          option.data(), &size))
      error_handler(asio::error(socket_ops::get_error()));
  }

  // Perform an IO control command on the socket.
  template <typename IO_Control_Command, typename Error_Handler>
  void io_control(implementation_type& impl, IO_Control_Command& command,
      Error_Handler error_handler)
  {
    if (socket_ops::ioctl(impl.socket_, command.name(),
          static_cast<ioctl_arg_type*>(command.data())))
      error_handler(asio::error(socket_ops::get_error()));
  }

  // Get the local endpoint.
  template <typename Endpoint, typename Error_Handler>
  void get_local_endpoint(const implementation_type& impl, Endpoint& endpoint,
      Error_Handler error_handler) const
  {
    socket_addr_len_type addr_len = endpoint.capacity();
    if (socket_ops::getsockname(impl.socket_, endpoint.data(), &addr_len))
    {
      error_handler(asio::error(socket_ops::get_error()));
      return;
    }

    endpoint.resize(addr_len);
  }

  // Get the remote endpoint.
  template <typename Endpoint, typename Error_Handler>
  void get_remote_endpoint(const implementation_type& impl, Endpoint& endpoint,
      Error_Handler error_handler) const
  {
    socket_addr_len_type addr_len = endpoint.capacity();
    if (socket_ops::getpeername(impl.socket_, endpoint.data(), &addr_len))
    {
      error_handler(asio::error(socket_ops::get_error()));
      return;
    }

    endpoint.resize(addr_len);
  }

  /// Disable sends or receives on the socket.
  template <typename Error_Handler>
  void shutdown(implementation_type& impl, socket_base::shutdown_type what,
      Error_Handler error_handler)
  {
    if (socket_ops::shutdown(impl.socket_, what) != 0)
      error_handler(asio::error(socket_ops::get_error()));
  }

  // Send the given data to the peer. Returns the number of bytes sent.
  template <typename Const_Buffers, typename Error_Handler>
  size_t send(implementation_type& impl, const Const_Buffers& buffers,
      socket_base::message_flags flags, Error_Handler error_handler)
  {
    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Const_Buffers::const_iterator iter = buffers.begin();
    typename Const_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::const_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = const_cast<char*>(
          asio::buffer_cast<const char*>(buffer));
    }

    // Send the data.
    DWORD bytes_transferred = 0;
    int result = ::WSASend(impl.socket_, bufs,
        i, &bytes_transferred, flags, 0, 0);
    if (result != 0)
    {
      DWORD last_error = ::WSAGetLastError();
      if (last_error == ERROR_NETNAME_DELETED)
        last_error = WSAECONNRESET;
      error_handler(asio::error(last_error));
      return 0;
    }

    return bytes_transferred;
  }

  template <typename Handler>
  class send_operation
    : public operation
  {
  public:
    send_operation(asio::io_service& io_service,
        weak_cancel_token_type cancel_token, Handler handler)
      : operation(&send_operation<Handler>::do_completion_impl),
        work_(io_service),
        cancel_token_(cancel_token),
        handler_(handler)
    {
    }

  private:
    static void do_completion_impl(operation* op,
        DWORD last_error, size_t bytes_transferred)
    {
      // Take ownership of the operation object.
      typedef send_operation<Handler> op_type;
      op_type* handler_op(static_cast<op_type*>(op));
      typedef handler_alloc_traits<Handler, op_type> alloc_traits;
      handler_ptr<alloc_traits> ptr(handler_op->handler_, handler_op);

      // Map ERROR_NETNAME_DELETED to more useful error.
      if (last_error == ERROR_NETNAME_DELETED)
      {
        if (handler_op->cancel_token_.expired())
          last_error = ERROR_OPERATION_ABORTED;
        else
          last_error = WSAECONNRESET;
      }

      // Make a copy of the handler so that the memory can be deallocated before
      // the upcall is made.
      Handler handler(handler_op->handler_);

      // Free the memory associated with the handler.
      ptr.reset();

      // Call the handler.
      asio::error error(last_error);
      handler(error, bytes_transferred);
    }

    asio::io_service::work work_;
    weak_cancel_token_type cancel_token_;
    Handler handler_;
  };

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename Const_Buffers, typename Handler>
  void async_send(implementation_type& impl, const Const_Buffers& buffers,
      socket_base::message_flags flags, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef send_operation<Handler> value_type;
    typedef handler_alloc_traits<Handler, value_type> alloc_traits;
    raw_handler_ptr<alloc_traits> raw_ptr(handler);
    handler_ptr<alloc_traits> ptr(raw_ptr,
        owner(), impl.cancel_token_, handler);

    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Const_Buffers::const_iterator iter = buffers.begin();
    typename Const_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::const_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = const_cast<char*>(
          asio::buffer_cast<const char*>(buffer));
    }

    // Send the data.
    DWORD bytes_transferred = 0;
    int result = ::WSASend(impl.socket_, bufs, i,
        &bytes_transferred, flags, ptr.get(), 0);
    DWORD last_error = ::WSAGetLastError();

    // Check if the operation completed immediately.
    if (result != 0 && last_error != WSA_IO_PENDING)
    {
      ptr.reset();
      asio::error error(last_error);
      iocp_service_.post(bind_handler(handler, error, bytes_transferred));
    }
    else
    {
      ptr.release();
    }
  }

  // Send a datagram to the specified endpoint. Returns the number of bytes
  // sent.
  template <typename Const_Buffers, typename Endpoint, typename Error_Handler>
  size_t send_to(implementation_type& impl, const Const_Buffers& buffers,
      const Endpoint& destination, socket_base::message_flags flags,
      Error_Handler error_handler)
  {
    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Const_Buffers::const_iterator iter = buffers.begin();
    typename Const_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::const_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = const_cast<char*>(
          asio::buffer_cast<const char*>(buffer));
    }

    // Send the data.
    DWORD bytes_transferred = 0;
    int result = ::WSASendTo(impl.socket_, bufs, i, &bytes_transferred,
        flags, destination.data(), destination.size(), 0, 0);
    if (result != 0)
    {
      DWORD last_error = ::WSAGetLastError();
      error_handler(asio::error(last_error));
      return 0;
    }

    return bytes_transferred;
  }

  template <typename Handler>
  class send_to_operation
    : public operation
  {
  public:
    send_to_operation(asio::io_service& io_service, Handler handler)
      : operation(&send_to_operation<Handler>::do_completion_impl),
        work_(io_service),
        handler_(handler)
    {
    }

  private:
    static void do_completion_impl(operation* op,
        DWORD last_error, size_t bytes_transferred)
    {
      // Take ownership of the operation object.
      typedef send_to_operation<Handler> op_type;
      op_type* handler_op(static_cast<op_type*>(op));
      typedef handler_alloc_traits<Handler, op_type> alloc_traits;
      handler_ptr<alloc_traits> ptr(handler_op->handler_, handler_op);

      // Make a copy of the handler so that the memory can be deallocated before
      // the upcall is made.
      Handler handler(handler_op->handler_);

      // Free the memory associated with the handler.
      ptr.reset();

      // Call the handler.
      asio::error error(last_error);
      handler(error, bytes_transferred);
    }

    asio::io_service::work work_;
    Handler handler_;
  };

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename Const_Buffers, typename Endpoint, typename Handler>
  void async_send_to(implementation_type& impl, const Const_Buffers& buffers,
      const Endpoint& destination, socket_base::message_flags flags,
      Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef send_to_operation<Handler> value_type;
    typedef handler_alloc_traits<Handler, value_type> alloc_traits;
    raw_handler_ptr<alloc_traits> raw_ptr(handler);
    handler_ptr<alloc_traits> ptr(raw_ptr, owner(), handler);

    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Const_Buffers::const_iterator iter = buffers.begin();
    typename Const_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::const_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = const_cast<char*>(
          asio::buffer_cast<const char*>(buffer));
    }

    // Send the data.
    DWORD bytes_transferred = 0;
    int result = ::WSASendTo(impl.socket_, bufs, i, &bytes_transferred,
        flags, destination.data(), destination.size(), ptr.get(), 0);
    DWORD last_error = ::WSAGetLastError();

    // Check if the operation completed immediately.
    if (result != 0 && last_error != WSA_IO_PENDING)
    {
      ptr.reset();
      asio::error error(last_error);
      iocp_service_.post(bind_handler(handler, error, bytes_transferred));
    }
    else
    {
      ptr.release();
    }
  }

  // Receive some data from the peer. Returns the number of bytes received.
  template <typename Mutable_Buffers, typename Error_Handler>
  size_t receive(implementation_type& impl, const Mutable_Buffers& buffers,
      socket_base::message_flags flags, Error_Handler error_handler)
  {
    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Mutable_Buffers::const_iterator iter = buffers.begin();
    typename Mutable_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::mutable_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = asio::buffer_cast<char*>(buffer);
    }

    // Receive some data.
    DWORD bytes_transferred = 0;
    DWORD recv_flags = flags;
    int result = ::WSARecv(impl.socket_, bufs, i,
        &bytes_transferred, &recv_flags, 0, 0);
    if (result != 0)
    {
      DWORD last_error = ::WSAGetLastError();
      if (last_error == ERROR_NETNAME_DELETED)
        last_error = WSAECONNRESET;
      error_handler(asio::error(last_error));
      return 0;
    }
    if (bytes_transferred == 0)
    {
      error_handler(asio::error(asio::error::eof));
      return 0;
    }

    return bytes_transferred;
  }

  template <typename Handler>
  class receive_operation
    : public operation
  {
  public:
    receive_operation(asio::io_service& io_service,
        weak_cancel_token_type cancel_token, Handler handler)
      : operation(&receive_operation<Handler>::do_completion_impl),
        work_(io_service),
        cancel_token_(cancel_token),
        handler_(handler)
    {
    }

  private:
    static void do_completion_impl(operation* op,
        DWORD last_error, size_t bytes_transferred)
    {
      // Take ownership of the operation object.
      typedef receive_operation<Handler> op_type;
      op_type* handler_op(static_cast<op_type*>(op));
      typedef handler_alloc_traits<Handler, op_type> alloc_traits;
      handler_ptr<alloc_traits> ptr(handler_op->handler_, handler_op);

      // Map ERROR_NETNAME_DELETED to more useful error.
      if (last_error == ERROR_NETNAME_DELETED)
      {
        if (handler_op->cancel_token_.expired())
          last_error = ERROR_OPERATION_ABORTED;
        else
          last_error = WSAECONNRESET;
      }

      // Check for connection closed.
      else if (last_error == 0 && bytes_transferred == 0)
      {
        last_error = asio::error::eof;
      }

      // Make a copy of the handler so that the memory can be deallocated before
      // the upcall is made.
      Handler handler(handler_op->handler_);

      // Free the memory associated with the handler.
      ptr.reset();

      // Call the handler.
      asio::error error(last_error);
      handler(error, bytes_transferred);
    }

    asio::io_service::work work_;
    weak_cancel_token_type cancel_token_;
    Handler handler_;
  };

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename Mutable_Buffers, typename Handler>
  void async_receive(implementation_type& impl, const Mutable_Buffers& buffers,
      socket_base::message_flags flags, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef receive_operation<Handler> value_type;
    typedef handler_alloc_traits<Handler, value_type> alloc_traits;
    raw_handler_ptr<alloc_traits> raw_ptr(handler);
    handler_ptr<alloc_traits> ptr(raw_ptr,
        owner(), impl.cancel_token_, handler);

    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Mutable_Buffers::const_iterator iter = buffers.begin();
    typename Mutable_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::mutable_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = asio::buffer_cast<char*>(buffer);
    }

    // Receive some data.
    DWORD bytes_transferred = 0;
    DWORD recv_flags = flags;
    int result = ::WSARecv(impl.socket_, bufs, i,
        &bytes_transferred, &recv_flags, ptr.get(), 0);
    DWORD last_error = ::WSAGetLastError();
    if (result != 0 && last_error != WSA_IO_PENDING)
    {
      ptr.reset();
      asio::error error(last_error);
      iocp_service_.post(bind_handler(handler, error, bytes_transferred));
    }
    else
    {
      ptr.release();
    }
  }

  // Receive a datagram with the endpoint of the sender. Returns the number of
  // bytes received.
  template <typename Mutable_Buffers, typename Endpoint, typename Error_Handler>
  size_t receive_from(implementation_type& impl, const Mutable_Buffers& buffers,
      Endpoint& sender_endpoint, socket_base::message_flags flags,
      Error_Handler error_handler)
  {
    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Mutable_Buffers::const_iterator iter = buffers.begin();
    typename Mutable_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::mutable_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = asio::buffer_cast<char*>(buffer);
    }

    // Receive some data.
    DWORD bytes_transferred = 0;
    DWORD recv_flags = flags;
    int endpoint_size = sender_endpoint.capacity();
    int result = ::WSARecvFrom(impl.socket_, bufs, i, &bytes_transferred,
        &recv_flags, sender_endpoint.data(), &endpoint_size, 0, 0);
    if (result != 0)
    {
      DWORD last_error = ::WSAGetLastError();
      error_handler(asio::error(last_error));
      return 0;
    }
    if (bytes_transferred == 0)
    {
      error_handler(asio::error(asio::error::eof));
      return 0;
    }

    sender_endpoint.resize(endpoint_size);

    return bytes_transferred;
  }

  template <typename Endpoint, typename Handler>
  class receive_from_operation
    : public operation
  {
  public:
    receive_from_operation(asio::io_service& io_service,
        Endpoint& endpoint, Handler handler)
      : operation(
          &receive_from_operation<Endpoint, Handler>::do_completion_impl),
        endpoint_(endpoint),
        endpoint_size_(endpoint.capacity()),
        work_(io_service),
        handler_(handler)
    {
    }

    int& endpoint_size()
    {
      return endpoint_size_;
    }

  private:
    static void do_completion_impl(operation* op,
        DWORD last_error, size_t bytes_transferred)
    {
      // Take ownership of the operation object.
      typedef receive_from_operation<Endpoint, Handler> op_type;
      op_type* handler_op(static_cast<op_type*>(op));
      typedef handler_alloc_traits<Handler, op_type> alloc_traits;
      handler_ptr<alloc_traits> ptr(handler_op->handler_, handler_op);

      // Check for connection closed.
      if (last_error == 0 && bytes_transferred == 0)
      {
        last_error = asio::error::eof;
      }

      // Record the size of the endpoint returned by the operation.
      handler_op->endpoint_.resize(handler_op->endpoint_size_);

      // Make a copy of the handler so that the memory can be deallocated before
      // the upcall is made.
      Handler handler(handler_op->handler_);

      // Free the memory associated with the handler.
      ptr.reset();

      // Call the handler.
      asio::error error(last_error);
      handler(error, bytes_transferred);
    }

    Endpoint& endpoint_;
    int endpoint_size_;
    asio::io_service::work work_;
    Handler handler_;
  };

  // Start an asynchronous receive. The buffer for the data being received and
  // the sender_endpoint object must both be valid for the lifetime of the
  // asynchronous operation.
  template <typename Mutable_Buffers, typename Endpoint, typename Handler>
  void async_receive_from(implementation_type& impl,
      const Mutable_Buffers& buffers, Endpoint& sender_endp,
      socket_base::message_flags flags, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef receive_from_operation<Endpoint, Handler> value_type;
    typedef handler_alloc_traits<Handler, value_type> alloc_traits;
    raw_handler_ptr<alloc_traits> raw_ptr(handler);
    handler_ptr<alloc_traits> ptr(raw_ptr, owner(), sender_endp, handler);

    // Copy buffers into WSABUF array.
    ::WSABUF bufs[max_buffers];
    typename Mutable_Buffers::const_iterator iter = buffers.begin();
    typename Mutable_Buffers::const_iterator end = buffers.end();
    DWORD i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
    {
      asio::mutable_buffer buffer(*iter);
      bufs[i].len = static_cast<u_long>(asio::buffer_size(buffer));
      bufs[i].buf = asio::buffer_cast<char*>(buffer);
    }

    // Receive some data.
    DWORD bytes_transferred = 0;
    DWORD recv_flags = flags;
    int result = ::WSARecvFrom(impl.socket_, bufs, i, &bytes_transferred,
        &recv_flags, sender_endp.data(), &ptr.get()->endpoint_size(),
        ptr.get(), 0);
    DWORD last_error = ::WSAGetLastError();
    if (result != 0 && last_error != WSA_IO_PENDING)
    {
      ptr.reset();
      asio::error error(last_error);
      iocp_service_.post(bind_handler(handler, error, bytes_transferred));
    }
    else
    {
      ptr.release();
    }
  }

  // Accept a new connection.
  template <typename Socket, typename Error_Handler>
  void accept(implementation_type& impl, Socket& peer,
      Error_Handler error_handler)
  {
    // We cannot accept a socket that is already open.
    if (peer.native() != invalid_socket)
    {
      error_handler(asio::error(asio::error::already_connected));
      return;
    }

    socket_holder new_socket(socket_ops::accept(impl.socket_, 0, 0));
    if (int err = socket_ops::get_error())
    {
      error_handler(asio::error(err));
      return;
    }

    asio::error temp_error;
    peer.open(new_socket.get(), asio::assign_error(temp_error));
    if (temp_error)
      error_handler(temp_error);
    else
      new_socket.release();
  }

  // Accept a new connection.
  template <typename Socket, typename Endpoint, typename Error_Handler>
  void accept_endpoint(implementation_type& impl, Socket& peer,
      Endpoint& peer_endpoint, Error_Handler error_handler)
  {
    // We cannot accept a socket that is already open.
    if (peer.native() != invalid_socket)
    {
      error_handler(asio::error(asio::error::already_connected));
      return;
    }

    socket_addr_len_type addr_len = peer_endpoint.capacity();
    socket_holder new_socket(socket_ops::accept(
          impl.socket_, peer_endpoint.data(), &addr_len));
    if (int err = socket_ops::get_error())
    {
      error_handler(asio::error(err));
      return;
    }

    peer_endpoint.resize(addr_len);

    asio::error temp_error;
    peer.open(new_socket.get(), asio::assign_error(temp_error));
    if (temp_error)
      error_handler(temp_error);
    else
      new_socket.release();
  }

  template <typename Socket, typename Handler>
  class accept_operation
    : public operation
  {
  public:
    accept_operation(asio::io_service& io_service, socket_type socket,
        socket_type new_socket, Socket& peer, Handler handler)
      : operation(
          &accept_operation<Socket, Handler>::do_completion_impl),
        io_service_(io_service),
        socket_(socket),
        new_socket_(new_socket),
        peer_(peer),
        work_(io_service),
        handler_(handler)
    {
    }

    socket_type new_socket()
    {
      return new_socket_.get();
    }

    void* output_buffer()
    {
      return output_buffer_;
    }

    DWORD address_length()
    {
      return sizeof(sockaddr_storage) + 16;
    }

  private:
    static void do_completion_impl(operation* op,
        DWORD last_error, size_t bytes_transferred)
    {
      // Take ownership of the operation object.
      typedef accept_operation<Socket, Handler> op_type;
      op_type* handler_op(static_cast<op_type*>(op));
      typedef handler_alloc_traits<Handler, op_type> alloc_traits;
      handler_ptr<alloc_traits> ptr(handler_op->handler_, handler_op);

      // Check for connection aborted.
      if (last_error == ERROR_NETNAME_DELETED)
      {
        last_error = asio::error::connection_aborted;
      }

      // Need to set the SO_UPDATE_ACCEPT_CONTEXT option so that getsockname
      // and getpeername will work on the accepted socket.
      if (last_error == 0)
      {
        DWORD update_ctx_param = handler_op->socket_;
        if (socket_ops::setsockopt(handler_op->new_socket_.get(), SOL_SOCKET,
              SO_UPDATE_ACCEPT_CONTEXT, &update_ctx_param, sizeof(DWORD)) != 0)
        {
          last_error = socket_ops::get_error();
        }
      }

      // If the socket was successfully accepted, transfer ownership of the
      // socket to the peer object.
      if (last_error == 0)
      {
        asio::error temp_error;
        handler_op->peer_.open(handler_op->new_socket_.get(),
            asio::assign_error(temp_error));
        if (temp_error)
          last_error = temp_error.code();
        else
          handler_op->new_socket_.release();
      }

      // Make a copy of the handler so that the memory can be deallocated before
      // the upcall is made.
      Handler handler(handler_op->handler_);

      // Free the memory associated with the handler.
      ptr.reset();

      // Call the handler.
      asio::error error(last_error);
      handler(error);
    }

    asio::io_service& io_service_;
    socket_type socket_;
    socket_holder new_socket_;
    Socket& peer_;
    asio::io_service::work work_;
    unsigned char output_buffer_[(sizeof(sockaddr_storage) + 16) * 2];
    Handler handler_;
  };

  // Start an asynchronous accept. The peer object must be valid until the
  // accept's handler is invoked.
  template <typename Socket, typename Handler>
  void async_accept(implementation_type& impl, Socket& peer, Handler handler)
  {
    // Check whether acceptor has been initialised.
    if (impl.socket_ == invalid_socket)
    {
      asio::error error(asio::error::bad_descriptor);
      owner().post(bind_handler(handler, error));
      return;
    }

    // Check that peer socket has not already been connected.
    if (peer.native() != invalid_socket)
    {
      asio::error error(asio::error::already_connected);
      owner().post(bind_handler(handler, error));
      return;
    }

    // Get information about the protocol used by the socket.
    WSAPROTOCOL_INFO protocol_info;
    std::size_t protocol_info_size = sizeof(protocol_info);
    if (socket_ops::getsockopt(impl.socket_, SOL_SOCKET, SO_PROTOCOL_INFO,
          &protocol_info, &protocol_info_size) != 0)
    {
      asio::error error(socket_ops::get_error());
      owner().post(bind_handler(handler, error));
      return;
    }

    // Create a new socket for the connection.
    socket_holder sock(socket_ops::socket(protocol_info.iAddressFamily,
          protocol_info.iSocketType, protocol_info.iProtocol));
    if (sock.get() == invalid_socket)
    {
      asio::error error(socket_ops::get_error());
      owner().post(bind_handler(handler, error));
      return;
    }

    // Allocate and construct an operation to wrap the handler.
    typedef accept_operation<Socket, Handler> value_type;
    typedef handler_alloc_traits<Handler, value_type> alloc_traits;
    raw_handler_ptr<alloc_traits> raw_ptr(handler);
    socket_type new_socket = sock.get();
    handler_ptr<alloc_traits> ptr(raw_ptr,
        owner(), impl.socket_, new_socket, peer, handler);
    sock.release();

    // Accept a connection.
    DWORD bytes_read = 0;
    BOOL result = ::AcceptEx(impl.socket_, ptr.get()->new_socket(),
        ptr.get()->output_buffer(), 0, ptr.get()->address_length(),
        ptr.get()->address_length(), &bytes_read, ptr.get());
    DWORD last_error = ::WSAGetLastError();

    // Check if the operation completed immediately.
    if (!result && last_error != WSA_IO_PENDING)
    {
      ptr.reset();
      asio::error error(last_error);
      iocp_service_.post(bind_handler(handler, error));
    }
    else
    {
      ptr.release();
    }
  }

  template <typename Socket, typename Endpoint, typename Handler>
  class accept_endp_operation
    : public operation
  {
  public:
    accept_endp_operation(asio::io_service& io_service, socket_type socket,
        socket_type new_socket, Socket& peer, Endpoint& peer_endpoint,
        Handler handler)
      : operation(&accept_endp_operation<
            Socket, Endpoint, Handler>::do_completion_impl),
        io_service_(io_service),
        socket_(socket),
        new_socket_(new_socket),
        peer_(peer),
        peer_endpoint_(peer_endpoint),
        work_(io_service),
        handler_(handler)
    {
    }

    socket_type new_socket()
    {
      return new_socket_.get();
    }

    void* output_buffer()
    {
      return output_buffer_;
    }

    DWORD address_length()
    {
      return sizeof(sockaddr_storage) + 16;
    }

  private:
    static void do_completion_impl(operation* op,
        DWORD last_error, size_t bytes_transferred)
    {
      // Take ownership of the operation object.
      typedef accept_endp_operation<Socket, Endpoint, Handler> op_type;
      op_type* handler_op(static_cast<op_type*>(op));
      typedef handler_alloc_traits<Handler, op_type> alloc_traits;
      handler_ptr<alloc_traits> ptr(handler_op->handler_, handler_op);

      // Check for connection aborted.
      if (last_error == ERROR_NETNAME_DELETED)
      {
        last_error = asio::error::connection_aborted;
      }

      // Get the address of the peer.
      if (last_error == 0)
      {
        LPSOCKADDR local_addr = 0;
        int local_addr_length = 0;
        LPSOCKADDR remote_addr = 0;
        int remote_addr_length = 0;
        GetAcceptExSockaddrs(handler_op->output_buffer(), 0,
            handler_op->address_length(), handler_op->address_length(),
            &local_addr, &local_addr_length, &remote_addr, &remote_addr_length);
        if (remote_addr_length > handler_op->peer_endpoint_.capacity())
        {
          last_error = asio::error::invalid_argument;
        }
        else
        {
          handler_op->peer_endpoint_.resize(remote_addr_length);
          using namespace std; // For memcpy.
          memcpy(handler_op->peer_endpoint_.data(),
              remote_addr, remote_addr_length);
        }
      }

      // Need to set the SO_UPDATE_ACCEPT_CONTEXT option so that getsockname
      // and getpeername will work on the accepted socket.
      if (last_error == 0)
      {
        DWORD update_ctx_param = handler_op->socket_;
        if (socket_ops::setsockopt(handler_op->new_socket_.get(), SOL_SOCKET,
              SO_UPDATE_ACCEPT_CONTEXT, &update_ctx_param, sizeof(DWORD)) != 0)
        {
          last_error = socket_ops::get_error();
        }
      }

      // If the socket was successfully accepted, transfer ownership of the
      // socket to the peer object.
      if (last_error == 0)
      {
        asio::error temp_error;
        handler_op->peer_.open(handler_op->new_socket_.get(),
            asio::assign_error(temp_error));
        if (temp_error)
          last_error = temp_error.code();
        else
          handler_op->new_socket_.release();
      }

      // Make a copy of the handler so that the memory can be deallocated before
      // the upcall is made.
      Handler handler(handler_op->handler_);

      // Free the memory associated with the handler.
      ptr.reset();

      // Call the handler.
      asio::error error(last_error);
      handler(error);
    }

    asio::io_service& io_service_;
    socket_type socket_;
    socket_holder new_socket_;
    Socket& peer_;
    Endpoint& peer_endpoint_;
    asio::io_service::work work_;
    unsigned char output_buffer_[(sizeof(sockaddr_storage) + 16) * 2];
    Handler handler_;
  };

  // Start an asynchronous accept. The peer and peer_endpoint objects
  // must be valid until the accept's handler is invoked.
  template <typename Socket, typename Endpoint, typename Handler>
  void async_accept_endpoint(implementation_type& impl, Socket& peer,
      Endpoint& peer_endpoint, Handler handler)
  {
    // Check whether acceptor has been initialised.
    if (impl.socket_ == invalid_socket)
    {
      asio::error error(asio::error::bad_descriptor);
      owner().post(bind_handler(handler, error));
      return;
    }

    // Check that peer socket has not already been connected.
    if (peer.native() != invalid_socket)
    {
      asio::error error(asio::error::already_connected);
      owner().post(bind_handler(handler, error));
      return;
    }

    // Get information about the protocol used by the socket.
    WSAPROTOCOL_INFO protocol_info;
    std::size_t protocol_info_size = sizeof(protocol_info);
    if (socket_ops::getsockopt(impl.socket_, SOL_SOCKET, SO_PROTOCOL_INFO,
          &protocol_info, &protocol_info_size) != 0)
    {
      asio::error error(socket_ops::get_error());
      owner().post(bind_handler(handler, error));
      return;
    }

    // Create a new socket for the connection.
    socket_holder sock(socket_ops::socket(protocol_info.iAddressFamily,
          protocol_info.iSocketType, protocol_info.iProtocol));
    if (sock.get() == invalid_socket)
    {
      asio::error error(socket_ops::get_error());
      owner().post(bind_handler(handler, error));
      return;
    }

    // Allocate and construct an operation to wrap the handler.
    typedef accept_endp_operation<Socket, Endpoint, Handler> value_type;
    typedef handler_alloc_traits<Handler, value_type> alloc_traits;
    raw_handler_ptr<alloc_traits> raw_ptr(handler);
    socket_type new_socket = sock.get();
    handler_ptr<alloc_traits> ptr(raw_ptr,
        owner(), impl.socket_, new_socket, peer, peer_endpoint, handler);
    sock.release();

    // Accept a connection.
    DWORD bytes_read = 0;
    BOOL result = ::AcceptEx(impl.socket_, ptr.get()->new_socket(),
        ptr.get()->output_buffer(), 0, ptr.get()->address_length(),
        ptr.get()->address_length(), &bytes_read, ptr.get());
    DWORD last_error = ::WSAGetLastError();

    // Check if the operation completed immediately.
    if (!result && last_error != WSA_IO_PENDING)
    {
      ptr.reset();
      asio::error error(last_error);
      iocp_service_.post(bind_handler(handler, error));
    }
    else
    {
      ptr.release();
    }
  }

  // Connect the socket to the specified endpoint.
  template <typename Endpoint, typename Error_Handler>
  void connect(implementation_type& impl, const Endpoint& peer_endpoint,
      Error_Handler error_handler)
  {
    // Open the socket if it is not already open.
    if (impl.socket_ == invalid_socket)
    {
      // Get the flags used to create the new socket.
      int family = peer_endpoint.protocol().family();
      int type = peer_endpoint.protocol().type();
      int proto = peer_endpoint.protocol().protocol();

      // Create a new socket.
      impl.socket_ = socket_ops::socket(family, type, proto);
      if (impl.socket_ == invalid_socket)
      {
        error_handler(asio::error(socket_ops::get_error()));
        return;
      }
      iocp_service_.register_socket(impl.socket_);
    }

    // Perform the connect operation.
    int result = socket_ops::connect(impl.socket_,
        peer_endpoint.data(), peer_endpoint.size());
    if (result == socket_error_retval)
      error_handler(asio::error(socket_ops::get_error()));
  }

  template <typename Handler>
  class connect_handler
  {
  public:
    connect_handler(socket_type socket, boost::shared_ptr<bool> completed,
        asio::io_service& io_service, reactor_type& reactor, Handler handler)
      : socket_(socket),
        completed_(completed),
        io_service_(io_service),
        reactor_(reactor),
        work_(io_service),
        handler_(handler)
    {
    }

    bool operator()(int result)
    {
      // Check whether a handler has already been called for the connection.
      // If it has, then we don't want to do anything in this handler.
      if (*completed_)
        return true;

      // Cancel the other reactor operation for the connection.
      *completed_ = true;
      reactor_.enqueue_cancel_ops_unlocked(socket_);

      // Check whether the operation was successful.
      if (result != 0)
      {
        asio::error error(result);
        io_service_.post(bind_handler(handler_, error));
        return true;
      }

      // Get the error code from the connect operation.
      int connect_error = 0;
      size_t connect_error_len = sizeof(connect_error);
      if (socket_ops::getsockopt(socket_, SOL_SOCKET, SO_ERROR,
            &connect_error, &connect_error_len) == socket_error_retval)
      {
        asio::error error(socket_ops::get_error());
        io_service_.post(bind_handler(handler_, error));
        return true;
      }

      // If connection failed then post the handler with the error code.
      if (connect_error)
      {
        asio::error error(connect_error);
        io_service_.post(bind_handler(handler_, error));
        return true;
      }

      // Make the socket blocking again (the default).
      ioctl_arg_type non_blocking = 0;
      if (socket_ops::ioctl(socket_, FIONBIO, &non_blocking))
      {
        asio::error error(socket_ops::get_error());
        io_service_.post(bind_handler(handler_, error));
        return true;
      }

      // Post the result of the successful connection operation.
      asio::error error(asio::error::success);
      io_service_.post(bind_handler(handler_, error));
      return true;
    }

  private:
    socket_type socket_;
    boost::shared_ptr<bool> completed_;
    asio::io_service& io_service_;
    reactor_type& reactor_;
    asio::io_service::work work_;
    Handler handler_;
  };

  // Start an asynchronous connect.
  template <typename Endpoint, typename Handler>
  void async_connect(implementation_type& impl, const Endpoint& peer_endpoint,
      Handler handler)
  {
    // Check if the reactor was already obtained from the io_service.
    reactor_type* reactor = static_cast<reactor_type*>(
          InterlockedCompareExchangePointer(
          reinterpret_cast<void**>(&reactor_), 0, 0));
    if (!reactor)
    {
      reactor = &(asio::use_service<reactor_type>(owner()));
      InterlockedExchangePointer(reinterpret_cast<void**>(&reactor_), reactor);
    }

    // Open the socket if it is not already open.
    if (impl.socket_ == invalid_socket)
    {
      // Get the flags used to create the new socket.
      int family = peer_endpoint.protocol().family();
      int type = peer_endpoint.protocol().type();
      int proto = peer_endpoint.protocol().protocol();

      // Create a new socket.
      impl.socket_ = socket_ops::socket(family, type, proto);
      if (impl.socket_ == invalid_socket)
      {
        asio::error error(socket_ops::get_error());
        owner().post(bind_handler(handler, error));
        return;
      }
      iocp_service_.register_socket(impl.socket_);
    }

    // Mark the socket as non-blocking so that the connection will take place
    // asynchronously.
    ioctl_arg_type non_blocking = 1;
    if (socket_ops::ioctl(impl.socket_, FIONBIO, &non_blocking))
    {
      asio::error error(socket_ops::get_error());
      owner().post(bind_handler(handler, error));
      return;
    }

    // Start the connect operation.
    if (socket_ops::connect(impl.socket_, peer_endpoint.data(),
          peer_endpoint.size()) == 0)
    {
      // The connect operation has finished successfully so we need to post the
      // handler immediately.
      asio::error error(asio::error::success);
      owner().post(bind_handler(handler, error));
    }
    else if (socket_ops::get_error() == asio::error::in_progress
        || socket_ops::get_error() == asio::error::would_block)
    {
      // The connection is happening in the background, and we need to wait
      // until the socket becomes writeable.
      boost::shared_ptr<bool> completed(new bool(false));
      reactor->start_write_and_except_ops(impl.socket_,
          connect_handler<Handler>(
            impl.socket_, completed, owner(), *reactor, handler));
    }
    else
    {
      // The connect operation has failed, so post the handler immediately.
      asio::error error(socket_ops::get_error());
      owner().post(bind_handler(handler, error));
    }
  }

private:
  // The IOCP service used for running asynchronous operations and dispatching
  // handlers.
  win_iocp_io_service& iocp_service_;

  // The reactor used for performing connect operations. This object is created
  // only if needed.
  reactor_type* reactor_;
};

} // namespace detail
} // namespace asio

#endif // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400)
#endif // defined(BOOST_WINDOWS)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_WIN_IOCP_SOCKET_SERVICE_HPP
