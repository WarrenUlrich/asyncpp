cmake_minimum_required (VERSION 3.8)
project (asyncpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++latest")

# Needed for release build to work, optimizations currently break coroutines.
add_compile_options("/d2CoroOptsWorkaround")

add_library("asyncpp" INTERFACE)

target_include_directories("asyncpp" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")