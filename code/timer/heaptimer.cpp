#include "heaptimer.h"

// 向上调整节点
void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;  // 父节点下标
    while(j >= 0) {
        if(heap_[j] < heap_[i]) { break; }
        SwapNode_(i, j);  // 小于父节点，交换节点
        i = j;  // 更新下标
        j = (i - 1) / 2;  // 下一个父节点
    }
}

// 交换节点
void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);  // 交换
    ref_[heap_[i].id] = i;  // 存储表
    ref_[heap_[j].id] = j;
} 

// 向下调整
bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if(heap_[i] < heap_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

// 将连接添加到小根堆中
void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size();  // 新插入节点的索引
        ref_[id] = i;  // 保存
        heap_.push_back({id, Clock::now() + MS(timeout), cb});  // 节点添加到小根堆
        siftup_(i);  // 向上调整
    } 
    else {
        /* 已有结点：调整堆 */
        i = ref_[id];  // 获取索引
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size())) {
            siftup_(i);
        }
    }
}

// 业务逻辑：删除超时连接
void HeapTimer::doWork(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

// 删除节点操作
void HeapTimer::del_(size_t index) {
    /* 删除指定位置的结点 */
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n) {
        SwapNode_(i, n);
        if(!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

// 更新时间之后要调整节点
void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);;
    siftdown_(ref_[id], heap_.size());
}

// 调用此函数就清除节点
void HeapTimer::tick() {
    /* 清除超时结点 */
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

// 下一次清除
int HeapTimer::GetNextTick() {
    tick();  // 清除已有的
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}