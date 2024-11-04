/**
 * @file circular_buffer.h
 * @brief Header file for the SPMCCircularBuffer class.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace scheduler {
namespace internal {

/**
 * @brief Thread-safe, semi-lock-free Single Producer Multiple Consumer (SPMC) or Single Producer Single Consumer (SPSC) circular buffer implementation.
 * 
 * @details
 * This circular buffer implementation is designed to be efficient and thread-safe for scenarios where there is a single producer thread and either a single or multiple consumer threads.
 * 
 * - **Thread Safety**: The buffer is thread-safe for multiple consumer threads, except for the `PopUnsafe` method, which should only be used in a single consumer scenario.
 * - **Lock-Free Writes**: Writes to the buffer are lock-free under normal conditions, ensuring high performance and low latency. Locks are only employed in the rare case of buffer overflow to maintain data integrity.
 * - **Use Cases**: 
 *   - **SPMC (Single Producer Multiple Consumer)**: Multiple consumer threads can safely read from the buffer concurrently, except when using `PopUnsafe`.
 *   - **SPSC (Single Producer Single Consumer)**: In this scenario, the `PopUnsafe` method can be used for even higher performance, as it avoids the overhead of synchronization mechanisms.
 *
 * @note This class is designed to be non-copyable and non-movable to ensure unique ownership of its resources.

 * 
 * @tparam T The type of elements stored in the buffer. This allows the buffer to be used with any data type.
 * @param size The amount of preallocated memory for the buffer, determining its capacity. This should be chosen based on the expected workload to minimize overflow conditions.
 */
template<typename T>
class SPMCCircularBuffer {
public:
    /**
     * @brief Constructs a circular buffer with a specified capacity.
     * 
     * @param size The maximum number of elements the buffer can hold. This value determines the preallocated memory size.
     * 
     * @details
     * Initializes the buffer with the given size, allocating memory for storing elements of type T. The buffer is empty upon construction.
     */
    constexpr SPMCCircularBuffer(size_t size) {
	buf_ = std::make_unique<T[]>(size);
	max_size_ = size;
    }

    /**
     * @brief Destructor for the circular buffer.
     * 
     * @details
     * Cleans up any resources used by the buffer. The default destructor is sufficient as the buffer uses smart pointers for memory management.
     */
    ~SPMCCircularBuffer() = default;

    SPMCCircularBuffer(const SPMCCircularBuffer&) = delete;
    SPMCCircularBuffer(SPMCCircularBuffer&&) = delete;
    SPMCCircularBuffer& operator=(const SPMCCircularBuffer& other) = delete;
    SPMCCircularBuffer& operator=(SPMCCircularBuffer&& other) = delete;


    /**
     * @brief Inserts a new element into the buffer by constructing it in place.
     * 
     * @tparam Args The types of the arguments to forward to the constructor of T.
     * @param args The arguments to forward to the constructor of T.
     * 
     * @details
     * Constructs a new element of type T in place at the current write position. If the buffer is full, the method waits until space is available.
     * 
     * @note This method is lock-free under normal conditions but may employ locks in case of buffer overflow.
     */
    template<typename... Args>
    void EmplacePush(Args&&... args) {
	if (write_counter_ != 0 && (write_counter_ - read_counter_ == max_size_)) {
	    int old_read = read_counter_;
	    if (write_counter_ % max_size_ == old_read % max_size_)
	    read_counter_.wait(old_read);
	}

	buf_[write_counter_ % max_size_] = T(std::forward<Args&&>(args)...);
	write_counter_.fetch_add(1);
	write_counter_.notify_all();
    }

    /**
     * @brief Removes and returns an element from the buffer without synchronization between consumers.
     * 
     * @return The element removed from the buffer.
     * 
     * @details
     * This method is unsafe and should only be used in a single consumer scenario. It does not perform any synchronization checks and will 
     * abort if the buffer is empty.
     * 
     * @warning Using this method in a multi-consumer scenario can lead to undefined behavior.
     */
    T PopUnsafe() noexcept {
	if (read_counter_ >= write_counter_) {
	    std::abort();
	}

	T element = std::move_if_noexcept(buf_[read_counter_ % max_size_]);
	read_counter_.fetch_add(1);
	read_counter_.notify_all();
	return element;
    }

    /**
     * @brief Attempts to remove and return an element from the buffer within a specified time limit.
     * 
     * @param limit_ms The maximum duration to wait for an element to become available.
     * @return An optional containing the element if successful, or std::nullopt if the time limit is exceeded.
     * 
     * @details
     * This method tries to acquire a lock and remove an element from the buffer. 
     * If the buffer is empty and the lock cannot be acquired within the specified time, it returns std::nullopt.
     */
    std::optional<T> TryPopFor(std::chrono::milliseconds limit_ms) noexcept { 
	std::unique_lock lock(mutex_read_, std::defer_lock);

	if (!lock.try_lock_for(std::chrono::duration(limit_ms))) {
	    return std::nullopt;
	} 

	if (read_counter_ >= write_counter_) {
	    return std::nullopt;
	}

	T element = std::move_if_noexcept(buf_[read_counter_.fetch_add(1) % max_size_]);
	read_counter_.notify_all();
	return { element };
    }

    /**
     * @brief Removes and returns an element from the buffer.
     * 
     * @return The element removed from the buffer.
     * 
     * @details
     * This method is thread-safe and blocks until an element is available. It uses a lock to ensure safe access in a multi-consumer scenario.
     */
    T Pop() noexcept {
	std::lock_guard lock(mutex_read_);

	if (read_counter_ >= write_counter_) {
	    auto old_write = write_counter_.load();
	    if (read_counter_ >= old_write) {
		write_counter_.wait(old_write);
	    }
	}

	T element = std::move_if_noexcept(buf_[read_counter_.fetch_add(1) % max_size_]);
	read_counter_.notify_all();
	return element;
    }

    /**
     * @brief Checks if the buffer is empty.
     * 
     * @return True if the buffer is empty, false otherwise.
     * 
     * @details
     * This method provides a quick way to check if there are any elements available in the buffer.
     * It is thread-safe and can be used in both single and multiple consumer scenarios.
     * The perfect usecase is to combine it with PopUnsafe method in single consumer scenario.
     */
    bool Empty() const noexcept {
	return read_counter_ == write_counter_;
    }

private:
    std::atomic<size_t> read_counter_ = 0;
    std::atomic<size_t> write_counter_ = 0;
    std::unique_ptr<T[]> buf_;
    size_t max_size_;
    std::timed_mutex mutex_read_;
};

} // namespace internal
} // namespace scheduler
