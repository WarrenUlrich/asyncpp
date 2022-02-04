#pragma once
#include <concepts>
#include <coroutine>
#include <stdexcept>
#include <iterator>
#include <vector>
#include <set>
#include <execution>
#include "task.hpp"

namespace async
{
    template <typename T>
    class generator
    {
    public:
        class promise_type
        {
        public:
            promise_type() = default;

            generator<T> get_return_object() noexcept
            {
                return generator<T>(std::coroutine_handle<promise_type>::from_promise(*this));
            }

            std::suspend_always initial_suspend() const noexcept
            {
                return {};
            }

            std::suspend_always final_suspend() const noexcept
            {
                return {};
            }

            void unhandled_exception() noexcept
            {
                _exception = std::current_exception();
            }

            void rethrow_if_unhandled_exception() const
            {
                if (_exception)
                {
                    std::rethrow_exception(_exception);
                }
            }

            void return_void() {}

            std::suspend_always yield_value(const T &value) noexcept
            {
                _value = value;
                return {};
            }

            std::suspend_always yield_value(T &&value) noexcept
            {
                _value = std::move(value);
                return {};
            }

            T &get_value() noexcept
            {
                return _value;
            }

        private:
            T _value;
            std::exception_ptr _exception;
        };

        class iterator
        {
        public:
            iterator() = default;


            iterator(std::coroutine_handle<promise_type> handle) noexcept
                : _handle(handle)
            {
            }

            bool operator==(const std::default_sentinel_t &) const noexcept
            {
                return !_handle || _handle.done();
            }

            bool operator!=(const std::default_sentinel_t &sent) const noexcept
            {
                return !(*this == sent);
            }

            iterator &operator++() noexcept
            {
                _handle.resume();
                _handle.promise().rethrow_if_unhandled_exception();
                return *this;
            }

            T &operator*() const noexcept
            {
                return _handle.promise().get_value();
            }

            T *operator->() const noexcept
            {
                return std::addressof(_handle.promise().get_value());
            }

        private:
            std::coroutine_handle<promise_type> _handle;
        };

        generator() = default;

        generator(std::coroutine_handle<promise_type> h) noexcept
            : _handle(h)
        {
        }

        generator(const std::ranges::range auto &range) noexcept
        {
            *this = [](const std::ranges::range auto &range_) -> generator<T>
            {
                for (const auto &value : range_)
                {
                    co_yield value;
                }
            }(range);
        }

        generator(std::ranges::range auto &&range) noexcept
        {
            *this = [](std::ranges::range auto range_) -> generator<T>
            {
                auto iter = std::make_move_iterator(std::ranges::begin(range_));
                auto end = std::make_move_iterator(std::ranges::end(range_));
                while (iter != end)
                {
                    co_yield std::move(*iter++);
                }
            }(std::move(range));
        }

        generator(generator &&other) noexcept
            : _handle(std::exchange(other._handle, nullptr))
        {
        }

        generator<T> &operator=(generator &&other) noexcept
        {
            _handle = std::exchange(other._handle, nullptr);
            return *this;
        }

        iterator begin() const noexcept
        {
            _handle.resume();
            _handle.promise().rethrow_if_unhandled_exception();
            return iterator(_handle);
        }

        std::default_sentinel_t end() const noexcept
        {
            return {};
        }

        bool all(std::invocable<T &&> auto &&pred) const
        {
            for (auto &&value : *this)
            {
                if (!pred(std::move(value)))
                {
                    return false;
                }
            }
            return true;
        }

        bool any(std::invocable<T &&> auto &&pred) const
        {
            for (auto &&value : *this)
            {
                if (pred(std::move(value)))
                {
                    return true;
                }
            }
            return false;
        }

        generator<T> append(const T &value)
        {
            return [](generator<T> gen_, T value_) -> generator<T>
            {
                for (auto &&v : gen_)
                {
                    co_yield v;
                }
                co_yield value_;
            }(std::move(*this), value);
        }

        generator<T> append(T &&value)
        {
            return [](generator<T> gen_, T value_) -> generator<T>
            {
                for (auto &&v : gen_)
                {
                    co_yield v;
                }
                co_yield value_;
            }(std::move(*this), std::move(value));
        }

        generator<T> append(generator<T> &&other)
        {
            return [](generator<T> gen_, generator<T> other_) -> generator<T>
            {
                for (auto &&v : gen_)
                {
                    co_yield v;
                }
                for (auto &&v : other_)
                {
                    co_yield v;
                }
            }(std::move(*this), std::move(other));
        }

        template <std::integral U = T>
        double average()
        {
            std::size_t count = 0;
            double sum = 0;
            for (auto &&value : *this)
            {
                sum += value;
                ++count;
            }
            return sum / count;
        }

        generator<std::vector<T>> chunk(std::size_t size)
        {
            return [](generator<T> gen_, std::size_t size_) -> generator<std::vector<T>>
            {
                std::vector<T> result;
                result.reserve(size_);
                for (auto &&v : gen_)
                {
                    result.push_back(v);
                    if (result.size() == size_)
                    {
                        co_yield std::move(result);
                    }
                }
            }(std::move(*this), size);
        }

        bool contains(const T &value) const
        {
            for (auto &&v : *this)
            {
                if (v == value)
                {
                    return true;
                }
            }
        }

        std::size_t count() const
        {
            std::size_t result = 0;
            for (auto &&_ : *this)
            {
                ++result;
            }
            return result;
        }

        generator<T> distinct()
        {
            return [](generator<T> gen_) -> generator<T>
            {
                std::set<T> seen;
                for (auto &&v : gen_)
                {
                    if (seen.insert(v).second)
                    {
                        co_yield v;
                    }
                }
            }(std::move(*this));
        }

        T element_at(std::size_t index) const
        {
            std::size_t i = 0;
            for (auto &&v : *this)
            {
                if (i == index)
                {
                    return v;
                }
                ++i;
            }

            throw std::out_of_range("element_at");
        }

        T first() const
        {
            return *begin();
        }

        T last() const
        {
            auto it = std::begin(*this);
            auto end = std::end(*this);
            while (it != end)
            {
                ++it;
                if (it == end)
                {
                    return *it;
                }
            }

            throw std::out_of_range("last");
        }

        generator<T> prepend(const T &value)
        {
            return [](generator<T> gen_, T value_) -> generator<T>
            {
                co_yield value_;
                for (auto &&v : gen_)
                {
                    co_yield v;
                }
            }(std::move(*this), value);
        }

        generator<T> prepend(T &&value)
        {
            return [](generator<T> gen_, T value_) -> generator<T>
            {
                co_yield value_;
                for (auto &&v : gen_)
                {
                    co_yield v;
                }
            }(std::move(*this), std::move(value));
        }

        generator<T> prepend(generator<T> &&other)
        {
            return [](generator<T> gen_, generator<T> other_) -> generator<T>
            {
                for (auto &&v : other_)
                {
                    co_yield v;
                }
                for (auto &&v : gen_)
                {
                    co_yield v;
                }
            }(std::move(*this), std::move(other));
        }

        generator<T> range(T from, T to)
        {
            for (auto i = from; i <= to; ++i)
            {
                co_yield i;
            }
        }

        generator<T> repeat(const T &value, std::size_t count)
        {
            for (int i = 0; i <= count; ++i)
            {
                co_yield value;
            }
        }

        generator<T> reverse()
        {
            return [](generator<T> gen_) -> generator<T>
            {
                std::vector<T> result;

                for (auto &&v : gen_)
                {
                    result.push_back(v);
                }

                auto it = std::rbegin(result);
                auto end = std::rend(result);
                while (it != end)
                {
                    co_yield *it;
                    ++it;
                }
            }(std::move(*this));
        }

        template <typename Selector, typename ResultType = std::invoke_result_t<Selector, T &&>>
        generator<ResultType> select(Selector &&selector)
        {
            return [](generator<T> gen_, Selector &&selector_) -> generator<ResultType>
            {
                for (auto &&v : gen_)
                {
                    co_yield selector_(std::move(v));
                }
            }(std::move(*this), std::move(selector));
        }

        generator<T> skip(std::size_t count)
        {
            return [](generator<T> gen_, std::size_t count_) -> generator<T>
            {
                for (auto &&v : gen_)
                {
                    if (count_ > 0)
                    {
                        --count_;
                    }
                    else
                    {
                        co_yield std::move(v);
                    }
                }
            }(std::move(*this), count);
        }

        generator<T> skip_while(std::invocable<const T &> auto &&predicate)
        {
            return [](generator<T> gen_, std::invocable<const T &> auto &&predicate_) -> generator<T>
            {
                bool skip = true;
                for (auto &&v : gen_)
                {
                    if (skip && predicate_(v))
                    {
                        continue;
                    }
                    skip = false;
                    co_yield std::move(v);
                }
            }(std::move(*this), predicate);
        }

        generator<T> where(std::invocable<const T &> auto &&predicate)
        {
            return [](generator<T> gen_, auto &&predicate_) -> generator<T>
            {
                for (auto &&v : gen_)
                {
                    if (predicate_(v))
                    {
                        co_yield std::move(v);
                    }
                }
            }(std::move(*this), predicate);
        }

        template <typename ExecutionMode = std::execution::sequenced_policy>
        void for_each(std::invocable<T &&> auto &&func)
        {
            if constexpr (std::is_same_v<ExecutionMode, std::execution::parallel_unsequenced_policy>)
            {
                std::vector<task<void>> tasks;
                for (auto &&v : *this)
                {
                    tasks.emplace_back(std::move(task<void>::run(func, v)));
                }

                task<void>::when_all(tasks).wait();
            }
            else if constexpr (std::is_same_v<ExecutionMode, std::execution::sequenced_policy>)
            {
                for (auto &&v : *this)
                {
                    func(std::move(v));
                }
            }
            else
            {
                static_assert(false, "Invalid execution mode");
            }
        }

        std::vector<T> to_vector()
        {
            std::vector<T> result;
            for (auto &&v : *this)
            {
                result.push_back(std::move(v));
            }
            return result;
        }

        ~generator() noexcept
        {
            if (_handle)
            {
                _handle.destroy();
            }
        }

    private:
        std::coroutine_handle<promise_type> _handle;
    };
}