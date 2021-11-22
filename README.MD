# async++
Some useful generic classes taking advantage of C++20 and coroutines.

Currently includes:
* [`task<T>`](#taskt)
* [`generator<T>`](#generatort)
* [`queue<T>`](#queuet)
* [`bounded_queue<T>`](#bounded_queuet)

## `task<T>`
```c++
    template <typename T>
    class task
    {
    public:
        class promise_type
        {
        public:
            class awaiter
            {
            public:
                awaiter() = default;

                constexpr bool await_ready() const noexcept;

                constexpr void await_suspend(std::coroutine_handle<> h) noexcept;

                constexpr void await_resume() noexcept;
            };

            promise_type();

            awaiter initial_suspend() noexcept;

            std::suspend_always final_suspend() noexcept;

            void return_value(const T &value) noexcept;

            void return_value(T &&value) noexcept;

            task<T> get_return_object() noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;

            T &&result();
        };

        task() = default;

        task(std::coroutine_handle<promise_type> h) noexcept;

        T &&result();

        T &&await_resume();

        constexpr bool await_suspend(std::coroutine_handle<> h) noexcept;

        bool await_ready() const noexcept;

        bool done() const noexcept;

        template <typename Func, typename... Args>
        static task<T> run(const Func &func, const Args&... args);

        ~task() noexcept;
    };
```

## `generator<T>`
```c++
    template <typename T>
    class generator
    {
    public:
        class promise_type
        {
        public:
            promise_type() = default;

            generator<T> get_return_object() noexcept;

            std::suspend_always initial_suspend() const noexcept;

            std::suspend_always final_suspend() const noexcept;

            void unhandled_exception() noexcept;

            void rethrow_if_unhandled_exception() const;

            void return_void();

            std::suspend_always yield_value(const T &value) noexcept;

            std::suspend_always yield_value(T &&value) noexcept;

            T &get_value() noexcept;
        };

        class iterator
        {
        public:
            iterator() = default;

            iterator(std::coroutine_handle<promise_type> handle) noexcept;

            iterator(const iterator &) = default;

            iterator(iterator &&) = default;

            iterator &operator=(const iterator &) = default;

            iterator &operator=(iterator &&) = default;

            bool operator==(const std::default_sentinel_t &) const noexcept;

            bool operator!=(const std::default_sentinel_t &) const noexcept;

            iterator &operator++() noexcept;

            T &operator*() const noexcept;

            T *operator->() const noexcept;
        };

        generator() = default;

        generator(std::coroutine_handle<promise_type> h) noexcept;

        template <std::ranges::range Range>
        generator(const Range &range) noexcept;

        template <std::ranges::range Range>
        generator(Range &&range) noexcept;

        generator(generator &&other) noexcept;

        iterator begin() const noexcept;

        std::default_sentinel_t end() const noexcept;

        template <class Predicate>
        bool all(const Predicate &pred) const;

        template <class Predicate>
        bool any(const Predicate &pred) const;

        generator<T> append(const T &value);

        generator<T> append(T &&value);

        generator<std::vector<T>> chunk(std::size_t size);

        bool contains(const T &value) const;

        template <std::integral Integral = std::size_t>
        Integral count() const;

        generator<T> distinct();

        T element_at(std::size_t index) const;

        T first() const;

        T last() const;

        generator<T> prepend(const T &value);

        generator<T> prepend(T &&value);

        generator<T> prepend(generator<T> &&other);

        static generator<T> range(T from, T to);

        static generator<T> repeat(const T &value, std::size_t count);

        generator<T> reverse();

        template <class Selector>
        generator<std::invoke_result_t<Selector, const T &>> select(const Selector &selector);

        template <class Predicate>
        generator<T> where(const Predicate &pred);

        ~generator() noexcept;
    };
```

## `queue<T>`
```c++
    template <typename T, std::size_t NodeCapacity = 1024>
    class queue
    {
    public:
        queue();
        
        void push(const T &item);
        
        void push(T &&item);

        T pop();

        std::size_t size() const;

        bool empty() const;
    };

```

## `bounded_queue<T>`
```c++
    template <typename T, std::size_t NodeCapacity>
    class bounded_queue
    {
    public:
        bounded_queue(std::size_t capacity);

        void push(const T &item);

        void push(T &&item);

        T pop();

        std::size_t size() const;
    };
```
