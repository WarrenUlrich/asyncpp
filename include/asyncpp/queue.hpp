#pragma once
#include <atomic>
#include <memory>
#include <optional>
#include "queue_exceptions.hpp"

namespace async
{
    /**
     * This is a "lock-free" implementation of a queue, it's implemented as a 
     * linked list of ring buffers, it operates on one buffer at a time until it becomes
     * full, then the tail moves on and creates a new buffer.
     */

    template <typename T, std::size_t NodeCapacity = 1024>
    class queue
    {
    public:
        queue() : _head(std::make_shared<ring_node>()), _tail(_head.load(std::memory_order_relaxed)) {}

        void push(T &&item)
        {
            auto tail = _tail.load();
            if (tail->try_push(item)) // Buffer still usable?
                return;

            // Slow route, we need to create a new buffer
            auto new_node = std::make_shared<ring_node>();
            new_node->try_push(item);
            while (true)
            {
                auto tail = _tail.load();
                auto next = tail->next.load();
                if (tail == _tail.load())
                {
                    if (next == nullptr)
                    {
                        if (tail->next.compare_exchange_strong(next, new_node))
                        {
                            _tail.compare_exchange_strong(tail, new_node);
                        }
                        break;
                    }
                    else
                    {
                        _tail.compare_exchange_strong(tail, next);
                    }
                }
            }
        }

        void push(const T &item)
        {
            auto tail = _tail.load();
            if (tail->try_push(item)) // Buffer still usable?
                return;

            // Slow route, we need to create a new buffer
            auto new_node = std::make_shared<ring_node>();
            new_node->try_push(item);
            while (true)
            {
                auto tail = _tail.load();
                auto next = tail->next.load();
                if (tail == _tail.load())
                {
                    if (next == nullptr)
                    {
                        if (tail->next.compare_exchange_strong(next, new_node))
                        {
                            _tail.compare_exchange_strong(tail, new_node);
                        }
                        break;
                    }
                    else
                    {
                        _tail.compare_exchange_strong(tail, next);
                    }
                }
            }
        }

        /**
         * @return The next item in the queue.
         * @throws async::queue_empty_exception if the queue is empty.
         */
        T pop()
        {
            auto head = _head.load();
            auto item = head->try_pop();
            if(item)
                return *item;

            // Slow route, we need to move to the next buffer
            while (true)
            {
                auto head = _head.load();
                auto tail = _tail.load();
                const auto next = head->next.load();

                if (head == _head.load())
                {
                    if (head == tail)
                    {
                        if (next == nullptr)
                            throw queue_empty_exception();

                        _tail.compare_exchange_strong(tail, next);
                    }
                    else
                    {
                        if (next == nullptr)
                            throw queue_empty_exception();

                        if (_head.compare_exchange_strong(head, next))
                        {
                            return next->pop();
                        }
                    }
                }
            }

            throw queue_empty_exception();
        }

        /**
         * @return The current size of the queue.
         */
        std::size_t size() const
        {
            auto head = _head.load();
            std::size_t count = 0;
            while (head != nullptr)
            {
                count += head->size();
                head = head->next.load();
            }
            return count;
        }

        bool empty() const
        {
            return this->size() == 0;
        }

    private:
        class ring_node;

        std::atomic<std::shared_ptr<ring_node>> _head;
        std::atomic<std::shared_ptr<ring_node>> _tail;
    };

    /**
     * A ring buffer node, it's a linked list of buffers, it's used to implement the queue,
     * it is functionally the same as bounded_queue, just with a next pointer.
     */

    template <typename T, std::size_t NodeCapacity>
    class queue<T, NodeCapacity>::ring_node
    {
    public:
        std::atomic<std::shared_ptr<ring_node>> next;

        ring_node() : _head(0), _tail(0) {}

        bool try_push(T &&item)
        {
            auto tail = _tail.load();
            auto head = _head.load();

            // Buffers full
            if ((tail + 1) % NodeCapacity == head)
                return false;

            // Check if the tail is still consistent, if it is, we can just update the tail, if not we retry the entire operation.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % NodeCapacity))
                return try_push(std::forward<T>(item));

            // Succeeded, so we're safe to update the data.
            _data[tail] = std::move(item);
            return true;
        }

        bool try_push(const T &item)
        {
            auto tail = _tail.load();
            auto head = _head.load();

            // Buffers full
            if ((tail + 1) % NodeCapacity == head)
                return false;

            // Check if the tail is still consistent, if it is, we can just update the tail, if not we retry the entire operation.
            if (!_tail.compare_exchange_strong(tail, (tail + 1) % NodeCapacity))
                return try_push(item);

            // Succeeded, so we're safe to update the data.
            _data[tail] = item;
            return true;
        }

        std::optional<T> try_pop()
        {
            auto tail = _tail.load();
            auto head = _head.load();
            if (head == tail) // No data available.
                return std::nullopt;

            // Check if the head is still consistent, if it is, we can just update the head, if not we retry the entire operation.
            if (!_head.compare_exchange_strong(head, (head + 1) % NodeCapacity))
                return pop();

            // Succeeeded, so we're safe to grab the data.
            auto item = std::move(_data[head]);
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
        std::atomic<std::size_t> _head{0};
        std::atomic<std::size_t> _tail{0};
    };
}
