# async++
Some useful generic classes taking advantage of C++20 and coroutines.

Currently includes:
* [`task<T>`](#taskt)
* [`enumerable<T>`](#enumerablet)

## `task<T>`
```c++
#include <string>
#include <asyncpp/task.hpp>

async::task<std::string> get_string_async()
{
    std::this_thread::sleep_for(std::chrono::seconds(1)); // simulate some work
    co_return "done!";
}

int main()
{
    auto task = get_string_async(); // get_string_async is scheduled on a thread pool.

    auto result = task.result(); // waits for the result.
    std::cout << "result was: " << result << std::endl;
    return -1;
}

async::task<int> async_main()
{
    auto t = get_string_async();

    auto result = co_await t; // can use co_await in a coroutine.
    std::cout << "result was: " << result << std::endl;
    co_return -1;
}
```
## `enumerable<T>`
```c++
#include <iostream>
#include <asyncpp/enumerable.hpp>

async::enumerable<int> alot_of_numbers()
{
    for (int i = 0; i < 10000; ++i)
    {
        co_yield i;
    }
}

async::enumerable<int> alot_of_even_numbers()
{
    return alot_of_numbers().where([] (int i) { return i % 2 == 0; }); 
}

async::enumerable<std::string> parity_strings()
{
    return alot_of_numbers().map([] (int i) 
    {
        return i % 2 == 0 ? std::string("even") : std::string("odd");
    });
}

int main()
{
    for(const auto& i : alot_of_even_numbers()) // enumerables are iterable
    {
        std::cout << i << std::endl;
    }

    for(const auto& s : parity_strings())
    {
        std::cout << s << std::endl;
    }
    return -1;
}
```
