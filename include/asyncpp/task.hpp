#pragma once
#include <coroutine>
#include <future>
#include <semaphore>
#include <optional>
#include <exception>
#include "scheduler.hpp"

namespace async
{
    /**
     * @brief A task is a coroutine that is executed on a seperate thread and returns a single result.
    */
    template <typename T>
    class task
    {
    public:
        class promise_type;

        using handle_type = std::coroutine_handle<promise_type>;

        task(handle_type handle) : _handle(handle) {}

        task(task &&other) noexcept : _handle(std::move(other._handle)) {}

        /**
         * @brief Waits for the task to complete and retrieves the result.
         * @throws any unhandled exception that may have occurred.
         */
        T&& get()
        {
            return _handle.promise().get();
        }

        T&& await_resume() noexcept
        {
            return _handle.promise().get();
        }

        bool await_suspend(handle_type handle) noexcept
        {
            return false;
        }

        bool await_ready() noexcept
        {
            return _handle.done();
        }

        template<typename F>
        static task<T> run(const F& func)
        {
            co_return func();
        }

        template<typename F>
        static task<T> run(F&& func)
        {
            co_return func();
        }
        
    private:
        handle_type _handle;
    };

    template <typename T>
    class task<T>::promise_type
    {
    public:
        class awaiter
        {
        public:
            constexpr bool await_ready() const noexcept
            {
                return false;
            }

            constexpr void await_suspend(handle_type h) const noexcept
            {
                // TODO: Implement thread pool scheduling.
                CUSTOM_SCHEDULER(h);
            }

            constexpr void await_resume() const noexcept {}
        };

        promise_type()
            : _value(T()), _exception(nullptr), _complete(0)
        {
        }

        promise_type(promise_type &&other) noexcept = default;
        promise_type(promise_type const &other) = delete;

        promise_type &operator=(promise_type const &other) = delete;
        promise_type &operator=(promise_type &&other) noexcept = default;

        awaiter initial_suspend() noexcept
        {
            return {};
        }

        std::suspend_always final_suspend() noexcept
        {
            return {};
        }

        void return_value(T t) noexcept
        {
            _value = t;
            _complete.release();
        }

        task<T> get_return_object()
        {
            return task<T>(handle_type::from_promise(*this));
        }

        void unhandled_exception()
        {
            _exception = std::current_exception();
            _complete.release();
        }

        void rethrow_if_unhandled_exception()
        {
            if (_exception)
                std::rethrow_exception(_exception);
        }

        /**
         * @brief Waits for the task to complete and returns the result.
         * @return The result of the task.
         * @throws any unhandled exception that may have occurred.
         */
        T&& get()
        {
            _complete.acquire();
            rethrow_if_unhandled_exception();
            return std::move(this->_value);
        }

    private:
        T _value;
        std::binary_semaphore _complete;
        std::exception_ptr _exception;
    };
}
