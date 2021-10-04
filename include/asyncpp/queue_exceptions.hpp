#pragma once
#include <exception>

namespace async
{
    class queue_empty_exception : public std::exception
    {
    public:
        queue_empty_exception() noexcept : std::exception("queue empty") {}
    };

    class queue_full_exception : public std::exception
    {
    public:
        queue_full_exception() noexcept : std::exception("queue full") {}
    };
}