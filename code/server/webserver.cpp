#include "webserver.h"

using namespace std;

// 构造函数：初始化服务器相关参数
WebServer::WebServer(
            int port, int trigMode, int timeoutMS, bool OptLinger,
            int sqlPort, const char* sqlUser, const  char* sqlPwd,
            const char* dbName, int connPoolNum, int threadNum,
            bool openLog, int logLevel, int logQueSize):
            port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
    {
    // 获取资源路径
    srcDir_ = getcwd(nullptr, 256);  // 获取当前文件路径
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);  // 拼接目录：资源路径
    HttpConn::userCount = 0; 
    HttpConn::srcDir = srcDir_;  
    // 数据库连接池初始化
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    // 设置事件模式
    InitEventMode_(trigMode);
    // 初始化套接字
    if(!InitSocket_()) { isClose_ = true;}  
    // 日志开始记录
    if(openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

// 析构函数：服务器关闭操作
WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

// 设置事件模式（监听的Socket和通信的Socet）
void WebServer::InitEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;  // TCP连接断开
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;  // 连接事件每次都重新设置epolloneshot事件
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;  // 监听Socket为ET模式
        connEvent_ |= EPOLLET;  // 连接Socket为ET模式
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

// 服务器启动
void WebServer::Start() {
    int timeMS = -1;  // epoll wait timeout == -1 无事件将阻塞
    if(!isClose_) { LOG_INFO("========== Server start =========="); }
    // 服务器不关闭就一直循环运行
    while(!isClose_) {
        // 就指定epoll的超时时间，这个时间是为了减少epoll_wait的调用次数
        // 因为没必要一直调用，到达下一个非活动连接的时间再调用即可
        if(timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();  // epoll的超时值，-1则一直阻塞，0立即退出
        }
        // 返回事件数量
        int eventCnt = epoller_->Wait(timeMS);
        // 处理事件
        for(int i = 0; i < eventCnt; i++) {
            int fd = epoller_->GetEventFd(i);  // 文件描述符
            uint32_t events = epoller_->GetEvents(i);  // 事件类型
            if(fd == listenFd_) {
                DealListen_();  // 处理监听操作：添加客户端连接
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);  // 出现错误，关闭对应文件描述符的HTTP连接
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);  // 处理读操作
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);  // 处理写操作
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

// 报错
void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

// 关闭连接
void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());  // 
    client->Close();
}

// 添加客户端连接：
void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    // 客户端连接初始化，添加到客户端连接表中
    users_[fd].init(fd, addr);
    // 添加到计时器中
    if(timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    // 添加到epoll事件表中
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    // epoll必须设置文件描述符为非阻塞
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

// 处理监听Socket：添加客户端连接
void WebServer::DealListen_() {
    struct sockaddr_in addr;  // 保存链接的客户端信息  
    socklen_t len = sizeof(addr);
    do {
        // 接受连接
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        // 添加连接
        AddClient_(fd, addr);  
    } while(listenEvent_ & EPOLLET);
}

// 处理读操作：交给工作线程
void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);  // 更新超时时间
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

// 处理写操作：交给工作线程
void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);  // 更新超时时间
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

// 当有新的读写事件发生之后：调整定时器，更新该连接的超时时间
void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
}

// 工作线程读操作
void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);  // 读取客户端的数据
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    // 处理，业务逻辑的处理
    OnProcess(client);
}

// 工作线程处理操作：根据HTTP处理请求和响应来修改epoll中的对应事件状态
void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

// 工作线程写操作
void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);  // 写客户端的数据
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

// 初始化Socket
bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET;  // 地址族：与协议类型对应，TCP/IPv4协议族
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 端口号：需要主机字节序向网络字节序转换
    addr.sin_port = htons(port_);  // 端口号：需要主机字节序向网络字节序转换
    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    // 创建socket
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }
    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }
    // 绑定地址
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    // 监听
    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    // 将待处理的客户连接添加到epoll事件表中
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

// 设置文件描述符非阻塞
int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


