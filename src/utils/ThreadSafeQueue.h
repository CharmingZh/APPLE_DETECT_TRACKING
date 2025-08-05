//
// Created by JmZha on 25-8-3.
//

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(value));
        m_cond.notify_one();
    }

    // 非阻塞pop, 如果队列为空则立即返回false
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // 阻塞pop, 如果队列为空则等待
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this]{ return !m_queue.empty(); });
        value = std::move(m_queue.front());
        m_queue.pop();
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

#endif // THREADSAFEQUEUE_H