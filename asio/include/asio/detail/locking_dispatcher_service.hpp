//
// locking_dispatcher_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

#ifndef ASIO_DETAIL_LOCKING_DISPATCHER_SERVICE_HPP
#define ASIO_DETAIL_LOCKING_DISPATCHER_SERVICE_HPP

#include "asio/detail/push_options.hpp"

#include "asio/detail/push_options.hpp"
#include <boost/noncopyable.hpp>
#include "asio/detail/pop_options.hpp"

#include "asio/detail/bind_handler.hpp"
#include "asio/detail/mutex.hpp"

namespace asio {
namespace detail {

template <typename Demuxer>
class locking_dispatcher_service
{
public:
  class dispatcher_impl
    : private boost::noncopyable
  {
  public:
    // Constructor.
    dispatcher_impl()
      : first_waiter_(0),
        last_waiter_(0),
        mutex_()
    {
    }

    // Request a dispatcher to invoke the given handler.
    template <typename Handler>
    void dispatch(Demuxer& demuxer, Handler handler)
    {
      detail::mutex::scoped_lock lock(mutex_);

      if (first_waiter_ == 0)
      {
        // This handler now has the lock, so can be dispatched immediately.
        first_waiter_ = last_waiter_ = new waiter<Handler>(handler);
        lock.unlock();
        demuxer.dispatch(waiter_handler(demuxer, *this));
      }
      else
      {
        // Another waiter already holds the lock, so this handler must join
        // the list of waiters. The handler will be posted automatically when
        // its turn comes.
        last_waiter_->next_ = new waiter<Handler>(handler);
        last_waiter_ = last_waiter_->next_;
      }
    }

    // Request a dispatcher to invoke the given handler and return immediately.
    template <typename Handler>
    void post(Demuxer& demuxer, Handler handler)
    {
      detail::mutex::scoped_lock lock(mutex_);

      if (first_waiter_ == 0)
      {
        // This handler now has the lock, so can be posted immediately.
        first_waiter_ = last_waiter_ = new waiter<Handler>(handler);
        lock.unlock();
        demuxer.post(waiter_handler(demuxer, *this));
      }
      else
      {
        // Another waiter already holds the lock, so this handler must join
        // the list of waiters. The handler will be posted automatically when
        // its turn comes.
        last_waiter_->next_ = new waiter<Handler>(handler);
        last_waiter_ = last_waiter_->next_;
      }
    }

  private:
    // Base class for all waiter types.
    class waiter_base
    {
    public:
      waiter_base()
        : next_(0)
      {
      }

      virtual ~waiter_base()
      {
      }

      virtual void call() = 0;

      waiter_base* next_;
    };

    // Class template for a waiter.
    template <typename Handler>
    class waiter
      : public waiter_base
    {
    public:
      waiter(Handler handler)
        : handler_(handler)
      {
      }

      virtual void call()
      {
        handler_();
      }

    private:
      Handler handler_;
    };

    // Helper class to allow waiting handlers to be dispatched.
    class waiter_handler
    {
    public:
      waiter_handler(Demuxer& demuxer, dispatcher_impl& impl)
        : demuxer_(demuxer),
          impl_(impl)
      {
      }

      void operator()()
      {
        do_upcall();

        detail::mutex::scoped_lock lock(impl_.mutex_);

        waiter_base* tmp = impl_.first_waiter_;
        impl_.first_waiter_ = impl_.first_waiter_->next_;
        if (impl_.first_waiter_)
        {
          lock.unlock();

          // Ensure the waiter is not deleted until after we have finished
          // accessing the dispatcher, since the waiter might indirectly own
          // the dispatcher and so destroying the waiter will cause the
          // dispatcher to be destroyed.
          delete tmp;

          // There is more work to do, so post this handler again.
          demuxer_.post(*this);
        }
        else
        {
          impl_.last_waiter_ = 0;

          lock.unlock();

          // Ensure the waiter is not deleted until after we have finished
          // accessing the dispatcher, since the waiter might indirectly own
          // the dispatcher and so destroying the waiter will cause the
          // dispatcher to be destroyed.
          delete tmp;
        }
      }

      void do_upcall()
      {
        try
        {
          impl_.first_waiter_->call();
        }
        catch (...)
        {
        }
      }

    private:
      Demuxer& demuxer_;
      dispatcher_impl& impl_;
    };

    friend class waiter_handler;

    // The start of the list of waiters for the dispatcher. If this pointer
    // is non-null then it indicates that a handler holds the lock.
    waiter_base* first_waiter_;
    
    // The end of the list of waiters for the dispatcher.
    waiter_base* last_waiter_;

    // Mutex to protect access to internal data.
    detail::mutex mutex_;
  };

  // The native type of the locking dispatcher.
  typedef dispatcher_impl* impl_type;

  // Return a null locking dispatcher implementation.
  static impl_type null()
  {
    return 0;
  }

  // Constructor.
  locking_dispatcher_service(Demuxer& d)
    : demuxer_(d)
  {
  }

  // The demuxer type for this service.
  typedef Demuxer demuxer_type;

  // Get the demuxer associated with the service.
  demuxer_type& demuxer()
  {
    return demuxer_;
  }

  // Create a new locking dispatcher implementation.
  void create(impl_type& impl)
  {
    impl = new dispatcher_impl;
  }

  // Destroy a locking dispatcher implementation.
  void destroy(impl_type& impl)
  {
    if (impl != null())
    {
      delete impl;
      impl = null();
    }
  }

  // Request a dispatcher to invoke the given handler.
  template <typename Handler>
  void dispatch(impl_type& impl, Handler handler)
  {
    impl->dispatch(demuxer_, handler);
  }

  // Request a dispatcher to invoke the given handler and return immediately.
  template <typename Handler>
  void post(impl_type& impl, Handler handler)
  {
    impl->post(demuxer_, handler);
  }

private:
  // The demuxer used for dispatching handlers.
  Demuxer& demuxer_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_LOCKING_DISPATCHER_SERVICE_HPP
