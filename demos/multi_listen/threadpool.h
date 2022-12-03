#pragma once

#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <type_traits>
#include <functional>
#include <memory>
#include <queue>

#include <iostream>

class ThreadPool {

public:

    ThreadPool(size_t size): stop_(false), size_(size) {
        for (auto i = 0; i < size; ++i) {
            threads_.emplace_back(
                [this] {
                    for(;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock lk(this->m_);
                            this->cv_.wait(lk, [this]{return this->stop_ || !this->tasks_.empty();});

                            if (this->stop_ && this->tasks_.empty()) {
                                return;
                            }

                            task = std::move(this->tasks_.front());
                            this->tasks_.pop();
                        }
                        task();
                        std::cout << "ending task." << std::endl;
                    }
                }
            );
        }
    }

    template<typename F, typename ...ARGS>
    auto enqueue(F func, ARGS... args)
        -> std::future<std::invoke_result_t<F, ARGS...>> {

        std::cout << "enqueue a job ..." << std::endl;

        using return_type = std::invoke_result_t<F, ARGS...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(func), std::forward<ARGS>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock lk(m_);
            tasks_.emplace([task]{ (*task)(); });
        }
        cv_.notify_one();

        return res;
    }

    ~ThreadPool() {
        {
            std::unique_lock lk(m_);
            stop_ = true;
        }
        cv_.notify_all();

        for (auto && t : threads_) {
            t.join();
        }
    }

private:
    size_t size_ = 0;
    bool stop_ = false;

    std::vector<std::thread> threads_;

    std::queue<std::function<void()>> tasks_;

    std::mutex m_;
    std::condition_variable cv_;
};
