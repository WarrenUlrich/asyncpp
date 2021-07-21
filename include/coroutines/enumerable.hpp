#pragma once
#include <coroutine>
#include <vector>

namespace coroutines
{
    template<class ReturnType = void>
    class enumerable
    {
    public:
        class promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        class promise_type
        {
        public:
            ReturnType current_value{};

            auto get_return_object()
            {
                return enumerable{ handle_type::from_promise(*this) };
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
                // TODO:
            }

            void return_void()
            {

            }

            auto yield_value(ReturnType& value) noexcept
            {
                current_value = std::move(value);
                return std::suspend_always{};
            }

            auto yield_value(ReturnType&& value) noexcept
            {
                return yield_value(value);
            }
        };

        class iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = ReturnType;
            using pointer = ReturnType*;
            using reference = ReturnType&;
        private:
            handle_type handle;

        public:
            iterator(handle_type handle)
                : handle(handle)
            {

            }

            reference operator*() const
            {
                return this->handle.promise().current_value;
            }

            pointer operator->()
            {
                return &this->handle.promise().current_value;
            }

            iterator& operator++()
            {
                this->handle.resume();
                return *this;
            }

            friend bool operator==(const iterator& it, std::default_sentinel_t s) noexcept
            {
                return !it.handle || it.handle.done();
            }

            friend bool operator!=(const iterator& it, std::default_sentinel_t s) noexcept
            {
                return !(it == s);
            }

            friend bool operator==(std::default_sentinel_t s, const iterator& it) noexcept
            {
                return (it == s);
            }

            friend bool operator!=(std::default_sentinel_t s, const iterator& it) noexcept
            {
                return it != s;
            }
        };

        enumerable() = delete;

        enumerable(handle_type h)
            : handle(h)
        {
            
        };

        enumerable(enumerable<ReturnType>&& other) noexcept
            : handle(other.handle)
        {
            other.handle = nullptr;
        }

        enumerable(const enumerable<ReturnType>&&) = delete;

        iterator begin()
        {
            if(this->handle)
                if(!this->handle.done())
                    this->handle.resume();

            return iterator(this->handle);
        }

        std::default_sentinel_t end()
        {
            return {};
        }

        // Filters a sequence of values based on a predicate.
        template<class Predicate>
        enumerable<ReturnType> where(Predicate&& pred)
        {
            return where(std::move(*this), pred);
        }

        // Maps each element of a sequence into a new form.
        template<class Mapper>
        enumerable<std::invoke_result_t<Mapper, ReturnType&>> map(Mapper&& m)
        {
            return map(std::move(*this), m);
        }

        // Return the first element of a sequence.
        ReturnType first()
        {
            for (auto& i : *this)
                return i;
        }

        // Returns the first element of a sequence that satisfies a predicate.
        template<class Predicate>
        ReturnType first(Predicate&& pred)
        {
            return first(std::move(*this), pred);
        }

        std::size_t count()
        {
            std::size_t temp{};
            for (auto& i : *this)
                temp++;
            return temp;
        }

        std::vector<ReturnType> collect()
        {
            std::vector<ReturnType> result{};
            for (const auto& i : *this)
                result.emplace_back(i);
            return result;
        }

        private:
            handle_type handle;

            ~enumerable()
            {
                if(this->handle)
                    this->handle.destroy();
            }

            template<class Predicate>
            static enumerable<ReturnType> where(enumerable<ReturnType> e, Predicate&& pred)
            {
                for (auto& i : e)
                    if (pred(i))
                        co_yield i;
            }

            //Maps each element of a sequence into a new form.
            template<class Mapper>
            static enumerable<std::invoke_result_t<Mapper, ReturnType&>> map(enumerable<ReturnType> e, Mapper&& m)
            {
                for (auto& i : e)
                    co_yield m(i);
            }

            template<class Predicate>
            static ReturnType first(enumerable<ReturnType> e, Predicate&& pred)
            {
                for (auto& i : e)
                    if (pred(i))
                        return i;
            }
    };

    template<>
    class enumerable<void>
    {
    public:
        template<std::integral Integral>
        static enumerable<Integral> range(Integral to, Integral from)
        {
            for (Integral i = to; i <= from; ++i)
                co_yield i;
        }

        template<std::ranges::range R>
        static enumerable<std::ranges::range_value_t<R>> from(R& range)
        {
            for (auto& i : range)
                co_yield i;
        }
    };
}