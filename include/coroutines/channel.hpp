#pragma once
#include <queue>
#include <mutex>
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
        channel(channel &&) = default;

        channel &operator=(const channel &) = delete;
        channel &operator=(channel &&) = default;

        virtual bool try_write(T const &value) = 0;
        virtual bool try_write(T &&value) = 0;

        virtual task<bool> try_write_async(T const &value) = 0;
        virtual task<bool> try_write_async(T &&value) = 0;

        virtual bool try_read(T &value) = 0;
        virtual task<bool> try_read_async(T &value) = 0;

        virtual T wait() = 0;
        virtual task<T> wait_async() = 0;

        static std::unique_ptr<channel<T>> make_unbounded()
        {
            return std::unique_ptr<channel<T>>(new unbounded_channel<T>());
        }

        static std::unique_ptr<channel<T>> make_bounded(size_t capacity)
        {
            return std::unique_ptr<channel<T>>(new bounded_channel<T>(capacity));
        }

        virtual ~channel() = default;
    };

    template <class T>
    class unbounded_channel final : public channel<T>
    {
    public:
        unbounded_channel() = default;

        unbounded_channel(unbounded_channel const &) = delete;
        unbounded_channel(unbounded_channel &&) = delete;

        unbounded_channel &operator=(unbounded_channel const &) = delete;
        unbounded_channel &operator=(unbounded_channel &&) = delete;

        bool try_write(T const &value) override
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->queue.push(value);
            lock.unlock();
            this->condition_variable.notify_one();
            return true;
        }

        bool try_write(T &&value) override
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->queue.push(std::move(value));
            lock.unlock();
            this->condition_variable.notify_one();
            return true;
        }

        task<bool> try_write_async(T const &value) override
        {
            co_return this->try_write(value);
        }

        task<bool> try_write_async(T &&value) override
        {
            co_return this->try_write(std::move(value));
        }

        bool try_read(T &value) override
        {
            const std::lock_guard<std::mutex> lock(this->mutex);
            if (this->queue.empty())
                return false;

            value = this->queue.front();
            this->queue.pop();
            return true;
        }

        task<bool> try_read_async(T &value) override
        {
            co_return this->try_read(value);
        }

        T wait() override
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->condition_variable.wait(lock, [&]()
                                          { return !this->queue.empty(); });
            auto value = this->queue.front();
            this->queue.pop();
            return value;
        }

        task<T> wait_async() override
        {
            co_return this->wait();
        }

        ~unbounded_channel() = default;

    private:
        std::condition_variable condition_variable;
        std::queue<T> queue;
        std::mutex mutex;
    };

    template <class T>
    class bounded_channel final : public channel<T>
    {
    public:
        bounded_channel(size_t capacity)
            : capacity(capacity)
        {
        }

        bounded_channel(bounded_channel const &) = delete;
        bounded_channel(bounded_channel &&) = delete;

        bounded_channel &operator=(bounded_channel const &) = delete;
        bounded_channel &operator=(bounded_channel &&) = delete;

        bool try_write(T const &value) override
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            if (this->queue.size() >= this->capacity)
                return false;

            this->queue.push(value);
            lock.unlock();
            this->condition_variable.notify_one();
            return true;
        }

        bool try_write(T &&value) override
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            if (this->queue.size() >= this->capacity)
                return false;

            this->queue.push(std::move(value));
            lock.unlock();
            this->condition_variable.notify_one();
            return true;
        }

        task<bool> try_write_async(T const &value) override
        {
            co_return this->try_write(value);
        }

        task<bool> try_write_async(T &&value) override
        {
            co_return this->try_write(std::move(value));
        }

        bool try_read(T &value) override
        {
            const std::lock_guard<std::mutex> lock(this->mutex);
            if (this->queue.empty())
                return false;

            value = this->queue.front();
            this->queue.pop();
            return true;
        }
        
        task<bool> try_read_async(T &value) override
        {
            co_return this->try_read(value);
        }

        T wait() override
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->condition_variable.wait(lock, [&]()
                                          { return !this->queue.empty(); });
            auto value = this->queue.front();
            this->queue.pop();
            return value;
        }

        task<T> wait_async() override
        {
            co_return this->wait();
        }

        ~bounded_channel() = default;

    private:
        std::queue<T> queue;
        std::condition_variable condition_variable;
        std::mutex mutex;
        std::size_t capacity;
    };
}