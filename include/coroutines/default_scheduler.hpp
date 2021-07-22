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

        static default_scheduler &instance()
        {
            static default_scheduler *scheduler;
            if (scheduler == nullptr)
            {
                scheduler = new default_scheduler(std::thread::hardware_concurrency());
            }
            return *scheduler;
        }

        ~default_scheduler()
        {
            this->finished = true;
            for (auto &t : this->workers)
                t.join();
        }

    private:
        std::atomic<bool> finished = false;
        std::vector<std::thread> workers;
        std::unique_ptr<channel<std::coroutine_handle<>>> queued_work;
    };
}