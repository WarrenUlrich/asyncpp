#pragma once
#include <coroutine>
#include <thread>
#include <future>

namespace async::detail
{
    void schedule_coroutine(std::coroutine_handle<> coro)
    {
        //TODO: use a thread pool.
        std::async(std::launch::async, coro);
    }
}

#ifndef CUSTOM_SCHEDULER
    #define CUSTOM_SCHEDULER(h) async::detail::schedule_coroutine(h)
#endif
