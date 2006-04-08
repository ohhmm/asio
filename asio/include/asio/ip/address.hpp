//
// address.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2006 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IP_ADDRESS_HPP
#define ASIO_IP_ADDRESS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/push_options.hpp"

#include "asio/detail/push_options.hpp"
#include <string>
#include <boost/throw_exception.hpp>
#include "asio/detail/pop_options.hpp"

#include "asio/error.hpp"
#include "asio/error_handler.hpp"
#include "asio/ipv4/address.hpp"
#include "asio/ipv6/address.hpp"

namespace asio {
namespace ip {

/// Implements version-independent IP addresses.
/**
 * The asio::ip::address class provides the ability to use either IP version 4
 * or version 6 addresses.
 *
 * @par Thread Safety:
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
class address
{
public:
  /// Default constructor.
  address()
    : type_(ipv4),
      ipv4_address_(),
      ipv6_address_()
  {
  }

  /// Construct an address from an IPv4 address.
  address(const asio::ipv4::address& ipv4_address)
    : type_(ipv4),
      ipv4_address_(ipv4_address),
      ipv6_address_()
  {
  }

  /// Construct an address from an IPv6 address.
  address(const asio::ipv6::address& ipv6_address)
    : type_(ipv6),
      ipv4_address_(),
      ipv6_address_(ipv6_address)
  {
  }

  /// Copy constructor.
  address(const address& other)
    : type_(other.type_),
      ipv4_address_(other.ipv4_address_),
      ipv6_address_(other.ipv6_address_)
  {
  }

  /// Assign from another address.
  address& operator=(const address& other)
  {
    type_ = other.type_;
    ipv4_address_ = other.ipv4_address_;
    ipv6_address_ = other.ipv6_address_;
    return *this;
  }

  /// Assign from an IPv4 address.
  address& operator=(const asio::ipv4::address& ipv4_address)
  {
    type_ = ipv4;
    ipv4_address_ = ipv4_address;
    ipv6_address_ = asio::ipv6::address();
    return *this;
  }

  /// Assign from an IPv6 address.
  address& operator=(const asio::ipv6::address& ipv6_address)
  {
    type_ = ipv6;
    ipv4_address_ = asio::ipv4::address();
    ipv6_address_ = ipv6_address;
    return *this;
  }

  /// Get whether the address is an IP version 4 address.
  bool is_ipv4() const
  {
    return type_ == ipv4;
  }

  /// Get whether the address is an IP version 6 address.
  bool is_ipv6() const
  {
    return type_ == ipv6;
  }

  /// Get the address as an IP version 4 address.
  asio::ipv4::address to_ipv4() const
  {
    if (type_ != ipv4)
    {
      asio::error error(asio::error::address_family_not_supported);
      boost::throw_exception(error);
    }
    return ipv4_address_;
  }

  /// Get the address as an IP version 6 address.
  asio::ipv6::address to_ipv6() const
  {
    if (type_ != ipv6)
    {
      asio::error error(asio::error::address_family_not_supported);
      boost::throw_exception(error);
    }
    return ipv6_address_;
  }

  /// Get the address as a string in dotted decimal format.
  std::string to_string() const
  {
    if (type_ == ipv6)
      return ipv6_address_.to_string();
    return ipv4_address_.to_string();
  }

  /// Get the address as a string in dotted decimal format.
  template <typename Error_Handler>
  std::string to_string(Error_Handler error_handler) const
  {
    if (type_ == ipv6)
      return ipv6_address_.to_string(error_handler);
    return ipv4_address_.to_string(error_handler);
  }

  /// Create an address from an IPv4 address string in dotted decimal form,
  /// or from an IPv6 address in hexadecimal notation.
  static address from_string(const char* str)
  {
    return from_string(str, asio::throw_error());
  }

  /// Create an address from an IPv4 address string in dotted decimal form,
  /// or from an IPv6 address in hexadecimal notation.
  template <typename Error_Handler>
  static address from_string(const char* str, Error_Handler error_handler)
  {
    asio::error error;
    asio::ipv6::address ipv6_address =
      asio::ipv6::address::from_string(str,
        asio::assign_error(error));
    if (!error)
    {
      address tmp;
      tmp.type_ = ipv6;
      tmp.ipv6_address_ = ipv6_address;
      return tmp;
    }

    error = asio::error();
    asio::ipv4::address ipv4_address =
      asio::ipv4::address::from_string(str,
        asio::assign_error(error));
    if (!error)
    {
      address tmp;
      tmp.type_ = ipv4;
      tmp.ipv4_address_ = ipv4_address;
      return tmp;
    }

    error_handler(error);
    return address();
  }

  /// Create an address from an IPv4 address string in dotted decimal form,
  /// or from an IPv6 address in hexadecimal notation.
  static address from_string(const std::string& str)
  {
    return from_string(str.c_str(), asio::throw_error());
  }

  /// Create an address from an IPv4 address string in dotted decimal form,
  /// or from an IPv6 address in hexadecimal notation.
  template <typename Error_Handler>
  static address from_string(const std::string& str,
      Error_Handler error_handler)
  {
    return from_string(str.c_str(), error_handler);
  }

  /// Compare two addresses for equality.
  friend bool operator==(const address& a1, const address& a2)
  {
    if (a1.type_ != a2.type_)
      return false;
    if (a1.type_ == ipv6)
      return a1.ipv6_address_ == a2.ipv6_address_;
    return a1.ipv4_address_ == a2.ipv4_address_;
  }

  /// Compare two addresses for inequality.
  friend bool operator!=(const address& a1, const address& a2)
  {
    if (a1.type_ != a2.type_)
      return true;
    if (a1.type_ == ipv6)
      return a1.ipv6_address_ != a2.ipv6_address_;
    return a1.ipv4_address_ != a2.ipv4_address_;
  }

  /// Compare addresses for ordering.
  friend bool operator<(const address& a1, const address& a2)
  {
    if (a1.type_ < a2.type_)
      return true;
    if (a1.type_ > a2.type_)
      return false;
    if (a1.type_ == ipv6)
      return a1.ipv6_address_ < a2.ipv6_address_;
    return a1.ipv4_address_ < a2.ipv4_address_;
  }

private:
  // The type of the address.
  enum { ipv4, ipv6 } type_;

  // The underlying IPv4 address.
  asio::ipv4::address ipv4_address_;

  // The underlying IPv6 address.
  asio::ipv6::address ipv6_address_;
};

/// Output an address as a string.
/**
 * Used to output a human-readable string for a specified address.
 *
 * @param os The output stream to which the string will be written.
 *
 * @param addr The address to be written.
 *
 * @return The output stream.
 *
 * @relates tcp::endpoint
 */
template <typename Ostream>
Ostream& operator<<(Ostream& os, const address& addr)
{
  os << addr.to_string();
  return os;
}

} // namespace ip
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IP_ADDRESS_HPP
