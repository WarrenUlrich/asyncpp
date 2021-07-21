#pragma once
#include <queue>
#include <mutex>

namespace coroutines 
{
    template<class T>
    class unbounded_channel;

    template<class T>
    class bounded_channel;

    template <class T>
    class channel
    {
    public:
        virtual bool try_write(T const& value) = 0;
        virtual bool try_read(T& value) = 0;
        virtual T wait() = 0;
        
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
        bool try_write(T const& value)
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->queue.push(value);
            lock.unlock();
            this->condition_variable.notify_one();
            return true;
        }

        bool try_read(T& value)
        {
            const std::lock_guard<std::mutex> lock(this->mutex);
            if (this->queue.empty())
                return false;

            value = this->queue.front();
            this->queue.pop();
            return true;
        }

        T wait()
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->condition_variable.wait(lock, [&]() { return !this->queue.empty(); });
            auto value = this->queue.front();
            this->queue.pop();
            return value;
        }

        ~unbounded_channel() = default;
    private:
        std::condition_variable condition_variable;
        std::queue<T> queue;
        std::mutex mutex;
    };

    template<class T>
    class bounded_channel final : public channel<T>
    {
    public:
        bounded_channel(size_t capacity)
            : capacity(capacity)
            {
                
            }

        bool try_write(T const& value)
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            if (this->queue.size() >= this->capacity)
                return false;

            this->queue.push(value);
            lock.unlock();
            this->condition_variable.notify_one();
            return true;
        }

        bool try_read(T& value)
        {
            const std::lock_guard<std::mutex> lock(this->mutex);
            if (this->queue.empty())
                return false;

            value = this->queue.front();
            this->queue.pop();
            return true;
        }

        T wait()
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->condition_variable.wait(lock, [&]() { return !this->queue.empty(); });
            auto value = this->queue.front();
            this->queue.pop();
            return value;
        }
        
    private:
        std::queue<T> queue;
        std::condition_variable condition_variable;
        std::mutex mutex;
        std::size_t capacity;
    };
}