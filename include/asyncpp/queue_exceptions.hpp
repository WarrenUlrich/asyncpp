#pragma once
#include <exception>

namespace async
{
    class queue_empty_exception : public std::exception
    {
    public:
        virtual const char* what() const throw()
        {
            return "Queue is empty";
        }
    };

    class queue_full_exception : public std::exception
    {
    public:
        virtual const char* what() const throw()
        {
            return "Queue is full";
        }
    };
}