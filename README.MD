# async++
Some useful generic classes taking advantage of C++20 and coroutines.

Currently includes:
* [`task<T>`](#taskt)
* [`enumerable<T>`](#enumerablet)
* Channel types
  * [`channel<T>`](#channelt)
  * [`unbounded_channel<T>`](#unbounded_channelt)
  * [`bounded_channel<T>`](#bounded_channelt)

## `task<T>`
```c++
#include <string>
#include <coroutines/task.hpp>

coroutines::task<std::string> get_string_async()
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

coroutines::task<int> async_main()
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
#include <coroutines/enumerable.hpp>

coroutines::enumerable<int> alot_of_numbers()
{
    for (int i = 0; i < 10000; ++i)
    {
        co_yield i;
    }
}

coroutines::enumerable<int> alot_of_even_numbers()
{
    return alot_of_numbers().where([] (int i) { return i % 2 == 0; }); 
}

coroutines::enumerable<std::string> parity_strings()
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

## `channel<T>`
```c++
#include <iostream>
#include <coroutines/channel.hpp>
#include <coroutines/task.hpp>

std::unique_ptr<coroutines::channel<int>> numbers_channel = coroutines::channel<int>::create_unbounded();

coroutines::task<> write_to_channel()
{
    int i = 0;
    while (numbers_channel->try_write(++i)) // Writes until the channel is full (if bounded), or closed.
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate some work
    }

    std::cout << "stopped writing" << std::endl;
    co_return; // must co_return.
}

int main()
{
    write_to_channel(); // Writes to the channel asynchronously.

    for (auto it = numbers_channel->begin(); it != numbers_channel->end(); ++it) // Channels are iterable until they are closed.
    {
        std::cout << "Received: " << *it << std::endl;
        if (*it == 5)
            break;
    }

    while (true)
    {
        std::optional<int> number = numbers_channel->wait(); // Returns an optional value that is empty if the channel is closed.
        if (number)
        {
            std::cout << "Received: " << *number << std::endl;
            if (*number == 10)
                break;
        }
    }

    numbers_channel->close(); // Closes the channel.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // So we can observe the effect of closing the channel.
    return -1;
}
```

## `unbounded_channel<T>`
```c++
#include <iostream>
#include <coroutines/channel.hpp>
#include <coroutines/task.hpp>

coroutines::unbounded_channel<int> numbers_channel = coroutines::unbounded_channel<int>();

coroutines::task<> write_to_channel()
{
    int i = 0;
    while (numbers_channel.try_write(++i)) // Writes until the channel is full (if bounded), or closed.
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate some work
    }

    std::cout << "stopped writing" << std::endl;
    co_return; // must co_return.
}

int main()
{
    write_to_channel(); // Writes to the channel asynchronously.
    for(const auto& i : numbers_channel) // Reads from the channel asynchronously.
    {
        std::cout << i << std::endl;
    }
    return -1;
}
```

## `bounded_channel<T>`
```c++
#include <iostream>
#include <coroutines/channel.hpp>
#include <coroutines/task.hpp>

coroutines::bounded_channel<int> numbers_channel = coroutines::bounded_channel<int>(50);

coroutines::task<> write_to_channel()
{
    int i = 0;
    while (numbers_channel.try_write(++i)) // Writes until the channel is full (if bounded), or closed.
    {
        // Let the bounded channel fill up.
    }

    std::cout << "stopped writing" << std::endl;
    co_return; // must co_return.
}

int main()
{
    write_to_channel(); // Writes to the channel asynchronously.
    
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Let the channel fill up before we start consuming it.
    for(const auto& i : numbers_channel) // Reads from the channel asynchronously.
    {
        std::cout << i << std::endl;
    } // Never stops because the channel is never closed.
    
    return -1;
}
```
