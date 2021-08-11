#pragma once
#include <coroutine>
#include <future>
#include <semaphore>
#include <optional>

namespace async
{
    template <typename T>
    class task
    {
    public:
        class promise_type;

        using handle_type = std::coroutine_handle<promise_type>;

        task(handle_type handle) : _handle(handle) {}

        task(task &&other) noexcept : _handle(std::move(other._handle)) {}

        T get()
        {
            return _handle.promise().get();
        }

        template <typename Rep, typename Period>
        std::optional<T> try_get_for(std::chrono::duration<Rep, Period> &&duration) noexcept
        {
            return _handle.promise().try_get_for(std::move(duration));
        }

        std::optional<T> try_get_until(std::chrono::time_point<std::chrono::high_resolution_clock> &&time) noexcept
        {
            return _handle.promise().try_get_until(std::move(time));
        }

    private:
        handle_type _handle;

        bool await_suspend(std::coroutine_handle<> handle) noexcept
        {
            return false;
        }

        bool await_ready() noexcept
        {
            return _handle.done();
        }

        T await_resume() noexcept
        {
            return get();
        }
    };

    template <typename T>
    class task<T>::promise_type
    {
    public:
        class awaiter;

        promise_type() = default;
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
            _value_ready.release();
        }

        task<T> get_return_object()
        {
            return task<T>(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        void unhandled_exception()
        {
            _exception = std::current_exception();
            _value_ready.release();
        }

        void rethrow_if_unhandled_exception()
        {
            if (_exception)
                std::rethrow_exception(_exception);
        }

        /**
         * @brief Waits for the task to complete and returns the result.
         */
        T get()
        {
            _value_ready.acquire();
            rethrow_if_unhandled_exception();
            return std::move(_value);
        }

        /**
         * @brief Returns the task's result if it's ready, otherwise immediately returns a nullopt.
         */
        std::optional<T> try_get() noexcept
        {
            if (_value_ready.try_acquire())
                return std::move(_value);

            return std::nullopt;
        }

        /**
         * @brief Tries to return the task's result for a duration, if it's not ready, immediately returns a nullopt.
         * @param duration The duration to wait for the task to complete before returning a nullopt.
         */
        template <typename Rep, typename Period>
        std::optional<T> try_get_for(std::chrono::duration<Rep, Period> &&duration) noexcept
        {
            if (_value_ready.try_acquire_for(duration))
                return std::move(_value);

            return std::nullopt;
        }

        /**
         * @brief Tries to return the task's result until a specified time, if it isn't ready by then, immediately returns a nullopt.
         * @param time The time to wait until before returning a nullopt.
         */
        std::optional<T> try_get_until(std::chrono::time_point<std::chrono::high_resolution_clock> &&time) noexcept;

    private:
        T _value;
        std::binary_semaphore _value_ready{0};
        std::exception_ptr _exception;
    };

    template <typename T>
    class task<T>::promise_type::awaiter
    {
    public:
        constexpr bool await_ready() const noexcept
        {
            return false;
        }

        constexpr void await_suspend(handle_type h) const noexcept
        {
            std::thread(h).detach();
        }

        constexpr void await_resume() const noexcept {}
    };
}
