#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    // 构造函数：最大事件为1024
    explicit Epoller(int maxEvent = 1024);
    // 析构函数
    ~Epoller();
    // 添加事件
    bool AddFd(int fd, uint32_t events);
    // 修改事件
    bool ModFd(int fd, uint32_t events);
    // 删除事件
    bool DelFd(int fd);
    // epoll_wait
    int Wait(int timeoutMs = -1);
    // 获取发生事件的文件描述符
    int GetEventFd(size_t i) const;
    // 获取事件表中的就绪事件
    uint32_t GetEvents(size_t i) const;
        
private:
    int epollFd_;  // epoll_create()创建一个epoll对象 返回值就是epollFd，表示唯一的内核事件表
    std::vector<struct epoll_event> events_;  // epoll事件表
};

#endif //EPOLLER_H