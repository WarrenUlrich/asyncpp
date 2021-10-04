#pragma once
#include <coroutine>
#include <future>
#include <semaphore>
#include <optional>

namespace async
{
    /**
     * @brief A task is a coroutine that is executed on a seperate thread and returns a single result.
    */
    template <typename T = void>
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
        T get()
        {
            return _handle.promise().get();
        }

        T await_resume() noexcept
        {
            return get();
        }

        bool await_suspend(std::coroutine_handle<> handle) noexcept
        {
            return false;
        }

        bool await_ready() noexcept
        {
            return _handle.done();
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
                std::thread(h).detach();
            }

            constexpr void await_resume() const noexcept {}
        };

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
            _value_ready = true;
        }

        task<T> get_return_object()
        {
            return task<T>(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        void unhandled_exception()
        {
            _exception = std::current_exception();
            _value_ready = true;
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
        T get()
        {
            while (!_value_ready.load())
                return get();

            rethrow_if_unhandled_exception();
            return _value;
        }

    private:
        T _value;
        std::atomic_bool _value_ready;
        std::exception_ptr _exception;
    };

    template <>
    class task<void>::promise_type
    {
    public:
        class awaiter
        {
        public:
            constexpr bool await_ready() const noexcept
            {
                return false;
            }

            void await_suspend(std::coroutine_handle<promise_type> h) const noexcept
            {
                // TODO: Implement thread pool scheduling.
                std::thread(h).detach();
            }

            constexpr void await_resume() const noexcept {}
        };

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

        void return_void() noexcept
        {

        }

        task<void> get_return_object()
        {
            return task<void>(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        void unhandled_exception()
        {
            _exception = std::current_exception();
            _value_ready = true;
        }

        void rethrow_if_unhandled_exception()
        {
            if (_exception)
                std::rethrow_exception(_exception);
        }

    private:
        std::atomic_bool _value_ready;
        std::exception_ptr _exception;
    };
}
