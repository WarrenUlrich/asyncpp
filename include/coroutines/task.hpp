#pragma once
#include <coroutine>
#include <future>

namespace coroutines
{
    template<class T = void>
    struct task
    {
        class promise_type
        {
            class awaiter
            {
            public:
                constexpr bool await_ready() const noexcept
                {
                    return false;
                }

                constexpr void await_suspend(std::coroutine_handle<> h) const noexcept
                {
                    /*
                    * TODO: Actual thread scheduling here.
                    */
                    
                    std::thread(h).detach();
                }

                constexpr void await_resume() const noexcept
                {

                }
            };
        public:
            std::promise<T> value;
            std::exception_ptr  m_exception = nullptr;

        public:
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

            task<T> get_return_object()
            {
                return { handle_type::from_promise(*this) };
            }

            void unhandled_exception()
            {
                m_exception = std::current_exception();
            }

            void rethrow_if_unhandled_exception()
            {
                if (m_exception)
                    std::rethrow_exception(std::move(m_exception));
            }
        };

        using handle_type = std::coroutine_handle<promise_type>;

        handle_type m_handle;

        task(handle_type handle)
            : m_handle(handle)
        {

        }

        task(task&& other) noexcept
            : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
        };

        bool await_ready()
        {
            return m_handle.done();
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
            return m_handle.promise().value.get_future().get();
        }
    };
}