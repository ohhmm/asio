#include "asio.hpp"
#include "asio/posix_file_socket.hpp"
#include "unit_test.hpp"
#include <cstdio>
#include <string>

ASIO_TEST_SUITE_BEGIN(posix_file_socket_test)

ASIO_TEST_CASE(constructor_test)
{
  asio::io_context ioc;
  asio::posix::file_socket file(ioc);
  ASIO_CHECK(!file.is_open());
}

ASIO_TEST_CASE(open_close_test)
{
  asio::io_context ioc;
  asio::posix::file_socket file(ioc);

  char temp_filename[] = "/tmp/asio_test_XXXXXX";
  int fd = mkstemp(temp_filename);
  ASIO_CHECK(fd != -1);
  close(fd);

  asio::error_code ec;
  file.open(temp_filename, asio::file_base::read_write, ec);
  ASIO_CHECK(!ec);
  ASIO_CHECK(file.is_open());

  file.close(ec);
  ASIO_CHECK(!ec);
  ASIO_CHECK(!file.is_open());

  std::remove(temp_filename);
}

ASIO_TEST_CASE(error_handling_test)
{
  asio::io_context ioc;
  asio::posix::file_socket file(ioc);

  asio::error_code ec;
  file.open("/non/existent/file", asio::file_base::read_only, ec);
  ASIO_CHECK(ec);
  ASIO_CHECK(!file.is_open());
}

ASIO_TEST_CASE(multiple_open_test)
{
  asio::io_context ioc;
  asio::posix::file_socket file(ioc);

  char temp_filename[] = "/tmp/asio_test_XXXXXX";
  int fd = mkstemp(temp_filename);
  ASIO_CHECK(fd != -1);
  close(fd);

  asio::error_code ec;
  file.open(temp_filename, asio::file_base::read_write, ec);
  ASIO_CHECK(!ec);
  ASIO_CHECK(file.is_open());

  // Try to open again
  file.open(temp_filename, asio::file_base::read_write, ec);
  ASIO_CHECK(ec);

  file.close(ec);
  ASIO_CHECK(!ec);

  std::remove(temp_filename);
}

ASIO_TEST_SUITE_END()