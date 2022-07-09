#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);
            // 创建threadCount个子线程
            for(size_t i = 0; i < threadCount; i++) {
                std::thread([pool = pool_] {
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while(true) {
                        // 判断任务是否为空
                        if(!pool->tasks.empty()) {
                            // 从任务队列中去一个任务
                            auto task = std::move(pool->tasks.front());
                            // 取出一个 移除一个
                            pool->tasks.pop();
                            locker.unlock();
                            task();
                            locker.lock();
                        } 
                        else if(pool->isClosed) break;
                        else pool->cond.wait(locker);
                    }
                }).detach();  // 线程分离
            }
    }

    ThreadPool() = default;  // 合成默认构造函数

    ThreadPool(ThreadPool&&) = default;  // 合成移动构造函数
    
    // 析构函数
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;  // 关闭池子设置为true
            }
            pool_->cond.notify_all();
        }
    }

    // 添加任务
    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));  // 添加一个线程池
        }
        pool_->cond.notify_one();  // 唤醒一个线程
    }

private:
    // 结构体 线程池
    struct Pool {
        std::mutex mtx;  // 互斥锁
        std::condition_variable cond;  // 条件变量
        bool isClosed;  // 是否关闭线程池
        std::queue<std::function<void()>> tasks;  // 队列（保存任务）
    };
    std::shared_ptr<Pool> pool_;
};


#endif //THREADPOOL_H