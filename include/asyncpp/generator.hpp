#pragma once
#include <concepts>
#include <coroutine>
#include <stdexcept>
#include <iterator>
#include <vector>
#include <set>
#include <execution>
#include "task.hpp"
#include <iostream>
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

            generator<T> get_return_object() noexcept;

            std::suspend_always initial_suspend() const noexcept;

            std::suspend_always final_suspend() const noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;

            void return_void();

            std::suspend_always yield_value(const T &value) noexcept;

            std::suspend_always yield_value(T &&value) noexcept;

            T &get_value() noexcept;

        private:
            T _value;
            std::exception_ptr _exception;
        };

        class iterator
        {
        public:
            iterator() = default;

            iterator(std::coroutine_handle<promise_type> handle) noexcept;

            iterator(const iterator &) = default;

            iterator(iterator &&) = default;

            iterator &operator=(const iterator &) = default;

            iterator &operator=(iterator &&) = default;

            bool operator==(const std::default_sentinel_t &) const noexcept;

            bool operator!=(const std::default_sentinel_t &) const noexcept;

            iterator &operator++() noexcept;

            T &operator*() const noexcept;

            T *operator->() const noexcept;

        private:
            std::coroutine_handle<promise_type> _handle;
        };

        generator() = default;

        generator(std::coroutine_handle<promise_type> h) noexcept;

        generator(const std::ranges::range auto &range) noexcept;

        generator(std::ranges::range auto &&range) noexcept;

        generator(generator &&other) noexcept;

        generator<T> &operator=(generator &&other) noexcept;

        iterator begin() const noexcept;

        std::default_sentinel_t end() const noexcept;

        bool all(std::invocable<T &&> auto &&pred) const;

        bool any(std::invocable<T &&> auto &&pred) const;

        generator<T> append(const T &value);

        generator<T> append(T &&value);

        generator<T> append(generator<T> &&other);

        template <std::integral U = T>
        double average();

        generator<std::vector<T>> chunk(std::size_t size);

        // template <typename = std::enable_if_t<std::is_integral_v<T>>>
        // generator<T> concat(const generator<T> &other) const;

        bool contains(const T &value) const;

        template <std::integral Integral = std::size_t>
        Integral count() const;

        generator<T> distinct();

        T element_at(std::size_t index) const;

        T first() const;

        // TODO: group_by

        // TODO: group_join

        // generator<T> intersect(const generator<T> &other) const;

        // TODO: join

        T last() const;

        // TODO: max

        // TODO: min

        // TODO: order_by

        // TODO: order_by_descending

        generator<T> prepend(const T &value);

        generator<T> prepend(T &&value);

        generator<T> prepend(generator<T> &&other);

        static generator<T> range(T from, T to);

        static generator<T> repeat(const T &value, std::size_t count);

        generator<T> reverse();

        template <typename Selector, typename ResultType = std::invoke_result_t<Selector, T &&>>
        generator<ResultType> select(Selector &&selector);

        generator<T> skip(std::size_t count);

        generator<T> skip_while(std::invocable<const T &> auto &&pred);

        generator<T> where(std::invocable<const T &> auto &&pred);

        template <auto ExecutionMode = std::execution::seq>
        void for_each(std::invocable<T &&> auto &&func);

        std::vector<T> to_vector();

        ~generator() noexcept;

    private:
        std::coroutine_handle<promise_type> _handle;
    };

    template <typename T>
    generator<T> generator<T>::promise_type::get_return_object() noexcept
    {
        return generator<T>(std::coroutine_handle<promise_type>::from_promise(*this));
    }

    template <typename T>
    std::suspend_always generator<T>::promise_type::initial_suspend() const noexcept
    {
        return {};
    }

    template <typename T>
    std::suspend_always generator<T>::promise_type::final_suspend() const noexcept
    {
        return {};
    }

    template <typename T>
    void generator<T>::promise_type::unhandled_exception() noexcept
    {
        _exception = std::current_exception();
    }

    template <typename T>
    void generator<T>::promise_type::rethrow_if_unhandled_exception() const
    {
        if (_exception)
        {
            std::rethrow_exception(_exception);
        }
    }

    template <typename T>
    void generator<T>::promise_type::return_void() {}

    template <typename T>
    std::suspend_always generator<T>::promise_type::yield_value(const T &value) noexcept
    {
        _value = value;
        return {};
    }

    template <typename T>
    std::suspend_always generator<T>::promise_type::yield_value(T &&value) noexcept
    {
        _value = std::move(value);
        return {};
    }

    template <typename T>
    T &generator<T>::promise_type::get_value() noexcept
    {
        return _value;
    }

    template <typename T>
    generator<T>::iterator::iterator(std::coroutine_handle<promise_type> handle) noexcept
        : _handle(handle)
    {
    }

    template <typename T>
    bool generator<T>::iterator::operator==(const std::default_sentinel_t &) const noexcept
    {
        return !_handle || _handle.done();
    }

    template <typename T>
    bool generator<T>::iterator::operator!=(const std::default_sentinel_t &sent) const noexcept
    {
        return !(*this == sent);
    }

    template <typename T>
    generator<T>::iterator &generator<T>::iterator::operator++() noexcept
    {
        _handle.resume();
        _handle.promise().rethrow_if_unhandled_exception();
        return *this;
    }

    template <typename T>
    T &generator<T>::iterator::operator*() const noexcept
    {
        return _handle.promise().get_value();
    }

    template <typename T>
    T *generator<T>::iterator::operator->() const noexcept
    {
        return std::addressof(_handle.promise().get_value());
    }

    template <typename T>
    generator<T>::generator(std::coroutine_handle<promise_type> h) noexcept
        : _handle(h)
    {
    }

    template <typename T>
    generator<T>::generator(const std::ranges::range auto &range) noexcept
    {
        *this = [](const std::ranges::range auto &range_) -> generator<T>
        {
            for (const auto &value : range_)
            {
                co_yield value;
            }
        }(range);
    }

    template <typename T>
    generator<T>::generator(std::ranges::range auto &&range) noexcept
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

    template <typename T>
    generator<T>::generator(generator &&other) noexcept
        : _handle(std::exchange(other._handle, nullptr))
    {
    }

    template <typename T>
    generator<T> &generator<T>::operator=(generator &&other) noexcept
    {
        _handle = std::exchange(other._handle, nullptr);
        return *this;
    }

    template <typename T>
    generator<T>::iterator generator<T>::begin() const noexcept
    {
        _handle.resume();
        _handle.promise().rethrow_if_unhandled_exception();
        return iterator(_handle);
    }

    template <typename T>
    std::default_sentinel_t generator<T>::end() const noexcept
    {
        return {};
    }

    template <typename T>
    bool generator<T>::all(std::invocable<T &&> auto &&pred) const
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

    template <typename T>
    bool generator<T>::any(std::invocable<T &&> auto &&pred) const
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

    template <typename T>
    generator<T> generator<T>::append(const T &value)
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

    template <typename T>
    generator<T> generator<T>::append(T &&value)
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

    template <typename T>
    generator<T> generator<T>::append(generator<T> &&other)
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

    template <typename T>
    template <std::integral U = T>
    double generator<T>::average()
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

    template <typename T>
    generator<std::vector<T>> generator<T>::chunk(std::size_t size)
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

    template <typename T>
    bool generator<T>::contains(const T &value) const
    {
        for (auto &&v : *this)
        {
            if (v == value)
            {
                return true;
            }
        }
    }

    template <typename T>
    template <std::integral Integral>
    Integral generator<T>::count() const
    {
        Integral result = 0;
        for (auto &&_ : *this)
        {
            ++result;
        }
        return result;
    }

    template <typename T>
    generator<T> generator<T>::distinct()
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

    template <typename T>
    T generator<T>::element_at(std::size_t index) const
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

    template <typename T>
    T generator<T>::first() const
    {
        return *begin();
    }

    template <typename T>
    T generator<T>::last() const
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

    template <typename T>
    generator<T> generator<T>::prepend(const T &value)
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

    template <typename T>
    generator<T> generator<T>::prepend(T &&value)
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

    template <typename T>
    generator<T> generator<T>::prepend(generator<T> &&other)
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

    template <typename T>
    generator<T> generator<T>::range(T from, T to)
    {
        for (auto i = from; i <= to; ++i)
        {
            co_yield i;
        }
    }

    template <typename T>
    generator<T> generator<T>::repeat(const T &value, std::size_t count)
    {
        for (int i = 0; i <= count; ++i)
        {
            co_yield value;
        }
    }

    template <typename T>
    generator<T> generator<T>::reverse()
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

    template <typename T>
    template <typename Selector, typename ResultType = std::invoke_result_t<Selector, T &&>>
    generator<ResultType> generator<T>::select(Selector &&selector)
    {
        return [](generator<T> gen_, Selector &&selector_) -> generator<ResultType>
        {
            for (auto &&v : gen_)
            {
                co_yield selector_(std::move(v));
            }
        }(std::move(*this), std::move(selector));
    }

    template <typename T>
    generator<T> generator<T>::skip(std::size_t count)
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

    template <typename T>
    generator<T> generator<T>::skip_while(std::invocable<const T &> auto &&predicate)
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

    template <typename T>
    generator<T> generator<T>::where(std::invocable<const T &> auto &&predicate)
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

    template <typename T>
    template <auto ExecutionMode>
    void generator<T>::for_each(std::invocable<T &&> auto &&func)
    {
        if constexpr (std::is_same_v<decltype(ExecutionMode), std::execution::parallel_policy>)
        {
            std::vector<task<void>> tasks;
            for (auto &&v : *this)
            {
                tasks.emplace_back(std::move(task<void>::run(func, v)));
            }

            task<void>::when_all(tasks).wait();
        }
        else if (std::is_same_v<decltype(ExecutionMode), std::execution::sequenced_policy>)
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

    template <typename T>
    std::vector<T> generator<T>::to_vector()
    {
        std::vector<T> result;
        for (auto &&v : *this)
        {
            result.push_back(std::move(v));
        }
        return result;
    }

    template <typename T>
    generator<T>::~generator() noexcept
    {
        if (_handle)
        {
            _handle.destroy();
        }
    }
}