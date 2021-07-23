#pragma once
#include <queue>
#include <mutex>
#include <optional>
#include "task.hpp"

namespace coroutines
{
    template <class T>
    class unbounded_channel;

    template <class T>
    class bounded_channel;

    template <class T>
    class channel
    {
    public:
        channel() = default;
        channel(const channel &) = delete;
        channel(channel &&) = delete;

        virtual bool try_write(T const &value) = 0;
        virtual bool try_write(T &&value) = 0;

        task<bool> try_write_async(T const &value)
        {
            co_return this->try_write(value);
        }

        task<bool> try_write_async(T &&value)
        {
            co_return this->try_write(std::move(value));
        }

        std::optional<T> try_read()
        {
            const std::lock_guard<std::mutex> lock(this->_mutex);
            if (this->_queue.empty() || this->closed())
                return std::nullopt;

            auto value = this->_queue.front();
            this->_queue.pop();
            return value;
        }

        task<std::optional<T>> try_read_async()
        {
            co_return this->try_read();
        }

        bool closed() const noexcept
        {
            return this->_closed;
        }

        void close() noexcept
        {
            this->_closed = true;
            this->_wait_condition.notify_all();
        }

        static std::unique_ptr<channel<T>> create_unbounded()
        {
            return std::make_unique<unbounded_channel<T>>();
        }

        static std::unique_ptr<channel<T>> create_bounded(size_t size)
        {
            return std::make_unique<bounded_channel<T>>(size);
        }

        std::optional<T> wait()
        {
            std::unique_lock<std::mutex> lock(this->_mutex);
            this->_wait_condition.wait(lock, [&]()
                                       { return !this->_queue.empty() || this->closed(); });

            if (this->closed())
                return std::nullopt;

            auto value = this->_queue.front();
            this->_queue.pop();
            return value;
        }

        task<std::optional<T>> wait_async()
        {
            co_return this->wait();
        }

        virtual ~channel() = default;

        class iterator
        {
        public:
            iterator(channel<T> &channel) : _channel(channel) {}

            T &operator*()
            {
                return this->_value;
            }

            T *operator->() const
            {
                return &this->_value;
            }

            iterator &operator++()
            {
                this->_value = this->_channel.wait().value_or(T{});
                return *this;
            }

            friend bool operator==(const iterator &it, std::default_sentinel_t s) noexcept
            {
                return it._channel.closed();
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
            channel<T> &_channel;
            T _value;

            friend class channel<T>;
        };

        iterator begin()
        {
            iterator it(*this);
            it._value = this->wait().value_or(T{});
            return it;
        }

        std::default_sentinel_t end()
        {
            return {};
        }

    protected:
        std::queue<T> _queue;
        std::mutex _mutex;
        std::condition_variable _wait_condition;
        std::atomic<bool> _closed;
    };

    template <class T>
    class unbounded_channel final : public channel<T>
    {
    public:
        unbounded_channel() = default;
        unbounded_channel(const unbounded_channel &) = delete;
        unbounded_channel(unbounded_channel &&) = delete;

        virtual bool try_write(T const &value) override
        {
            std::unique_lock<std::mutex> lock(this->_mutex);
            if (this->closed())
                return false;

            this->_queue.push(value);
            lock.unlock();
            this->_wait_condition.notify_one();
            return true;
        }

        virtual bool try_write(T &&value) override
        {
            std::unique_lock<std::mutex> lock(this->_mutex);
            this->_queue.push(std::move(value));
            lock.unlock();
            this->_wait_condition.notify_one();
            return true;
        }
    };

    template <class T>
    class bounded_channel final : public channel<T>
    {
    public:
        bounded_channel(size_t capacity) : _capacity(capacity)
        {
        }

        bounded_channel() = default;
        bounded_channel(const bounded_channel &) = delete;
        bounded_channel(bounded_channel &&) = delete;

        virtual bool try_write(T const &value) override
        {
            std::unique_lock<std::mutex> lock(this->_mutex);
            if (this->_queue.size() == this->_capacity || this->closed())
                return false;
            this->_queue.push(value);
            lock.unlock();
            this->_wait_condition.notify_one();
            return true;
        }

        virtual bool try_write(T &&value) override
        {
            std::unique_lock<std::mutex> lock(this->_mutex);
            if (this->_queue.size() == this->_capacity)
                return false;
            this->_queue.push(std::move(value));
            lock.unlock();
            this->_wait_condition.notify_one();
            return true;
        }

    private:
        size_t _capacity;
    };
}