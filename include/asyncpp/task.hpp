#pragma once
#include <exception>
#include <future>
#include <coroutine>
#include <iostream>

namespace async
{
    template <typename T>
    class task
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

                constexpr void await_suspend(std::coroutine_handle<> h) noexcept;

                constexpr void await_resume() noexcept;
            };

            promise_type() = default;

            awaiter initial_suspend() noexcept;

            std::suspend_always final_suspend() noexcept;

            void return_value(const T &value) noexcept;

            void return_value(T &&value) noexcept;

            task<T> get_return_object() noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;

            T &&result();

        private:
            T _value;
            std::exception_ptr _unhandled_exception;
        };

        task() = default;

        task(std::coroutine_handle<promise_type> h) noexcept;

        T &&result();

        T &&await_resume();

        constexpr bool await_suspend(std::coroutine_handle<> h) noexcept;

        bool await_ready() const noexcept;

        bool done() const noexcept;

        template <typename Func, typename... Args>
        static task<T> run(const Func &func, const Args &...args);

        ~task() noexcept;

    private:
        std::coroutine_handle<promise_type> _handle;
    };

    template <typename T>
    constexpr bool task<T>::promise_type::awaiter::await_ready() const noexcept
    {
        return false;
    }

    template <typename T>
    constexpr void task<T>::promise_type::awaiter::await_suspend(std::coroutine_handle<> h) noexcept
    {
        // TODO: scheduling
        std::async(std::launch::async, h);
    }

    template <typename T>
    constexpr void task<T>::promise_type::awaiter::await_resume() noexcept
    {
    }

    template <typename T>
    task<T>::promise_type::awaiter task<T>::promise_type::initial_suspend() noexcept
    {
        return awaiter();
    }

    template <typename T>
    std::suspend_always task<T>::promise_type::final_suspend() noexcept
    {
        return {};
    }

    template <typename T>
    void task<T>::promise_type::return_value(const T &value) noexcept
    {
        _value = value;
    }

    template <typename T>
    void task<T>::promise_type::return_value(T &&value) noexcept
    {
        _value = std::move(value);
    }

    template <typename T>
    task<T> task<T>::promise_type::get_return_object() noexcept
    {
        return task<T>{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    template <typename T>
    void task<T>::promise_type::unhandled_exception() noexcept
    {
        _unhandled_exception = std::current_exception();
    }

    template <typename T>
    void task<T>::promise_type::rethrow_if_unhandled_exception() const
    {
        if (_unhandled_exception)
        {
            std::rethrow_exception(_unhandled_exception);
        }
    }

    template <typename T>
    T &&task<T>::promise_type::result()
    {
        rethrow_if_unhandled_exception();
        return std::move(_value);
    }

    template <typename T>
    task<T>::task(std::coroutine_handle<promise_type> h) noexcept
        : _handle(h)
    {
    }

    template <typename T>
    T &&task<T>::result()
    {
        // TODO: better way to sync this.
        while (!_handle.done())
        {
        }

        return _handle.promise().result();
    }

    template <typename T>
    T &&task<T>::await_resume()
    {
        return _handle.promise().result();
    }

    template <typename T>
    constexpr bool task<T>::await_suspend(std::coroutine_handle<> h) noexcept
    {
        return false;
    }

    template <typename T>
    bool task<T>::await_ready() const noexcept
    {
        return done();
    }

    template <typename T>
    bool task<T>::done() const noexcept
    {
        return _handle.done();
    }

    template <typename T>
    template <typename Func, typename... Args>
    task<T> task<T>::run(const Func &func, const Args &...args)
    {
        co_return func(args...);
    }

    template <typename T>
    task<T>::~task() noexcept
    {
        if (_handle)
        {
            if (_handle.done())
            {
                _handle.destroy();
            }
        }
    }

    template <>
    class task<void>
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

                void await_suspend(std::coroutine_handle<> h) noexcept;

                constexpr void await_resume() noexcept;
            };

            promise_type() = default;

            awaiter initial_suspend() noexcept;

            std::suspend_always final_suspend() noexcept;

            void return_void() noexcept;

            task<void> get_return_object() noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;

        private:
            std::exception_ptr _unhandled_exception;
        };

        task() = default;
        task(std::coroutine_handle<promise_type> h) noexcept;

        void await_resume() noexcept;

        constexpr bool await_suspend(std::coroutine_handle<> h) noexcept;

        bool await_ready() const noexcept;

        bool done() const noexcept;

        template <typename Func, typename... Args>
        task<void> run(const Func &func, const Args &...args);

        ~task() noexcept;

    private:
        std::coroutine_handle<promise_type> _handle;
    };

    constexpr bool task<void>::promise_type::awaiter::await_ready() const noexcept
    {
        return false;
    }

    void task<void>::promise_type::awaiter::await_suspend(std::coroutine_handle<> h) noexcept
    {
        // TODO: scheduling
        std::async(std::launch::async, h);
    }

    constexpr void task<void>::promise_type::awaiter::await_resume() noexcept
    {
    }

    task<void>::promise_type::awaiter task<void>::promise_type::initial_suspend() noexcept
    {
        return task<void>::promise_type::awaiter();
    }

    std::suspend_always task<void>::promise_type::final_suspend() noexcept
    {
        return {};
    }

    void task<void>::promise_type::return_void() noexcept
    {
    }

    task<void> task<void>::promise_type::get_return_object() noexcept
    {
        return task<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    void task<void>::promise_type::unhandled_exception() noexcept
    {
        _unhandled_exception = std::current_exception();
    }

    void task<void>::promise_type::rethrow_if_unhandled_exception() const
    {
        if (_unhandled_exception)
        {
            std::rethrow_exception(_unhandled_exception);
        }
    }

    task<void>::task(std::coroutine_handle<promise_type> h) noexcept
        : _handle(h)
    {
    }

    void task<void>::await_resume() noexcept
    {
    }

    constexpr bool task<void>::await_suspend(std::coroutine_handle<> h) noexcept
    {
        return false;
    }

    bool task<void>::await_ready() const noexcept
    {
        return done();
    }

    bool task<void>::done() const noexcept
    {
        return _handle.done();
    }

    template <typename Func, typename... Args>
    task<void> task<void>::run(const Func &func, const Args &...args)
    {
        func(args...);
        co_return;
    }

    task<void>::~task() noexcept
    {
        if (_handle)
        {
            if (_handle.done())
            {
                _handle.destroy();
            }
        }
    }
}