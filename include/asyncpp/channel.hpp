#pragma once
#include <optional>
#include <memory>
#include <semaphore>
#include "queue.hpp"
#include "channel_exceptions.hpp"

namespace async
{
    template <typename T>
    class unbounded_channel;

    template <typename T>
    class channel
    {
    public:
        class reader;
        class writer;

        virtual bool write(T &&data) = 0;
        virtual bool write(T const &data) = 0;

        virtual T read() = 0;

        virtual std::size_t size() const = 0;

        void close()
        {
            this->_closed = true;
        }
        
        bool closed() const
        {
            return this->_closed;
        }

        static std::shared_ptr<channel<T>> create_unbounded()
        {
            return std::make_shared<unbounded_channel<T>>();
        }

    protected:
        std::atomic_bool _closed{false};
    };

    template <typename T>
    class unbounded_channel : public channel<T>
    {
    public:
        unbounded_channel() : _queue(queue<T>()) {}

        bool write(T &&data) override
        {
            if(this->closed())
                throw channel_closed_exception();
            
            _queue.push(data);
            _semaphore.release();
            return true;
        }

        bool write(const T &data) override
        {
            _queue.push(data);
            _semaphore.release();
            return true;
        }

        T read() override
        {
            if(this->closed())
                throw channel_closed_exception();

            _semaphore.acquire();
            try
            {
                auto result = _queue.pop();

                // Let another reader know there's more data.
                if (!_queue.empty())
                    _semaphore.release();

                return result;
            }
            catch(const queue_empty_exception&)
            {
                throw channel_empty_exception();
            }
        }

        std::size_t size() const override
        {
            return _queue.size();
        }
        
    private:
        queue<T> _queue;
        std::binary_semaphore _semaphore{0};
    };
}