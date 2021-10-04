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
        channel() = default;

        virtual void write(T &&data) = 0;
        virtual void write(T const &data) = 0;

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
        
        /**
         * @brief Creates a shared pointer to a unbounded channel.
         * @return A shared pointer to a unbounded channel.
         */
        static std::shared_ptr<channel<T>> create_unbounded()
        {
            return std::make_shared<unbounded_channel<T>>();
        }

    protected:
        std::atomic_bool _closed;
    };

    template <typename T>
    class unbounded_channel final : public channel<T>
    {
    public:
        unbounded_channel() : _queue(queue<T>()) {}

        /**
         * @brief Writes data to the channel.
         * @param[in] data The data to write.
         * @return True if the data was written, false otherwise.
         * @throws channel_closed_exception If the channel is closed.
         */
        void write(T &&data) override
        {
            if (this->closed())
                throw channel_closed_exception();

            _queue.push(data);
            _semaphore.release();
        }

        /**
         * @brief Writes data to the channel.
         * @param[in] data The data to write.
         * @return True if the data was written, false otherwise.
         * @throws channel_closed_exception If the channel is closed.
         */
        void write(const T &data) override
        {
            if (this->closed())
                throw channel_closed_exception();

            _queue.push(data);
            _semaphore.release();
        }

        /**
         * @brief Reads data from the channel.
         * @return The data read.
         * @throws channel_closed_exception If the channel is closed.
         * @throws channel_empty_exception If the channel is empty.
         */
        T read() override
        {
            if (this->closed() && _queue.empty())
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
            catch (const queue_empty_exception &)
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