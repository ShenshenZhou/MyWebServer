#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

// 定时器节点
struct TimerNode {
    int id;  // 文件描述符
    TimeStamp expires;  // 超时时间
    TimeoutCallBack cb;  // 回调函数：绑定关闭连接操作
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

// 小根堆定时器类
class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }
    
    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const TimeoutCallBack& cb);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int GetNextTick();

private:
    // 删除节点
    void del_(size_t i);
    // 向上调整
    void siftup_(size_t i);
    // 向下调整
    bool siftdown_(size_t index, size_t n);
    // 交换节点
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;  // vector实现堆

    std::unordered_map<int, size_t> ref_;  // 节点索引和节点值的关系
};

#endif //HEAP_TIMER_H