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

            auto initial_suspend() noexcept
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

            auto final_suspend() noexcept
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

            void return_value(const T &value) noexcept
            {
                _value = value;
                _done.release();
            }

            void return_value(T &&value) noexcept
            {
                _value = std::move(value);
                _done.release();
            }

            task<T> get_return_object() noexcept
            {
                return task<T>{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            void unhandled_exception() noexcept
            {
                _unhandled_exception = std::current_exception();
                _done.release();
            }

            void rethrow_if_unhandled_exception() const
            {
                if (_unhandled_exception)
                {
                    std::rethrow_exception(_unhandled_exception);
                }
            }

            T get_result()
            {
                wait();
                return std::move(_value);
            }

            void wait()
            {
                _done.acquire();
                rethrow_if_unhandled_exception();
            }

        private:
            T _value;
            std::binary_semaphore _done{0};
            std::exception_ptr _unhandled_exception;
        };

        task() = default;

        task(std::coroutine_handle<promise_type> h) noexcept
            : _handle(h)
        {
        }

        task(task &&other) noexcept
            : _handle(std::exchange(other._handle, nullptr))
        {
        }

        task<T> &operator=(task &&other) noexcept
        {
            _handle = std::exchange(other._handle, nullptr);
            return *this;
        }

        T get_result()
        {
            return _handle.promise().get_result();
        }

        T await_resume()
        {
            return _handle.promise().get_result();
        }

        constexpr bool await_suspend(std::coroutine_handle<> h) noexcept
        {
            return false;
        }

        bool await_ready() const noexcept
        {
            return done();
        }

        bool done() const noexcept
        {
            return _handle.done();
        }


        task<T> run(auto &&func, const auto &...args)
        {
            co_return func(args...);
        }

        task<T> run(auto &&func, auto &&...args)
        {
            return [](auto &&func_, auto... args_) -> task<T> {
                co_return func_(args_...);
            }(func, std::move(args)...);
        }

        void wait() const
        {
            _handle.promise().wait();
        }

        ~task() noexcept
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

    private:
        std::coroutine_handle<promise_type> _handle;
    };

   

    template <>
    class task<void>
    {
    public:
        class promise_type
        {
        public:
            promise_type() = default;

            auto initial_suspend() noexcept
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

            auto final_suspend() noexcept
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

            void return_void() noexcept
            {
                _done.release();
            }

            task<void> get_return_object() noexcept
            {
                return task<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            void unhandled_exception() noexcept
            {
                _unhandled_exception = std::current_exception();
                _done.release();
            }

            void rethrow_if_unhandled_exception() const
            {
                if (_unhandled_exception)
                {
                    std::rethrow_exception(_unhandled_exception);
                }
            }

            void wait()
            {
                _done.acquire();
                rethrow_if_unhandled_exception();
            }

        private:
            std::binary_semaphore _done{0};
            std::exception_ptr _unhandled_exception;
        };

        task() = default;

        task(std::coroutine_handle<promise_type> h) noexcept
            : _handle(h)
        {
        }

        task(task &&other) noexcept
            : _handle(std::exchange(other._handle, nullptr))
        {
        }

        void await_resume() const
        {
            wait();
        }

        constexpr bool await_suspend(std::coroutine_handle<> h) const noexcept
        {
            return false;
        }

        bool await_ready() const noexcept
        {
            return done();
        }

        bool done() const noexcept
        {
            return _handle.done();
        }

        static task<void> run(auto &&func, const auto &...args)
        {
            co_return func(args...);
        }

        static task<void> run(auto &&func, auto &&...args)
        {
            return [](auto &&func_, auto... args_) -> task<void> {
                co_return func_(args_...);
            }(func, std::move(args)...);
        }

        void wait() const
        {
            _handle.promise().wait();
        }

        static task<void> when_all(const std::ranges::range auto &tasks)
        {
            for (auto &task : tasks)
            {
                co_await task;
            }

            co_return;
        }

        ~task() noexcept
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

    private:
        std::coroutine_handle<promise_type> _handle;
    };
}