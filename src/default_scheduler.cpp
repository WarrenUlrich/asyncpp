#include "coroutines/channel.hpp"
#include "coroutines/default_scheduler.hpp"

namespace coroutines
{
    default_scheduler::default_scheduler(std::size_t thread_count)
    {
        this->_queued_work = channel<std::coroutine_handle<>>::create_unbounded();
        for (std::size_t i = 0; i < thread_count; ++i)
        {
            this->_workers.emplace_back(std::thread([&]
                                                   {
                                                       while (true)
                                                       {
                                                           if (this->_finished)
                                                               break;

                                                           auto work_handle = this->_queued_work->wait();
                                                           if(!work_handle.has_value())
                                                               break;

                                                           auto handle_val = work_handle.value(); 
                                                           handle_val.resume();
                                                           if (handle_val.done())
                                                               handle_val.destroy();
                                                       }
                                                   }));
        }
    }

    default_scheduler &default_scheduler::instance()
    {
        static default_scheduler *scheduler;
        if (scheduler == nullptr)
        {
            scheduler = new default_scheduler(std::thread::hardware_concurrency());
        }
        return *scheduler;
    }

    default_scheduler::~default_scheduler()
    {
        this->_finished = true;
        for (auto &t : this->_workers)
            t.join();
    }

    void default_scheduler::schedule(std::coroutine_handle<> coroutine)
    {
        _queued_work->try_write(coroutine);
    }
}