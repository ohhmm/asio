//
// basic_timer_queue.hpp
// ~~~~~~~~~~~~~~~~~~~~~
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

#ifndef ASIO_BASIC_TIMER_QUEUE_HPP
#define ASIO_BASIC_TIMER_QUEUE_HPP

#include "asio/detail/push_options.hpp"

#include "asio/detail/push_options.hpp"
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/xtime.hpp>
#include "asio/detail/pop_options.hpp"

#include "asio/completion_context.hpp"
#include "asio/demuxer.hpp"

namespace asio {

/// The basic_timer_queue class template provides asynchronous timer
/// functionality. Most applications will simply use the timer_queue typedef.
template <typename Service>
class basic_timer_queue
  : private boost::noncopyable
{
public:
  /// Constructor.
  explicit basic_timer_queue(demuxer& d)
    : service_(dynamic_cast<Service&>(d.get_service(Service::id)))
  {
  }

  /// The handler for when a timer expires.
  typedef typename Service::timer_handler timer_handler;

  /// Schedule a timer to fire once at the given start_time. The id of the new
  /// timer is returned so that it may be cancelled.
  int schedule_timer(const boost::xtime& start_time,
      const timer_handler& handler,
      completion_context& context = completion_context::null())
  {
    return service_.schedule_timer(this, start_time, handler, context);
  }

  /// Schedule a timer to fire first after at the start time, and then every
  /// interval until the timer is cancelled. The id of the new timer is
  /// returned so that it may be cancelled.
  int schedule_timer(const boost::xtime& start_time,
      const boost::xtime& interval, const timer_handler& handler,
      completion_context& context = completion_context::null())
  {
    return service_.schedule_timer(this, start_time, interval, handler,
        context);
  }

  /// Cancel the timer with the given id.
  void cancel_timer(int timer_id)
  {
    service_.cancel_timer(this, timer_id);
  }

private:
  /// The backend service implementation.
  Service& service_;
};

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_BASIC_TIMER_QUEUE_HPP
