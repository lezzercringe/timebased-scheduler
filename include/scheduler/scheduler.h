/**
 * @file scheduler.h
 * @brief Header file for the Scheduler class and related components.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <ctime>
#include <list>
#include <stack>
#include <thread>

#include "circular_buffer.h"
#include "threadpool.h"

namespace scheduler {
using namespace internal;

/**
 * @class Scheduler
 * @brief A task scheduler that manages and executes tasks at specified times using a thread pool.
 *
 * @note This class is designed to be non-copyable and non-movable to ensure unique ownership of its resources.
 */
class Scheduler {

public:
    /**
     * @brief Constructs a Scheduler with a specified buffer size and number of threads.
     * @param buffer_size The size of the circular buffer for storing tasks.
     * @param threads_count The number of threads in the thread pool.
     */
    Scheduler(size_t buffer_size, size_t threads_count)
	: tasks_buffer_{buffer_size}, pool_{threads_count, buffer_size}
    {}

    /**
     * @brief Shuts down the scheduler, stopping the event loop and thread pool.
     * 
     * The scheduler can be shut down and restarted multiple times. This method
     * ensures that the event loop and thread pool are properly stopped, allowing
     * for a clean restart if needed.
     */
    ~Scheduler() {
	Shutdown();
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler(const Scheduler&&) = delete;
    Scheduler& operator=(const Scheduler&)= delete;
    Scheduler& operator=(Scheduler&&) = delete;

    /**
     * @brief Adds a task to the scheduler with a specified execution time.
     * @param callable The function to be executed.
     * @param timestamp The time at which the task should be executed.
     */
    void Add(std::function<void()> callable, std::time_t timestamp) {
	tasks_buffer_.EmplacePush(Task {
	    .timestamp = std::move(timestamp),
	    .func = std::move(callable),
	});
    }

    /**
     * @brief Shuts down the scheduler, stopping the event loop and thread pool,
     * waiting for all pending tasks to be executed.
     * 
     * The scheduler can be shut down and restarted multiple times. This method
     * ensures that the event loop and thread pool are properly stopped, allowing
     * for a clean restart if needed.
     */
    void Shutdown() {
	break_ = true;
	if (event_loop_thread_.joinable()) {
	    event_loop_thread_.join();
	}
	pool_.Shutdown();
    }

    /**
     * @brief Starts the scheduler's event loop and thread pool.
     * 
     * The event loop can be started multiple times, allowing the scheduler to
     * restart after being shut down. This method initializes the
     * event loop and thread pool, preparing them to handle tasks.
     */
    void Run() {
	break_ = false;
	event_loop_thread_ = std::thread(std::bind(&Scheduler::EventLoop, this));
	pool_.Run();
    }

private:
    /**
     * @struct Task
     * @brief Represents a task with a scheduled execution time and a callable function.
     */
    struct Task {
	std::time_t timestamp;
	std::function<void()> func;
    };
    
    /**
     * @brief The event loop that continuously checks and executes tasks at their scheduled times.
     */
    void EventLoop() {
	while (!break_ || !tasks_.empty() || !tasks_buffer_.Empty()) {
	    if (!tasks_buffer_.Empty()) {
		tasks_.push_front(tasks_buffer_.PopUnsafe());
	    }

	    std::stack<std::list<Task>::iterator> to_remove;

	    for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
		using namespace std::chrono;
		auto timestamp_now = system_clock::to_time_t(system_clock::now());

		if (it->timestamp <= timestamp_now) {
		    pool_.AddTask(std::move(it->func));
		    to_remove.push(it);
		} 
	    }

	    while (!to_remove.empty()) {
		auto top = to_remove.top();
		tasks_.erase(top);
		to_remove.pop();
	    }
	}
    }

    std::thread event_loop_thread_;
    std::atomic<bool> break_;
    std::list<Task> tasks_; 
    SPMCCircularBuffer<Task> tasks_buffer_;
    ThreadPool pool_;
};

} // namespace scheduler
