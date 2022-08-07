#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>  
#include <condition_variable>  
#include <queue>
#include <thread> 
#include <functional>

using namespace std;

// 线程池类
class ThreadPool {
public:
    // 构造函数
    explicit ThreadPool(size_t threadCount = 8) : pool_(make_shared<Pool>()) {
            // 创建子线程，默认为8个，创建完成后处于休眠
            for(size_t i = 0; i < threadCount; i++) {
                thread([pool = pool_] {
                    unique_lock<mutex> locker(pool->mtx);  // 加锁
                    // 判断任务是否为空
                    while(true) {
                        if(!pool->tasks.empty()) {
                            // 不为空
                            auto task = move(pool->tasks.front());  // 取任务
                            pool->tasks.pop();  // 移除任务
                            locker.unlock();  // 解锁
                            task();  // 执行任务
                            locker.lock();  // 再次上锁
                        } 
                            // 判断线程池是否要关闭
                        else if(pool->isClosed) break;  // 关闭直接退出循环
                            // 为空
                        else pool->cond.wait(locker);  // 处于等待状态
                    }
                }).detach();  // 线程分离
            }
    }

    // 合成的构造函数
    ThreadPool() = default;  

    // 合成的移动构造函数
    ThreadPool(ThreadPool&&) = default;  
    
    // 析构函数
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                lock_guard<mutex> locker(pool_->mtx);  // 释放锁
                pool_->isClosed = true;  // 关闭池子设置为true
            }
            pool_->cond.notify_all();
        }
    }

    // 添加任务
    template<class F>
    void AddTask(F&& task) {
        // 添加任务到任务队列
        {
            lock_guard<mutex> locker(pool_->mtx);
            // std::forward是一个完美转发点，可以将类型原封不动的转发走
            pool_->tasks.emplace(forward<F>(task));  
        }
        // 唤醒一个线程
        pool_->cond.notify_one();  
    }

private:
    // 线程池结构体
    struct Pool {
        mutex mtx;  // 互斥锁
        condition_variable cond;  // 条件变量
        bool isClosed;  // 是否关闭线程池
        queue<function<void()>> tasks;  // 任务队列
    };
    // 线程池对象
    shared_ptr<Pool> pool_;  
};

#endif //THREADPOOL_H