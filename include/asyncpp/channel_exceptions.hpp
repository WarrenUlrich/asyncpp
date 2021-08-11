#pragma once
#include <exception>

namespace async
{
    class channel_closed_exception : public std::exception
    {
    public:
        channel_closed_exception() : std::exception("Channel closed") {}
    };

    class channel_empty_exception : public std::exception
    {
    public:
        channel_empty_exception() : std::exception("Channel empty") {}
    };
}