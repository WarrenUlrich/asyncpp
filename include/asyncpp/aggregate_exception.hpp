#pragma once
#include <stdexcept>

namespace async
{
    class aggregate_exception : public std::exception
    {
    public:
        std::vector<std::exception_ptr> exceptions;

        aggregate_exception(const std::vector<std::exception_ptr> &exceptions) noexcept;
        aggregate_exception(std::vector<std::exception_ptr> &&exceptions) noexcept;

        const char *what() const noexcept override;
    };

    aggregate_exception::aggregate_exception(const std::vector<std::exception_ptr> &exceptions) noexcept
        : exceptions(exceptions)
    {
    }

    aggregate_exception::aggregate_exception(std::vector<std::exception_ptr> &&exceptions) noexcept
        : exceptions(std::move(exceptions))
    {
    }

    const char *aggregate_exception::what() const noexcept
    {
        return "aggregate exception";
    }
}