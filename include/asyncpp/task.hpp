#pragma once
#include <exception>
#include <coroutine>
#include <thread>
#include <semaphore>
#include "aggregate_exception.hpp"

namespace async
{
    template <typename T>
    class task
    {
    public:
        class promise_type
        {
        public:
            promise_type() = default;

            auto initial_suspend() noexcept;

            auto final_suspend() noexcept;

            void return_value(const T &value) noexcept;

            void return_value(T &&value) noexcept;

            task<T> get_return_object() noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;

            T get_result();

            void wait();

        private:
            T _value;
            std::binary_semaphore _done{0};
            std::exception_ptr _unhandled_exception;
        };

        task() = default;

        task(std::coroutine_handle<promise_type> h) noexcept;

        task(task &&other) noexcept;

        task &operator=(task &&other) noexcept;

        T get_result();

        T await_resume();

        constexpr bool await_suspend(std::coroutine_handle<> h) noexcept;

        bool await_ready() const noexcept;

        bool done() const noexcept;

        static task<T> run(auto &&func, const auto &...args);

        static task<T> run(auto &&func, auto &&...args);

        void wait() const;

        ~task() noexcept;

    private:
        std::coroutine_handle<promise_type> _handle;
    };

    template <typename T>
    auto task<T>::promise_type::initial_suspend() noexcept
    {
        class awaiter : public std::suspend_always
        {
        public:
            awaiter() = default;

            constexpr void await_suspend(std::coroutine_handle<> h) noexcept
            {
                std::thread(h).detach();
            }
        };

        return awaiter();
    }

    template <typename T>
    auto task<T>::promise_type::final_suspend() noexcept
    {
        class awaiter : public std::suspend_always
        {
        public:
            awaiter() = default;
            constexpr void await_suspend(std::coroutine_handle<> h) noexcept
            {
            }
        };

        return awaiter();
    }

    template <typename T>
    void task<T>::promise_type::return_value(const T &value) noexcept
    {
        _value = value;
        _done.release();
    }

    template <typename T>
    void task<T>::promise_type::return_value(T &&value) noexcept
    {
        _value = std::move(value);
        _done.release();
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
        _done.release();
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
    T task<T>::promise_type::get_result()
    {
        wait();
        return std::move(_value);
    }

    template <typename T>
    void task<T>::promise_type::wait()
    {
        _done.acquire();
        rethrow_if_unhandled_exception();
    }

    template <typename T>
    task<T>::task(std::coroutine_handle<promise_type> h) noexcept
        : _handle(h)
    {
    }


    template <typename T>
    task<T>::task(task &&other) noexcept
        : _handle(std::exchange(other._handle, nullptr))
    {
    }

    template <typename T>
    task<T> &task<T>::operator=(task &&other) noexcept
    {
        _handle = std::exchange(other._handle, nullptr);
        return *this;
    }
    
    template <typename T>
    T task<T>::get_result()
    {
        return _handle.promise().get_result();
    }

    template <typename T>
    T task<T>::await_resume()
    {
        return _handle.promise().get_result();
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
    task<T> task<T>::run(auto &&func, const auto &...args)
    {
        co_return func(args...);
    }

    template <typename T>
    task<T> task<T>::run(auto &&func, auto &&...args)
    {
        return [](auto &&func_, auto... args_) -> task<T> {
            co_return func_(args_...);
        }(func, std::move(args)...);
    }

    template <typename T>
    void task<T>::wait() const
    {
        _handle.promise().wait();
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
            else
            {
                _handle.promise().wait();
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
            promise_type() = default;

            auto initial_suspend() noexcept;

            auto final_suspend() noexcept;

            void return_void() noexcept;

            task<void> get_return_object() noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;

            void wait();

        private:
            std::binary_semaphore _done{0};
            std::exception_ptr _unhandled_exception;
        };

        task() = default;

        task(std::coroutine_handle<promise_type> h) noexcept;

        task(task &&other) noexcept;

        void await_resume() const;

        constexpr bool await_suspend(std::coroutine_handle<> h) const noexcept;

        bool await_ready() const noexcept;

        bool done() const noexcept;

        static task<void> run(auto &&func, const auto &...args);

        static task<void> run(auto &&func, auto &&...args);

        void wait() const;

        static task<void> when_all(const std::ranges::range auto &tasks);

        ~task() noexcept;

    private:
        std::coroutine_handle<promise_type> _handle;
    };

    auto task<void>::promise_type::initial_suspend() noexcept
    {
        class awaiter : public std::suspend_always
        {
        public:
            awaiter() = default;

            void await_suspend(std::coroutine_handle<> h) noexcept
            {
                std::thread(h).detach();
            }
        };

        return awaiter();
    }

    auto task<void>::promise_type::final_suspend() noexcept
    {
        class awaiter : public std::suspend_always
        {
        public:
            awaiter() = default;
            constexpr void await_suspend(std::coroutine_handle<> h) noexcept
            {
            }
        };

        return awaiter();
    }

    void task<void>::promise_type::return_void() noexcept
    {
        _done.release();
    }

    task<void> task<void>::promise_type::get_return_object() noexcept
    {
        return task<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    void task<void>::promise_type::unhandled_exception() noexcept
    {
        _unhandled_exception = std::current_exception();
        _done.release();
    }

    void task<void>::promise_type::rethrow_if_unhandled_exception() const
    {
        if (_unhandled_exception)
        {
            std::rethrow_exception(_unhandled_exception);
        }
    }

    void task<void>::promise_type::wait()
    {
        _done.acquire();
        rethrow_if_unhandled_exception();
    }

    task<void>::task(std::coroutine_handle<promise_type> h) noexcept
        : _handle(h)
    {
    }

    task<void>::task(task &&other) noexcept
        : _handle(std::exchange(other._handle, nullptr))
    {
    }

    void task<void>::await_resume() const
    {
        wait();
    }

    constexpr bool task<void>::await_suspend(std::coroutine_handle<> h) const noexcept
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

    task<void> task<void>::run(auto &&func, const auto &...args)
    {
        co_return func(args...);
    }

    task<void> task<void>::run(auto &&func, auto &&...args)
    {
        return [](auto&& func_, auto... args_) -> task<void> {
            co_return func_(args_...);
        }(func, std::move(args)...);
    }

    void task<void>::wait() const
    {
        _handle.promise().wait();
    }

    task<void> task<void>::when_all(const std::ranges::range auto &tasks)
    {
        for (auto &task : tasks)
        {
            co_await task;
        }

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
            else
            {
                _handle.promise().wait();
                _handle.destroy();
            }
        }
    }
}