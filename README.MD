# async++
Some useful generic classes taking advantage of C++20 and coroutines.

Currently includes:
* [`task<T>`](#taskt)
* [`generator<T>`](#generatort)
* [`queue<T>`](#queuet)

## `task<T>`
```c++
    /**
     * @brief A task is a coroutine that is executed on a seperate thread and returns a single result.
    */
    template <typename T>
    class task
    {
    public:
        task(task &&other) noexcept ...
        
        /**
         * @brief Waits for the task to complete and retrieves the result.
         * @throws any unhandled exception that may have occurred.
         */
        T get() ...
    };
```
## Examples
```c++
#include <iostream>
#include <asyncpp/task.hpp>

async::task<int> simulate_work()
{
    std::cout << "simulate_work: " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    co_return 42;
}

async::task<> other_work()
{
    auto t = simulate_work();
    auto res = co_await t; // We may use co_await inside of a coroutine.
    co_return;
}

int main()
{
    auto t = simulate_work(); // simulate_work is executed in a separate thread
    std::cout << "main: " << std::this_thread::get_id() << std::endl;
    return t.get(); // Have to use get to wait for the task to finish, rather than co_await.
}
```

## `generator<T>`
```c++
    /**
     * @brief Represents a sequence of values
     */
    template <class T>
    class generator
    {
    public:
        class promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        class iterator;

        generator() = delete;

        generator(handle_type h)

        generator(generator<T> &&other) noexcept

        generator<T> &operator=(generator<T> &&other) noexcept

        template <std::ranges::range Range>
        generator(const Range &r) ...

        template <std::ranges::range Range>
        generator(Range &&r) ...

        iterator begin() ...
        
        std::default_sentinel_t end() const noexcept ...
        
        /**
         * @brief Filters a sequence of values based on a predicate.
         * @param pred The predicate to use to filter the sequence.
         * @return A new enumerable sequence containing the filtered values.
         */
        template <class Predicate>
        generator<T> where(Predicate &&pred) ...

        /**
         * @brief Projects a sequence of values into a new sequence of a different type.
         * @param mapper The function to use to map the sequence.
         * @return A new enumerable sequence containing the projected values.
         */
        template <class Mapper>
        generator<std::invoke_result_t<Mapper, T &>> map(Mapper &&mapper) ...

        /**
         * @brief Returns the first value in a sequence.
         * @return The first value in the sequence.
         */
        T first() ...

        /**
         * @brief Returns the first value in a sequence that satisfies a predicate.
         * @param pred The predicate to use to filter the sequence.
         * @return The first value in the sequence that satisfies the predicate.
         */
        template <class Predicate>
        T first(Predicate &&pred) ...

        /**
         * @brief Counts and returns the number of elements in a sequence.
         * @return The number of elements in the sequence.
         */
        std::size_t count() ...

        /**
         * @brief Enumerates the sequence and stores the results in a vector.
         * @return The vector containing the results.
         */
        std::vector<T> collect() ...
    };
    
    template <>
    class generator<void>
    {
    public:
        /**
         * @brief Creates a new enumerable sequence of integers from the specified range.
         * @param to The lower bound of the range.
         * @param from The upper bound of the range.
         * @return A new enumerable sequence of integers.
         */
        template <std::integral Integral>
        static generator<Integral> range(Integral to, Integral from) ...
    };
```
## Examples

```c++
    #include <iostream>
    #include <asyncpp/generator.hpp>

    async::generator<int> lots_of_numbers()
    {
        for (auto i = 0; i < 10000; ++i)
            co_yield i; // support for co_yield in C++20
    }

    async::generator<int> lots_of_odd_numbers()
    {
        return lots_of_numbers().where([](int i) { return i % 2 == 1; }); // C# LINQ like functionality.
    }

    int main()
    {
        for(auto& i : lots_of_odd_numbers())
            std::cout << i << '\n';

        return 1;
    }
```
## `queue<T>`
```c++
    /**
     * @brief A lock-free queue implementation.
     */
    template <typename T, std::size_t NodeCapacity = 1024>
    class queue
    {
    public:
        queue() ...

        /**
         * @brief Pushes an element to the queue
         * @param item The item to push
         * @throws queue_full_exception if the queue is full.
         */
        void push(T &&item) ...

        /**
         * @brief Pushes an element to the queue
         * @param item The item to push
         * @throws queue_full_exception if the queue is full.
         */
        void push(const T &item) ...

        /**
         * @brief Pops an element from the queue
         * @return The next item in the queue.
         * @throws queue_empty_exception if the queue is empty.
         */
        T pop() ...

        /**
         * @return The current size of the queue.
         */
        std::size_t size() const ...
        
        /**
         * @return True if the queue is empty, false otherwise.
         */
        bool empty() const ...        
    };
```