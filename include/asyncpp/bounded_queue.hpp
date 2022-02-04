#pragma once
#include <optional>
#include <atomic>
#include "queue_exceptions.hpp"

namespace async
{
    /**
     * @brief A fixed size lock-free queue implementation.
     */
    template <typename T, std::size_t NodeCapacity>
    class bounded_queue
    {
    public:
        bounded_queue(std::size_t capacity) : _data(new T[capacity]), _node_capacity(capacity), _head(0), _tail(0) {}

        template <typename T>
        void bounded_queue<T>::push(const T &item)
        {
            auto tail = _tail.load();

            // full
            if ((tail + 1) % _node_capacity == _head.load())
                throw queue_full_exception();

            // retry if we couldn't update the tail in time.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % _node_capacity))
                return push(item);

            _data[tail] = item;
        }

        void push(T &&item)
        {
            auto tail = _tail.load();

            // full
            if ((tail + 1) % _node_capacity == _head.load())
                throw queue_full_exception();

            // retry if we couldn't update the tail in time.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % _node_capacity))
                return push(item);

            _data[tail] = std::move(item);
        }

        T pop()
        {
            auto head = _head.load();

            // empty
            if (head == _tail.load())
                throw queue_empty_exception();

            // retry if we couldn't update the head in time.
            if (!_head.compare_exchange_strong(head, (head + 1) % _node_capacity))
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
        T *_data;
        std::size_t _node_capacity;
        std::atomic<std::size_t> _head;
        std::atomic<std::size_t> _tail;
    };
}