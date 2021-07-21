#pragma once
#include <coroutine>
#include "channel.hpp"
#include <vector>
#include <iostream>

namespace coroutines
{
    class default_scheduler
    {
    public:
        default_scheduler() = default;
        default_scheduler(std::size_t thread_count)
        {
            for (std::size_t i = 0; i < thread_count; ++i)
            {
                this->workers.emplace_back(std::thread([this]()
                                                       {
                                                           while (true)
                                                           {
                                                               if (this->finished)
                                                                   break;

                                                               auto handle = this->queued_work.wait();
                                                               handle.resume();
                                                               if (handle.done())
                                                                   handle.destroy();
                                                           }
                                                       }));
            }
        }

        void schedule(std::coroutine_handle<> coroutine)
        {
            queued_work.try_write(coroutine);
        }

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
        unbounded_channel<std::coroutine_handle<>> queued_work;
    };
}