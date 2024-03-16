#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <utility>

template <typename T>
class ThreadPool
{
public:
    using Work = std::function<void(T)>;

private:
    using Task = std::pair<Work, T>;

    std::vector<std::thread> threads;
    std::queue<Task> taskQueue;
    std::mutex mutex;
    bool stop;

    void run()
    {
        while (!stop)
        {
            std::unique_lock lock(mutex);
            if (!taskQueue.empty())
            {
                Task t = taskQueue.front();
                taskQueue.pop();
                lock.unlock();
                t.first(t.second);
            }
        }
    }

public:
    explicit ThreadPool(int threadCount)
    {
        for (int i = 0; i < threadCount; i++)
            threads.push_back(std::thread(&ThreadPool::run, this));
    }

    ~ThreadPool()
    {
        stop = true;
        for (int i = 0; i < threads.size(); i++)
            threads[i].join();
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void addTask(const Work &task, const T param)
    {
        std::unique_lock lock(mutex);
        taskQueue.push(std::make_pair(task, param));
    }
};
