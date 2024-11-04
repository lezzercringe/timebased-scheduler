#include <iostream>

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
