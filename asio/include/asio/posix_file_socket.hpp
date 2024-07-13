//
// posix_file_socket.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_POSIX_FILE_SOCKET_HPP
#define ASIO_POSIX_FILE_SOCKET_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_FILE) && defined(ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

#include "asio/basic_stream_socket.hpp"
#include "asio/detail/push_options.hpp"
#include "asio/posix/basic_stream_file.hpp"
#include <string>

namespace asio {

/// A POSIX file socket.
class posix_file_socket : public basic_stream_socket<posix::stream_file>
{
public:
  /// Construct a posix_file_socket without opening it.
  explicit posix_file_socket(asio::io_context& io_context)
    : basic_stream_socket<posix::stream_file>(io_context)
  {
  }

  /// Destructor.
  ~posix_file_socket();

  /// Open a file.
  void open(const std::string& path);

  /// Open a file.
  ASIO_SYNC_OP_VOID open(const std::string& path,
      asio::error_code& ec);

  /// Close the file.
  void close();

  /// Close the file.
  ASIO_SYNC_OP_VOID close(asio::error_code& ec);
};

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_FILE) && defined(ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

#endif // ASIO_POSIX_FILE_SOCKET_HPP