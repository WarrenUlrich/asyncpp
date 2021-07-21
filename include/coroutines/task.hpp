#pragma once
#include <coroutine>
#include <future>
#include "default_scheduler.hpp"

namespace coroutines
{
    template<class T, class Scheduler>
    class task_promise_type;

    // Created tasks are "hot" or "eager", they immediately run on a seperate thread
    // and the result can be awaited, or waited for with "result".
    template<class T = void, class Scheduler = default_scheduler>
    class task
    {
    public:
        using promise_type = task_promise_type<T, Scheduler>;
        using handle_type = std::coroutine_handle<promise_type>;

        handle_type handle;

        task(handle_type handle)
            : handle(handle)
        {

        }

        task(task&& other) noexcept
            : handle(other.handle)
        {
            other.handle = nullptr;
        };

        bool await_ready()
        {
            return handle.done();
        }

        bool await_suspend(std::coroutine_handle<promise_type> handle)
        {
            return false;
        }

        auto await_resume()
        {
            return result();
        }

        T result() const
        {
            return handle.promise().value.get_future().get();
        }
    };

    template<class T, class Scheduler>
    class task_promise_type
    {
    public:
        std::promise<T> value;
        std::exception_ptr exception = nullptr;

        auto initial_suspend()
        {
            return awaiter{};
        }

        auto final_suspend() noexcept
        {
            return std::suspend_always{};
        }

        void return_value(T t) noexcept
        {
            value.set_value(std::move(t));
        }

        task<T, Scheduler> get_return_object()
        {
            return { task<T, Scheduler>::handle_type::from_promise(*this) };
        }

        void unhandled_exception()
        {
            exception = std::current_exception();
        }

        void rethrow_if_unhandled_exception()
        {
            if (exception)
                std::rethrow_exception(std::move(exception));
        }
    private:
        class awaiter
        {
        public:
            constexpr bool await_ready() const noexcept
            {
                return false;
            }

            constexpr void await_suspend(std::coroutine_handle<> h) const noexcept
            {
                Scheduler::instance().schedule(h);
            }

            constexpr void await_resume() const noexcept
            {

            }
        };
    };

    template<class Scheduler>
    class task_promise_type<void, Scheduler>
    {
    public:
        std::exception_ptr  exception = nullptr;

        auto initial_suspend()
        {
            return awaiter{};
        }

        auto final_suspend() noexcept
        {
            return std::suspend_always{};
        }

        void return_void() noexcept {}

        task<void, Scheduler> get_return_object()
        {
            return { task<void, Scheduler>::handle_type::from_promise(*this) };
        }

        void unhandled_exception()
        {
            exception = std::current_exception();
        }

        void rethrow_if_unhandled_exception()
        {
            if (exception)
                std::rethrow_exception(std::move(exception));
        }
    private:
        class awaiter
        {
        public:
            constexpr bool await_ready() const noexcept
            {
                return false;
            }

            constexpr void await_suspend(std::coroutine_handle<> h) const noexcept
            {
                Scheduler::instance().schedule(h);
            }

            constexpr void await_resume() const noexcept
            {

            }
        };
    };
}