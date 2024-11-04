# Scheduler Library

The Scheduler Library provides a simple and efficient way to schedule and execute tasks at specified timestamps using C++.
The core component of this library is the `Scheduler` class, which manages task scheduling and execution with the help of a thread pool for concurrency.

## Usage

```cpp
#include "scheduler.h"

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
