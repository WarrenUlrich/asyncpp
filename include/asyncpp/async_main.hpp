#pragma once
#include <coroutine>
#include <exception>

namespace async::detail
{
    class async_main_coro
    {
    public:
        class promise_type
        {
        public:
            class awaiter
            {
            public:
                awaiter() = default;

                constexpr bool await_ready() const noexcept;

                void await_suspend(std::coroutine_handle<promise_type> h) noexcept;

                constexpr void await_resume() noexcept;
            };

            int value;
            
            std::exception_ptr exception;

            promise_type() = default;

            awaiter initial_suspend() const noexcept;

            std::suspend_always final_suspend() const noexcept;

            void return_value(int v) noexcept;

            async_main_coro get_return_object() noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;
        };

        async_main_coro() noexcept = default;

        async_main_coro(std::coroutine_handle<promise_type> h) noexcept;

        int await_resume() noexcept;

        constexpr bool await_suspend(std::coroutine_handle<promise_type> h) noexcept;

        bool await_ready() const noexcept;

    private:
        std::coroutine_handle<promise_type> _handle;
    };

    constexpr bool async_main_coro::promise_type::awaiter::await_ready() const noexcept
    {
        return false;
    }

    void async_main_coro::promise_type::awaiter::await_suspend(std::coroutine_handle<promise_type> h) noexcept
    {

        if (h)
        {
            if (!h.done())
                h.resume();
        }
    }

    constexpr void async_main_coro::promise_type::awaiter::await_resume() noexcept
    {
    }

    async_main_coro::promise_type::awaiter async_main_coro::promise_type::initial_suspend() const noexcept
    {
        return awaiter();
    }

    std::suspend_always async_main_coro::promise_type::final_suspend() const noexcept
    {
        return std::suspend_always();
    }

    void async_main_coro::promise_type::return_value(int v) noexcept
    {
        value = v;
    }

    async_main_coro async_main_coro::promise_type::get_return_object() noexcept
    {
        return async_main_coro(std::coroutine_handle<promise_type>::from_promise(*this));
    }

    void async_main_coro::promise_type::unhandled_exception() noexcept
    {
        exception = std::current_exception();
    }

    void async_main_coro::promise_type::rethrow_if_unhandled_exception() const
    {
        if (exception)
        {
            std::rethrow_exception(exception);
        }
    }

    async_main_coro::async_main_coro(std::coroutine_handle<promise_type> h) noexcept
        : _handle(h)
    {
    }

    int async_main_coro::await_resume() noexcept
    {
        return _handle.promise().value;
    }

    constexpr bool async_main_coro::await_suspend(std::coroutine_handle<promise_type> h) noexcept
    {
        return false;
    }

    bool async_main_coro::await_ready() const noexcept
    {
        return false;
    }
}

async::detail::async_main_coro async_main(int argc, char **argv);

//macro that wraps around main function
#define ASYNC_MAIN                                                               \
    int main(int argc, char **argv)                                              \
    {                                                                            \
        auto coro = async_main(argc, argv);                                                  \
        return coro.await_resume();                                                                \
    }                                                                            \
    async::detail::async_main_coro async_main \