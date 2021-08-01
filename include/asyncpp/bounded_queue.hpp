#pragma once
#include <optional>
#include <atomic>
#include <iostream>

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
         * @brief Pushes a value to the queue.
         * @param[in] item The value to push.
         * @return A boolean value indicating whether the operation was successful, the operation is only unsuccessful if the queue is full.
         */
        bool try_push(T &&item)
        {
            auto tail = _tail.load();

            // full
            if ((tail + 1) % NodeCapacity == _head.load())
                return false;

            // retry if we couldn't update the tail in time.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % NodeCapacity))
                return try_push(std::forward<T>(item));

            _data[tail] = std::move(item);
            return true;
        }

        /**
         * @brief Pushes a value to the queue.
         * @param[in] item The value to push.
         * @return A boolean value indicating whether the operation was successful, the operation is only unsuccessful if the queue is full.
         */
        bool try_push(T const &item)
        {
            auto tail = _tail.load();

            // full
            if ((tail + 1) % NodeCapacity == _head.load())
                return false;

            // retry if we couldn't update the tail in time.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % NodeCapacity))
                return try_push(item);

            _data[tail] = item;
            return true;
        }

        /**
         * @brief Pops a value from the queue.
         * @return A optional<T> value that contains the value that was pooped, if the queue is empty, it's a nullopt.
         */
        std::optional<T> try_pop()
        {
            auto head = _head.load();

            // empty
            if (head == _tail.load())
                return std::nullopt;

            // retry if we couldn't update the head in time.
            if (!_head.compare_exchange_strong(head, (head + 1) % NodeCapacity))
            {
                return try_pop();
            }

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
        T _data[NodeCapacity];
        std::atomic<std::size_t> _head;
        std::atomic<std::size_t> _tail;
    };
}