#include "coroutines/channel.hpp"
#include "coroutines/default_scheduler.hpp"

namespace coroutines
{
    default_scheduler::default_scheduler(std::size_t thread_count)
    {
        this->queued_work = channel<std::coroutine_handle<>>::make_unbounded();
        for (std::size_t i = 0; i < thread_count; ++i)
        {
            this->workers.emplace_back(std::thread([&]
                                                   {
                                                       while (true)
                                                       {
                                                           if (this->finished)
                                                               break;

                                                           auto handle = this->queued_work->wait();
                                                           handle.resume();
                                                           if (handle.done())
                                                               handle.destroy();
                                                       }
                                                   }));
        }
    }

    void default_scheduler::schedule(std::coroutine_handle<> coroutine)
    {
        queued_work->try_write(coroutine);
    }
}