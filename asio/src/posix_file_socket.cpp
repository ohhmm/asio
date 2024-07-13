#include "asio/posix_file_socket.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/error.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace asio {

posix_file_socket::posix_file_socket(asio::io_context& io_context)
  : basic_stream_socket<asio::posix::stream_file_descriptor>(io_context)
{
}

posix_file_socket::~posix_file_socket()
{
  close();
}

void posix_file_socket::open(const std::string& path)
{
  asio::error_code ec;
  open(path, ec);
  asio::detail::throw_error(ec, "open");
}

ASIO_SYNC_OP_VOID posix_file_socket::open(const std::string& path, asio::error_code& ec)
{
  if (is_open())
  {
    ec = asio::error::already_open;
    ASIO_SYNC_OP_VOID_RETURN(ec);
  }

  int fd = ::open(path.c_str(), O_RDWR);
  if (fd < 0)
  {
    ec = asio::error_code(errno, asio::error::get_system_category());
    ASIO_SYNC_OP_VOID_RETURN(ec);
  }

  assign(asio::posix::stream_file_descriptor(fd));
  ec = asio::error_code();
  ASIO_SYNC_OP_VOID_RETURN(ec);
}

void posix_file_socket::close()
{
  asio::error_code ec;
  close(ec);
}

ASIO_SYNC_OP_VOID posix_file_socket::close(asio::error_code& ec)
{
  if (is_open())
  {
    basic_stream_socket<asio::posix::stream_file_descriptor>::close(ec);
  }
  else
  {
    ec = asio::error_code();
  }
  ASIO_SYNC_OP_VOID_RETURN(ec);
}

} // namespace asio