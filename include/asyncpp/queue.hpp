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

    /**
     * @brief A lock-free queue implementation.
     */
    template <typename T, std::size_t NodeCapacity = 1024>
    class queue
    {
    public:
        queue() : _head(std::make_shared<ring_node>()), _tail(_head.load(std::memory_order_relaxed)) {}

        /**
         * @brief Pushes an element to the queue
         * @param item The item to push
         * @throws queue_full_exception if the queue is full.
         */
        void push(T &&item);

        /**
         * @brief Pushes an element to the queue
         * @param item The item to push
         * @throws queue_full_exception if the queue is full.
         */
        void push(const T &item);

        /**
         * @brief Pops an element from the queue
         * @return The next item in the queue.
         * @throws queue_empty_exception if the queue is empty.
         */
        T pop();

        /**
         * @return The current size of the queue.
         */
        std::size_t size() const;

        /**
         * @return True if the queue is empty, false otherwise.
         */
        bool empty() const;

    private:
        class ring_node
        {
        public:
            std::atomic<std::shared_ptr<ring_node>> _next;

            ring_node() : _head(0), _tail(0) {}

            bool try_push(const T &item);
            bool try_push(T &&item);

            std::optional<T> try_pop();

            std::size_t size() const;

        private:
            T _data[NodeCapacity];
            std::atomic<std::size_t> _head;
            std::atomic<std::size_t> _tail;
        };

        std::atomic<std::shared_ptr<ring_node>> _head;
        std::atomic<std::shared_ptr<ring_node>> _tail;
    };

    template <typename T, std::size_t NodeCapacity>
    void queue<T, NodeCapacity>::push(const T &item)
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

    template <typename T, std::size_t NodeCapacity>
    void queue<T, NodeCapacity>::push(T &&item)
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

    template <typename T, std::size_t NodeCapacity>
    T queue<T, NodeCapacity>::pop()
    {
        try
        {
            auto head = _head.load();
            auto item = head->pop();
            return item;
        }
        catch (queue_empty_exception &)
        {
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
    }

    template <typename T, std::size_t NodeCapacity>
    std::size_t queue<T, NodeCapacity>::size() const
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

    template <typename T, std::size_t NodeCapacity>
    bool queue<T, NodeCapacity>::empty() const
    {
        return this->size() == 0;
    }

    template <typename T, std::size_t NodeCapacity>
    bool queue<T, NodeCapacity>::ring_node::try_push(const T &item)
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

    template <typename T, std::size_t NodeCapacity>
    bool queue<T, NodeCapacity>::ring_node::try_push(T &&item)
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

    template <typename T, std::size_t NodeCapacity>
    std::optional<T> queue<T, NodeCapacity>::ring_node::try_pop()
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

    template <typename T, std::size_t NodeCapacity>
    std::size_t queue<T, NodeCapacity>::ring_node::size() const
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
}
