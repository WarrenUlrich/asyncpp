#include <iostream>
#include <coroutines/task.hpp>
#include <coroutines/channel.hpp>
#include <windows.h>
#include <chrono>
#define ASYNC_MAIN

//std::unique_ptr<coroutines::channel<int>> chan = std::unique_ptr<coroutines::channel<int>>(new coroutines::unbounded_channel<int>());
std::unique_ptr<coroutines::channel<int>> chan = coroutines::channel<int>::make_bounded(50);

coroutines::task<int> read_channel()
{
    while(true)
    {
        int value;
        auto success = chan->try_read(value);
        if(success)
        {
            std::cout << "Read " << value << std::endl;
        }
    }

    co_return 5;
}

coroutines::task<int> write_channel()
{
    while(true)
    {
        int value = rand() % 10;
        std::cout << "Write " << value << std::endl;
        chan->try_write(value);
        Sleep(5000);
    }
    co_return 5;
}

coroutines::task<int> wait_for_channel()
{
    while(true)
    {
        int value;
        value = chan->wait();
        std::cout << "Waited for " << value << std::endl;
    }
    co_return 5;
}
coroutines::task<int> test()
{
    std::cout << "running on thread: " << GetCurrentThreadId() << std::endl;
    co_return 5;
}

coroutines::task<> print_hello()
{
    std::cout << "Hello" << '\n';
    co_return;
}

coroutines::task<int> async_main()
{
    print_hello();    
    co_return 5;
}

int main()
{
    return async_main().result();
}