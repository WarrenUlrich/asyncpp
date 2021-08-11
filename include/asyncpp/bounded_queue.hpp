#pragma once
#include <optional>
#include <atomic>
#include <iostream>
#include "queue_exceptions.hpp"

namespace async
{
    /**
     * @brief A bounded queue (FIFO).
     * @details This queue is thread-safe and "lock-free", and is implemented using a circular buffer.
     */
    template <typename T, std::size_t NodeCapacity>
    class bounded_queue
    {
    public:
        bounded_queue() : _head(0), _tail(0) {}
        
        /**
         * @brief Pushes a value to the queue, throws queue_full_exception if full.
         * @param[in] item The value to push.
         * @return A boolean value indicating whether the operation was successful, the operation is only unsuccessful if the queue is full.
         * @throw queue_full_exception when the queue is full.
         */
        void push(T &&item)
        {
            auto tail = _tail.load();

            // full
            if ((tail + 1) % NodeCapacity == _head.load())
                throw queue_full_exception();

            // retry if we couldn't update the tail in time.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % NodeCapacity))
                return push(std::forward<T>(item));

            _data[tail] = std::move(item);
        }

        /**
         * @brief Pushes a value to the queue, throws queue_full_exception if full.
         * @param[in] item The value to push.
         * @return A boolean value indicating whether the operation was successful, the operation is only unsuccessful if the queue is full.
         * @throw queue_full_exception when the queue is full.
         */
        void push(T const &item)
        {
            auto tail = _tail.load();

            // full
            if ((tail + 1) % NodeCapacity == _head.load())
                throw queue_full_exception();

            // retry if we couldn't update the tail in time.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % NodeCapacity))
                return try_push(item);

            _data[tail] = item;
        }

        /**
         * @return A optional<T> value that contains the value that was popped, if the queue is empty, it's a nullopt.
         * @throw queue_empty if the queue is empty.
         */
        T pop()
        {
            auto head = _head.load();

            // empty
            if (head == _tail.load())
                throw queue_empty_exception();

            // retry if we couldn't update the head in time.
            if (!_head.compare_exchange_strong(head, (head + 1) % NodeCapacity))
                return pop();

            auto item = _data[head];
            return item;
        }

        std::size_t size() const
        {
            auto tail = _tail.load();
            auto head = _head.load();

            if (head > tail)
                return head - tail;
            else if (head == tail)
                return 0;
            else
                return (tail - head);
        }

    private:
    
        T _data[NodeCapacity + 1];
        std::atomic<std::size_t> _head;
        std::atomic<std::size_t> _tail;
    };
}