//
// ssl/stream.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SSL_STREAM_HPP
#define ASIO_SSL_STREAM_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_ENABLE_OLD_SSL)
# include "asio/ssl/old/stream.hpp"
#else // defined(ASIO_ENABLE_OLD_SSL)
# include "asio/detail/buffer_sequence_adapter.hpp"
# include "asio/detail/noncopyable.hpp"
# include "asio/ssl/context.hpp"
# include "asio/ssl/detail/buffer_space.hpp"
# include "asio/ssl/detail/engine.hpp"
# include "asio/ssl/detail/handshake_op.hpp"
# include "asio/ssl/detail/read_op.hpp"
# include "asio/ssl/detail/shutdown_op.hpp"
# include "asio/ssl/detail/transport.hpp"
# include "asio/ssl/detail/write_op.hpp"
# include "asio/ssl/stream_base.hpp"
# include <boost/type_traits/remove_reference.hpp>
#endif // defined(ASIO_ENABLE_OLD_SSL)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {

#if defined(ASIO_ENABLE_OLD_SSL)

using asio::ssl::old::stream;

#else // defined(ASIO_ENABLE_OLD_SSL)

/// Provides stream-oriented functionality using SSL.
/**
 * The stream class template provides asynchronous and blocking stream-oriented
 * functionality using SSL.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe. The application must also ensure that all
 * asynchronous operations are performed within the same implicit or explicit
 * strand.
 *
 * @par Example
 * To use the SSL stream template with an ip::tcp::socket, you would write:
 * @code
 * asio::io_service io_service;
 * asio::ssl::context context(io_service, asio::ssl::context::sslv23);
 * asio::ssl::stream<asio::ip::tcp::socket> sock(io_service, context);
 * @endcode
 *
 * @par Concepts:
 * AsyncReadStream, AsyncWriteStream, Stream, SyncReadStream, SyncWriteStream.
 */
template <typename Stream>
class stream :
  public stream_base,
  private noncopyable
{
public:
  /// The native handle type of the SSL stream.
  typedef SSL* native_handle_type;

  /// The type of the next layer.
  typedef typename boost::remove_reference<Stream>::type next_layer_type;

  /// The type of the lowest layer.
  typedef typename next_layer_type::lowest_layer_type lowest_layer_type;

  /// Construct a stream.
  /**
   * This constructor creates a stream and initialises the underlying stream
   * object.
   *
   * @param arg The argument to be passed to initialise the underlying stream.
   *
   * @param context The SSL context to be used for the stream.
   */
  template <typename Arg>
  stream(Arg& arg, context& ctx)
    : engine_(ctx.native_handle()),
      transport_(arg)
  {
  }

  /// Destructor.
  ~stream()
  {
  }

  /// Get the io_service associated with the object.
  /**
   * This function may be used to obtain the io_service object that the stream
   * uses to dispatch handlers for asynchronous operations.
   *
   * @return A reference to the io_service object that stream will use to
   * dispatch handlers. Ownership is not transferred to the caller.
   */
  asio::io_service& get_io_service()
  {
    return transport_.get_io_service();
  }

  /// Get the underlying implementation in the native type.
  /**
   * This function may be used to obtain the underlying implementation of the
   * context. This is intended to allow access to context functionality that is
   * not otherwise provided.
   */
  native_handle_type native_handle()
  {
    return engine_.native_handle();
  }

  /// Get a reference to the next layer.
  /**
   * This function returns a reference to the next layer in a stack of stream
   * layers.
   *
   * @return A reference to the next layer in the stack of stream layers.
   * Ownership is not transferred to the caller.
   */
  const next_layer_type& next_layer() const
  {
    return transport_.next_layer();
  }

  /// Get a reference to the next layer.
  /**
   * This function returns a reference to the next layer in a stack of stream
   * layers.
   *
   * @return A reference to the next layer in the stack of stream layers.
   * Ownership is not transferred to the caller.
   */
  next_layer_type& next_layer()
  {
    return transport_.next_layer();
  }

  /// Get a reference to the lowest layer.
  /**
   * This function returns a reference to the lowest layer in a stack of
   * stream layers.
   *
   * @return A reference to the lowest layer in the stack of stream layers.
   * Ownership is not transferred to the caller.
   */
  lowest_layer_type& lowest_layer()
  {
    return transport_.lowest_layer();
  }

  /// Get a reference to the lowest layer.
  /**
   * This function returns a reference to the lowest layer in a stack of
   * stream layers.
   *
   * @return A reference to the lowest layer in the stack of stream layers.
   * Ownership is not transferred to the caller.
   */
  const lowest_layer_type& lowest_layer() const
  {
    return transport_.lowest_layer();
  }

  /// Perform SSL handshaking.
  /**
   * This function is used to perform SSL handshaking on the stream. The
   * function call will block until handshaking is complete or an error occurs.
   *
   * @param type The type of handshaking to be performed, i.e. as a client or as
   * a server.
   *
   * @throws asio::system_error Thrown on failure.
   */
  void handshake(handshake_type type)
  {
    asio::error_code ec;
    handshake(type, ec);
    asio::detail::throw_error(ec, "handshake");
  }

  /// Perform SSL handshaking.
  /**
   * This function is used to perform SSL handshaking on the stream. The
   * function call will block until handshaking is complete or an error occurs.
   *
   * @param type The type of handshaking to be performed, i.e. as a client or as
   * a server.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  asio::error_code handshake(handshake_type type,
      asio::error_code& ec)
  {
    for (;;)
    {
      int result = engine_.handshake(type, space_, ec);
      result = transport_.sync(result, space_, ec);
      if (result >= 0)
        return engine_.map_error_code(ec);
    }
  }

  /// Start an asynchronous SSL handshake.
  /**
   * This function is used to asynchronously perform an SSL handshake on the
   * stream. This function call always returns immediately.
   *
   * @param type The type of handshaking to be performed, i.e. as a client or as
   * a server.
   *
   * @param handler The handler to be called when the handshake operation
   * completes. Copies will be made of the handler as required. The equivalent
   * function signature of the handler must be:
   * @code void handler(
   *   const asio::error_code& error // Result of operation.
   * ); @endcode
   */
  template <typename HandshakeHandler>
  void async_handshake(handshake_type type,
      HandshakeHandler handler)
  {
    detail::handshake_op<Stream, HandshakeHandler>(
      engine_, transport_, space_, type, handler)(
        asio::error_code(), 0, 1);
  }

  /// Shut down SSL on the stream.
  /**
   * This function is used to shut down SSL on the stream. The function call
   * will block until SSL has been shut down or an error occurs.
   *
   * @throws asio::system_error Thrown on failure.
   */
  void shutdown()
  {
    asio::error_code ec;
    shutdown(ec);
    asio::detail::throw_error(ec, "shutdown");
  }

  /// Shut down SSL on the stream.
  /**
   * This function is used to shut down SSL on the stream. The function call
   * will block until SSL has been shut down or an error occurs.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  asio::error_code shutdown(asio::error_code& ec)
  {
    for (;;)
    {
      int result = engine_.shutdown(space_, ec);
      result = transport_.sync(result, space_, ec);
      if (result >= 0)
        return engine_.map_error_code(ec);
    }
  }

  /// Asynchronously shut down SSL on the stream.
  /**
   * This function is used to asynchronously shut down SSL on the stream. This
   * function call always returns immediately.
   *
   * @param handler The handler to be called when the handshake operation
   * completes. Copies will be made of the handler as required. The equivalent
   * function signature of the handler must be:
   * @code void handler(
   *   const asio::error_code& error // Result of operation.
   * ); @endcode
   */
  template <typename ShutdownHandler>
  void async_shutdown(ShutdownHandler handler)
  {
    detail::shutdown_op<Stream, ShutdownHandler>(
      engine_, transport_, space_, handler)(
        asio::error_code(), 0, 1);
  }

  /// Write some data to the stream.
  /**
   * This function is used to write data on the stream. The function call will
   * block until one or more bytes of data has been written successfully, or
   * until an error occurs.
   *
   * @param buffers The data to be written.
   *
   * @returns The number of bytes written.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The write_some operation may not transmit all of the data to the
   * peer. Consider using the @ref write function if you need to ensure that all
   * data is written before the blocking operation completes.
   */
  template <typename ConstBufferSequence>
  std::size_t write_some(const ConstBufferSequence& buffers)
  {
    asio::error_code ec;
    std::size_t n = write_some(buffers, ec);
    asio::detail::throw_error(ec, "write_some");
    return n;
  }

  /// Write some data to the stream.
  /**
   * This function is used to write data on the stream. The function call will
   * block until one or more bytes of data has been written successfully, or
   * until an error occurs.
   *
   * @param buffers The data to be written to the stream.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of bytes written. Returns 0 if an error occurred.
   *
   * @note The write_some operation may not transmit all of the data to the
   * peer. Consider using the @ref write function if you need to ensure that all
   * data is written before the blocking operation completes.
   */
  template <typename ConstBufferSequence>
  std::size_t write_some(const ConstBufferSequence& buffers,
      asio::error_code& ec)
  {
    asio::const_buffer buffer =
      asio::detail::buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence>::first(buffers);

    for (;;)
    {
      int result = engine_.write(buffer, space_, ec);
      result = transport_.sync(result, space_, ec);
      if (result >= 0)
      {
        engine_.map_error_code(ec);
        return static_cast<std::size_t>(result);
      }
    }
  }

  /// Start an asynchronous write.
  /**
   * This function is used to asynchronously write one or more bytes of data to
   * the stream. The function call always returns immediately.
   *
   * @param buffers The data to be written to the stream. Although the buffers
   * object may be copied as necessary, ownership of the underlying buffers is
   * retained by the caller, which must guarantee that they remain valid until
   * the handler is called.
   *
   * @param handler The handler to be called when the write operation completes.
   * Copies will be made of the handler as required. The equivalent function
   * signature of the handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred           // Number of bytes written.
   * ); @endcode
   *
   * @note The async_write_some operation may not transmit all of the data to
   * the peer. Consider using the @ref async_write function if you need to
   * ensure that all data is written before the blocking operation completes.
   */
  template <typename ConstBufferSequence, typename WriteHandler>
  void async_write_some(const ConstBufferSequence& buffers,
      WriteHandler handler)
  {
    detail::write_op<Stream, ConstBufferSequence, WriteHandler>(
      engine_, transport_, space_, buffers, handler)(
        asio::error_code(), 0, 1);
  }

  /// Read some data from the stream.
  /**
   * This function is used to read data from the stream. The function call will
   * block until one or more bytes of data has been read successfully, or until
   * an error occurs.
   *
   * @param buffers The buffers into which the data will be read.
   *
   * @returns The number of bytes read.
   *
   * @throws asio::system_error Thrown on failure.
   *
   * @note The read_some operation may not read all of the requested number of
   * bytes. Consider using the @ref read function if you need to ensure that the
   * requested amount of data is read before the blocking operation completes.
   */
  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers)
  {
    asio::error_code ec;
    std::size_t n = read_some(buffers, ec);
    asio::detail::throw_error(ec, "read_some");
    return n;
  }

  /// Read some data from the stream.
  /**
   * This function is used to read data from the stream. The function call will
   * block until one or more bytes of data has been read successfully, or until
   * an error occurs.
   *
   * @param buffers The buffers into which the data will be read.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns The number of bytes read. Returns 0 if an error occurred.
   *
   * @note The read_some operation may not read all of the requested number of
   * bytes. Consider using the @ref read function if you need to ensure that the
   * requested amount of data is read before the blocking operation completes.
   */
  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers,
      asio::error_code& ec)
  {
    asio::mutable_buffer buffer =
      asio::detail::buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence>::first(buffers);

    for (;;)
    {
      int result = engine_.read(buffer, space_, ec);
      result = transport_.sync(result, space_, ec);
      if (result >= 0)
      {
        engine_.map_error_code(ec);
        return static_cast<std::size_t>(result);
      }
    }
  }

  /// Start an asynchronous read.
  /**
   * This function is used to asynchronously read one or more bytes of data from
   * the stream. The function call always returns immediately.
   *
   * @param buffers The buffers into which the data will be read. Although the
   * buffers object may be copied as necessary, ownership of the underlying
   * buffers is retained by the caller, which must guarantee that they remain
   * valid until the handler is called.
   *
   * @param handler The handler to be called when the read operation completes.
   * Copies will be made of the handler as required. The equivalent function
   * signature of the handler must be:
   * @code void handler(
   *   const asio::error_code& error, // Result of operation.
   *   std::size_t bytes_transferred           // Number of bytes read.
   * ); @endcode
   *
   * @note The async_read_some operation may not read all of the requested
   * number of bytes. Consider using the @ref async_read function if you need to
   * ensure that the requested amount of data is read before the asynchronous
   * operation completes.
   */
  template <typename MutableBufferSequence, typename ReadHandler>
  void async_read_some(const MutableBufferSequence& buffers,
      ReadHandler handler)
  {
    detail::read_op<Stream, MutableBufferSequence, ReadHandler>(
      engine_, transport_, space_, buffers, handler)(
        asio::error_code(), 0, 1);
  }

private:
  detail::engine engine_;
  detail::transport<Stream> transport_;
  detail::buffer_space space_;
};

#endif // defined(ASIO_ENABLE_OLD_SSL)

} // namespace ssl
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_SSL_STREAM_HPP
