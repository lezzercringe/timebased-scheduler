/**
 * @file threadpool.h
 * @brief Header file for the ThreadPool class.
 */

#pragma once

#include <atomic>
#include <functional>
#include <utility>
#include <vector>
#include <thread>

#include "circular_buffer.h"

namespace scheduler {
namespace internal {


/**
 * @brief A simple thread pool implementation for managing and executing tasks concurrently.
 *
 * The ThreadPool class allows you to add tasks to a queue and have them executed by a pool of threads.
 * It uses a circular buffer to store tasks and provides methods to start and stop the execution of tasks.
 *
 * @note This class is designed to be non-copyable and non-movable to ensure unique ownership of its resources.
 */

class ThreadPool {
public:
    /**
     * @typedef Fn
     * @brief A type alias for a callable task in the thread pool.
     */
    using Fn = std::function<void()>;

    /**
     * @brief Constructs a ThreadPool with a specified number of threads and buffer size.
     *
     * @param threads_amount The number of threads to be created in the pool.
     * @param buffer_size The size of the circular buffer used to store tasks.
     */
    ThreadPool(size_t threads_amount, size_t buffer_size)
	: threads_amount_{threads_amount},
	  tasks_buffer_{buffer_size}
    {}

    /**
     * @brief Destructor for the ThreadPool class.
     * 
     * This destructor ensures that the thread pool is properly shut down when the ThreadPool object is destroyed.
     * It calls the `Shutdown` method to signal all worker threads to stop processing tasks and waits for them to finish execution.
     * This guarantees that all resources are released and no threads are left running in the background.
     */
    ~ThreadPool() {
	Shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&)= delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Adds a new task to the thread pool's task queue.
     *
     * This method allows you to enqueue a task, represented as a callable object, to be executed by the thread pool.
     * @param task A callable object (e.g., a lambda, function pointer, or std::function) representing the task to be executed.
     */
    void AddTask(Fn task) {
	tasks_buffer_.EmplacePush(std::move(task));
    } 

    /**
     * @brief Starts the execution of tasks by launching the worker threads.
     * 
     * This method initializes the thread pool by creating and starting the specified number of worker threads.
     * Each thread will continuously fetch and execute tasks from the task queue until the pool is shut down.
     */
    void Run() {
	break_ = false;

	for (size_t i = 0; i < threads_amount_; ++i) {
	    threads_.emplace_back(std::bind(&ThreadPool::Worker, this));
	}
    }

    /**
     * @brief Shuts down the thread pool and joins all worker threads.
     * 
     * This method signals all worker threads to stop processing tasks and waits for them to finish execution.
     * It ensures that all threads are properly joined and resources are cleaned up.
     */
    void Shutdown() {
	break_ = true;

	for (auto& thread: threads_) {
	    thread.join();
	}

	threads_.clear();
    }

private:
    /**
     * @brief The worker function executed by each thread in the pool.
     * 
     * This function runs in a loop, continuously attempting to fetch tasks from the task queue.
     * If a task is available, it is executed. The loop continues until the pool is signaled to shut down
     * and the task queue is empty.
     */
    void Worker() {
	while (!break_ || !tasks_buffer_.Empty()) {
	    using namespace std::chrono_literals;
	    auto task = tasks_buffer_.TryPopFor(500ms);

	    if (task) {
		std::invoke(task.value());
	    }
	}
    }

    size_t threads_amount_;
    std::vector<std::thread> threads_;
    SPMCCircularBuffer<Fn> tasks_buffer_;
    std::atomic<bool> break_ = false;
};


} // namespace internal
} // namespace scheduler
