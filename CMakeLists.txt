cmake_minimum_required (VERSION 3.8)
project ("coroutines")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_library("coroutines" STATIC 
    "src/default_scheduler.cpp"
    )

target_include_directories("coroutines" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")