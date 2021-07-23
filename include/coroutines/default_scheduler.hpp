#pragma once
#include <coroutine>
#include <vector>
#include <iostream>

namespace coroutines
{
    template <class T>
    class channel;

    class default_scheduler
    {
    public:
        default_scheduler() = default;
        default_scheduler(std::size_t thread_count);

        void schedule(std::coroutine_handle<> coroutine);

        static default_scheduler &instance();

        ~default_scheduler();

    private:
        std::atomic<bool> _finished = false;
        std::vector<std::thread> _workers;
        std::unique_ptr<channel<std::coroutine_handle<>>> _queued_work;
    };
}