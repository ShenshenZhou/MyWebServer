#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
public:
    // 构造函数
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, 
        int sqlPort, const char* sqlUser, const  char* sqlPwd, 
        const char* dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);
    // 析构函数
    ~WebServer();
    // 服务器启动入口
    void Start();

private:
    bool InitSocket_();  // 初始化Socket
    void InitEventMode_(int trigMode);  // 设置事件模式
    void AddClient_(int fd, sockaddr_in addr);  // 添加客户端连接
  
    void DealListen_();  // 处理监听
    void DealWrite_(HttpConn* client);  // 处理写
    void DealRead_(HttpConn* client);  // 处理读

    void SendError_(int fd, const char*info);  // 报错
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);  // 关闭连接

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;  // 最大的文件描述符的个数

    static int SetFdNonblock(int fd);  // 设置文件描述符非阻塞

    int port_;  // 端口
    bool openLinger_;  // 是否打开优雅关闭
    int timeoutMS_;  // 超时时间：毫秒MS
    bool isClose_;  // 是否关闭
    int listenFd_;  // 监听的文件描述符
    char* srcDir_;  // 资源目录
    
    uint32_t listenEvent_;  // 监听的文件描述符事件
    uint32_t connEvent_;  // 链接的文件描述符事件
   
    std::unique_ptr<HeapTimer> timer_;   // 定时器
    std::unique_ptr<ThreadPool> threadpool_;  // 线程池
    std::unique_ptr<Epoller> epoller_;  // epoll对象
    std::unordered_map<int, HttpConn> users_;  // 保存的是客户端连接的信息：文件描述符，HTTP连接
};


#endif //WEBSERVER_H