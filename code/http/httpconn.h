#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    // 初始化HTTP连接
    void init(int sockFd, const sockaddr_in& addr);
    // 分散读，读取请求报文
    ssize_t read(int* saveErrno);
    // 集中写，传输响应报文
    ssize_t write(int* saveErrno);
    // 关闭连接
    void Close();
    // 获取文件描述符
    int GetFd() const;
    // 获取端口号
    int GetPort() const;
    // 获取IP地址
    const char* GetIP() const;
    // socket地址结构体
    sockaddr_in GetAddr() const;
    // 处理请求
    bool process();
    // 返回结构体数组内存的长度
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }
    // 是否保持连接
    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;  // 是否ET模式
    static const char* srcDir;  // 资源目录
    static std::atomic<int> userCount;  // 用户账号
    
private:
   
    int fd_;  // 文件描述符
    struct  sockaddr_in addr_;  // socke结构体

    bool isClose_;  // 是否关闭连接
    
    int iovCnt_;  // 结构体数组对应的内存标记块
    struct iovec iov_[2];  // 结构体数组 分别存储响应体和响应数据
    
    Buffer readBuff_; // 读缓冲区 保存请求数据的内容
    Buffer writeBuff_; // 写缓冲区  保存响应数据的内容

    HttpRequest request_;  // HTTP请求对象
    HttpResponse response_;  // HTTP响应对象
};


#endif //HTTP_CONN_H