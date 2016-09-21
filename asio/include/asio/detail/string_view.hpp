//
// detail/string_view.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_STRING_VIEW_HPP
#define ASIO_DETAIL_STRING_VIEW_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO_HAS_STD_STRING_VIEW)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
# include <experimental/string_view>
#else // defined(ASIO_HAS_EXPERIMENTAL_STRING_VIEW)
# include <string_view>
#endif // defined(ASIO_HAS_EXPERIMENTAL_STRING_VIEW)

namespace asio {

#if defined(ASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
using std::experimental::basic_string_view;
using std::experimental::string_view;
#else // defined(ASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
using std::basic_string_view;
using std::string_view;
#endif // defined(ASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)

} // namespace asio

#endif // defined(ASIO_HAS_STD_STRING_VIEW)

#endif // ASIO_DETAIL_STRING_VIEW_HPP
