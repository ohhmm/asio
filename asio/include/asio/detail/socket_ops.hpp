//
// socket_ops.hpp
// ~~~~~~~~~~~~~~
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

#ifndef ASIO_DETAIL_SOCKET_OPS_HPP
#define ASIO_DETAIL_SOCKET_OPS_HPP

#include "asio/detail/push_options.hpp"

#include "asio/detail/push_options.hpp"
#include <cstring>
#include <cerrno>
#if !defined(_WIN32)
#include <sys/ioctl.h>
#endif
#include "asio/detail/pop_options.hpp"

#include "asio/detail/socket_types.hpp"

namespace asio {
namespace detail {
namespace socket_ops {

inline int get_error()
{
#if defined(_WIN32)
  return WSAGetLastError();
#else // defined(_WIN32)
  return errno;
#endif // defined(_WIN32)
}

inline void set_error(int error)
{
  errno = error;
#if defined(_WIN32)
  WSASetLastError(error);
#endif // defined(_WIN32)
}

template <typename ReturnType>
inline ReturnType error_wrapper(ReturnType return_value)
{
#if defined(_WIN32)
  errno = WSAGetLastError();
#endif // defined(_WIN32)
  return return_value;
}

inline socket_type accept(socket_type s, socket_addr_type* addr,
    socket_addr_len_type* addrlen)
{
  set_error(0);
  return error_wrapper(::accept(s, addr, addrlen));
}

inline int bind(socket_type s, const socket_addr_type* addr,
    socket_addr_len_type addrlen)
{
  set_error(0);
  return error_wrapper(::bind(s, addr, addrlen));
}

inline int close(socket_type s)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::closesocket(s));
#else // defined(_WIN32)
  return error_wrapper(::close(s));
#endif // defined(_WIN32)
}

inline int shutdown(socket_type s, int what)
{
  set_error(0);
  return error_wrapper(::shutdown(s, what));
}

inline int connect(socket_type s, const socket_addr_type* addr,
    socket_addr_len_type addrlen)
{
  set_error(0);
  return error_wrapper(::connect(s, addr, addrlen));
}

inline int listen(socket_type s, int backlog)
{
  set_error(0);
  return error_wrapper(::listen(s, backlog));
}

inline int recv(socket_type s, void* buf, size_t len, int flags)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::recv(s, static_cast<char*>(buf), len, flags));
#else // defined(_WIN32)
  return error_wrapper(::recv(s, buf, len, flags));
#endif // defined(_WIN32)
}

inline int recvfrom(socket_type s, void* buf, size_t len, int flags,
    socket_addr_type* addr, socket_addr_len_type* addrlen)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::recvfrom(s, static_cast<char*>(buf), len, flags, addr,
        addrlen));
#else // defined(_WIN32)
  return error_wrapper(::recvfrom(s, buf, len, flags, addr, addrlen));
#endif // defined(_WIN32)
}

inline int send(socket_type s, const void* buf, size_t len, int flags)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::send(s, static_cast<const char*>(buf), len, flags));
#else // defined(_WIN32)
  return error_wrapper(::send(s, buf, len, flags));
#endif // defined(_WIN32)
}

inline int sendto(socket_type s, const void* buf, size_t len, int flags,
    const socket_addr_type* addr, socket_addr_len_type addrlen)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::sendto(s, static_cast<const char*>(buf), len, flags,
        addr, addrlen));
#else // defined(_WIN32)
  return error_wrapper(::sendto(s, buf, len, flags, addr, addrlen));
#endif // defined(_WIN32)
}

inline socket_type socket(int af, int type, int protocol)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::WSASocket(af, type, protocol, 0, 0,
        WSA_FLAG_OVERLAPPED));
#else // defined(_WIN32)
  return error_wrapper(::socket(af, type, protocol));
#endif // defined(_WIN32)
}

inline int setsockopt(socket_type s, int level, int optname,
    const void* optval, socket_len_type optlen)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::setsockopt(s, level, optname,
        reinterpret_cast<const char*>(optval), optlen));
#else // defined(_WIN32)
  return error_wrapper(::setsockopt(s, level, optname, optval, optlen));
#endif // defined(_WIN32)
}

inline int getsockopt(socket_type s, int level, int optname, void* optval,
    socket_len_type* optlen)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::getsockopt(s, level, optname,
        reinterpret_cast<char*>(optval), optlen));
#else // defined(_WIN32)
  return error_wrapper(::getsockopt(s, level, optname, optval, optlen));
#endif // defined(_WIN32)
}

inline int getpeername(socket_type s, socket_addr_type* addr,
    socket_addr_len_type* addrlen)
{
  set_error(0);
  return error_wrapper(::getpeername(s, addr, addrlen));
}

inline int getsockname(socket_type s, socket_addr_type* addr,
    socket_addr_len_type* addrlen)
{
  set_error(0);
  return error_wrapper(::getsockname(s, addr, addrlen));
}

inline int ioctl(socket_type s, long cmd, ioctl_arg_type* arg)
{
  set_error(0);
#if defined(_WIN32)
  return error_wrapper(::ioctlsocket(s, cmd, arg));
#else // defined(_WIN32)
  return error_wrapper(::ioctl(s, cmd, arg));
#endif // defined(_WIN32)
}

inline int select(int nfds, fd_set* readfds, fd_set* writefds,
    fd_set* exceptfds, timeval* timeout)
{
  set_error(0);
#if defined(_WIN32)
  if (!readfds && !writefds && !exceptfds && timeout)
  {
    ::Sleep(timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
    return 0;
  }
#endif // defined(_WIN32)
  return error_wrapper(::select(nfds, readfds, writefds, exceptfds, timeout));
}

inline const char* inet_ntop(int af, const void* src, char* dest,
    size_t length)
{
  set_error(0);
#if defined(_WIN32)
  using namespace std; // For strncat.

  if (af != AF_INET)
  {
    set_error(socket_error::address_family_not_supported);
    return 0;
  }

  char* addr_str = error_wrapper(
      ::inet_ntoa(*static_cast<const in_addr*>(src)));
  if (addr_str)
  {
    *dest = '\0';
    strncat(dest, addr_str, length);
    return dest;
  }

  // Windows may not set an error code on failure.
  if (get_error() == 0)
    set_error(socket_error::invalid_argument);

  return 0;

#else // defined(_WIN32)
  const char* result = error_wrapper(::inet_ntop(af, src, dest, length));
  if (result == 0 && get_error() == 0)
    set_error(socket_error::invalid_argument);
  return result;
#endif // defined(_WIN32)
}

inline int inet_pton(int af, const char* src, void* dest)
{
  set_error(0);
#if defined(_WIN32)
  using namespace std; // For strcmp.

  if (af != AF_INET)
  {
    set_error(socket_error::address_family_not_supported);
    return -1;
  }

  unsigned long addr = error_wrapper(::inet_addr(src));
  if (addr != INADDR_NONE || strcmp(src, "255.255.255.255") == 0)
  {
    static_cast<in_addr*>(dest)->s_addr = addr;
    return 1;
  }

  // Windows may not set an error code on failure.
  if (get_error() == 0)
    set_error(socket_error::invalid_argument);

  return 0;
#else // defined(_WIN32)
  int result = error_wrapper(::inet_pton(af, src, dest));
  if (result <= 0 && get_error() == 0)
    set_error(socket_error::invalid_argument);
  return result;
#endif // defined(_WIN32)
}

inline int gethostname(char* name, int namelen)
{
  set_error(0);
  return error_wrapper(::gethostname(name, namelen));
}

inline int translate_netdb_error(int error)
{
  switch (error)
  {
  case 0:
    return socket_error::success;
  case HOST_NOT_FOUND:
    return socket_error::host_not_found;
  case TRY_AGAIN:
    return socket_error::host_not_found_try_again;
  case NO_RECOVERY:
    return socket_error::no_recovery;
  case NO_DATA:
    return socket_error::no_host_data;
  default:
    return get_error();
  }
}

inline hostent* gethostbyaddr_r(const char* addr, int length, int type,
    hostent* result, char* buffer, int buflength, int* error)
{
  set_error(0);
#if defined(_WIN32)
  hostent* ent_result = error_wrapper(::gethostbyaddr(addr, length, type));
  *error = get_error();
  if (!ent_result)
    return 0;
  *result = *ent_result;
  return result;
#elif defined(__sun)
  hostent* retval = error_wrapper(::gethostbyaddr_r(addr, length, type, result,
        buffer, buflength, error));
  *error = translate_netdb_error(*error);
  return retval;
#else
  hostent* ent_result = 0;
  error_wrapper(::gethostbyaddr_r(addr, length, type, result, buffer,
        buflength, &ent_result, error));
  *error = translate_netdb_error(*error);
  return ent_result;
#endif
}

inline hostent* gethostbyname_r(const char* name, struct hostent* result,
    char* buffer, int buflength, int* error)
{
  set_error(0);
#if defined(_WIN32)
  hostent* ent_result = error_wrapper(::gethostbyname(name));
  *error = get_error();
  if (!ent_result)
    return 0;
  *result = *ent_result;
  return result;
#elif defined(__sun)
  hostent* retval = error_wrapper(::gethostbyname_r(name, result, buffer,
        buflength, error));
  *error = translate_netdb_error(*error);
  return retval;
#else
  hostent* ent_result = 0;
  error_wrapper(::gethostbyname_r(name, result, buffer, buflength, &ent_result,
        error));
  *error = translate_netdb_error(*error);
  return ent_result;
#endif
}

} // namespace socket_ops
} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_SOCKET_OPS_HPP
