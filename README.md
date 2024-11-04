# Scheduler Library

This header-only library provides a simple and efficient way to schedule and execute tasks at specified timestamps.
The core component of this library is the `Scheduler` class, which manages task scheduling and execution with the help of a thread pool for concurrency.

## Requirements
- c++20 standard

## Quickstart
The simplest way to integrate this library into your project is by using FetchContent.

```cmake
cmake_minimum_required(VERSION 3.22.1)
project(SomeProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    scheduler
    GIT_REPOSITORY https://github.com/lezzercringe/timebased-scheduler.git
    GIT_TAG main # or a specific tag/commit
)

FetchContent_MakeAvailable(scheduler)

add_executable(SomeProject main.cpp)

target_link_libraries(SomeProject PRIVATE scheduler)
```


## Usage example

```cpp
#include "scheduler/scheduler.h"

using namespace scheduler;

int main() {
    Scheduler scheduler(10, 4); // 10 tasks buffer size, 4 concurrent threads

    scheduler.Add([]() {
        std::cout << "Task executed!" << std::endl;
    }, std::time(nullptr) + 5);

    scheduler.Run();
    scheduler.Shutdown();
    return 0;
}
```
