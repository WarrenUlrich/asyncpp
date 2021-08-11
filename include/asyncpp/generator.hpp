#pragma once
#include <coroutine>
#include <vector>
#include <iostream>

namespace async
{
    /**
     * @brief Represents a sequence of values
     */
    template <class T>
    class generator
    {
    public:
        class promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        class iterator;

        generator() = delete;

        generator(handle_type h)
            : _handle(h){

              };

        generator(generator<T> &&other) noexcept
            : _handle(other._handle)
        {
            other._handle = nullptr;
        }

        generator<T> &operator=(generator<T> &&other) noexcept
        {
            _handle = other._handle;
            other._handle = nullptr;
            return *this;
        }

        template <std::ranges::range Range>
        generator(const Range &r)
        {
            *this = [](auto &range) -> generator<T>
            {
                for (auto &i : range)
                    co_yield i;
            }(r);
        }

        template <std::ranges::range Range>
        generator(Range &&r)
        {
            *this = [](auto range) -> generator<T>
            {
                for (auto &i : range)
                    co_yield i;
            }(std::move(r));
        }

        iterator begin()
        {
            if (this->_handle)
                if (!this->_handle.done())
                    this->_handle.resume();

            auto exception = std::current_exception();
            if (exception)
                std::rethrow_exception(exception);

            return iterator(this->_handle);
        }

        std::default_sentinel_t end() const noexcept
        {
            return {};
        }

        /**
         * @brief Filters a sequence of values based on a predicate.
         * @param pred The predicate to use to filter the sequence.
         * @return A new enumerable sequence containing the filtered values.
         */
        template <class Predicate>
        generator<T> where(Predicate &&pred)
        {
            return where(std::move(*this), pred);
        }

        /**
         * @brief Projects a sequence of values into a new sequence of a different type.
         * @param mapper The function to use to map the sequence.
         * @return A new enumerable sequence containing the projected values.
         */
        template <class Mapper>
        generator<std::invoke_result_t<Mapper, T &>> map(Mapper &&mapper)
        {
            return map(std::move(*this), mapper);
        }

        /**
         * @brief Returns the first value in a sequence.
         * @return The first value in the sequence.
         */
        T first()
        {
            for (auto &i : *this)
                return i;
        }

        /**
         * @brief Returns the first value in a sequence that satisfies a predicate.
         * @param pred The predicate to use to filter the sequence.
         * @return The first value in the sequence that satisfies the predicate.
         */
        template <class Predicate>
        T first(Predicate &&pred)
        {
            return first(std::move(*this), pred);
        }

        /**
         * @brief Counts and returns the number of elements in a sequence.
         * @return The number of elements in the sequence.
         */
        std::size_t count()
        {
            std::size_t temp{};
            for (auto &i : *this)
                temp++;
            return temp;
        }

        /**
         * @brief Enumerates the sequence and stores the results in a vector.
         * @return The vector containing the results.
         */
        std::vector<T> collect()
        {
            std::vector<T> result{};
            for (const auto &i : *this)
                result.emplace_back(i);
            return result;
        }

        ~generator()
        {
            if (this->_handle)
                this->_handle.destroy();
        }

    private:
        handle_type _handle;

        template <class Predicate>
        static generator<T> where(generator<T> e, Predicate &&pred)
        {
            for (auto &i : e)
                if (pred(i))
                    co_yield i;
        }

        template <class Mapper>
        static generator<std::invoke_result_t<Mapper, T &>> map(generator<T> e, Mapper &&m)
        {
            for (auto &i : e)
                co_yield m(i);
        }

        template <class Predicate>
        static T first(generator<T> e, Predicate &&pred)
        {
            for (auto &i : e)
                if (pred(i))
                    return i;
        }
    };

    template <>
    class generator<void>
    {
    public:
        /**
         * @brief Creates a new enumerable sequence of integers from the specified range.
         * @param to The lower bound of the range.
         * @param from The upper bound of the range.
         * @return A new enumerable sequence of integers.
         */
        template <std::integral Integral>
        static generator<Integral> range(Integral to, Integral from)
        {
            for (Integral i = to; i <= from; ++i)
                co_yield i;
        }
    };

    template <typename T>
    class generator<T>::promise_type
    {
    public:
        T current_value;
        std::exception_ptr exception;

        promise_type() : current_value{}, exception{nullptr} {}

        auto get_return_object()
        {
            return generator{handle_type::from_promise(*this)};
        }

        auto initial_suspend()
        {
            return std::suspend_always{};
        }

        auto final_suspend() noexcept
        {
            return std::suspend_always();
        }

        void unhandled_exception()
        {
            exception = std::current_exception();
        }

        void rethrow_if_unhandled_exception()
        {
            if (exception)
                std::rethrow_exception(exception);
        }

        void return_void()
        {
        }

        auto yield_value(T const &value) noexcept
        {
            current_value = value;
            return std::suspend_always{};
        }

        auto yield_value(T &&value) noexcept
        {
            current_value = std::move(value);
            return std::suspend_always{};
        }
    };

    template <typename T>
    class generator<T>::iterator
    {
    public:
        iterator(handle_type handle)
            : _handle(handle)
        {
        }

        T &operator*() const
        {
            return this->_handle.promise().current_value;
        }

        T *operator->()
        {
            return &this->_handle.promise().current_value;
        }

        iterator &operator++()
        {
            this->_handle.resume();

            if (this->_handle.done())
                this->_handle.promise().rethrow_if_unhandled_exception();
            return *this;
        }

        friend bool operator==(const iterator &it, std::default_sentinel_t s) noexcept
        {
            return !it._handle || it._handle.done();
        }

        friend bool operator!=(const iterator &it, std::default_sentinel_t s) noexcept
        {
            return !(it == s);
        }

        friend bool operator==(std::default_sentinel_t s, const iterator &it) noexcept
        {
            return (it == s);
        }

        friend bool operator!=(std::default_sentinel_t s, const iterator &it) noexcept
        {
            return it != s;
        }

    private:
        handle_type _handle;
    };
}