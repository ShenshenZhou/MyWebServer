#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    // 单例模式：静态成员变量
    static SqlConnPool *Instance();

    // 获取连接
    MYSQL *GetConn();
    // 释放连接  
    void FreeConn(MYSQL * conn);
    // 获取空闲的连接数量
    int GetFreeConnCount();

    // 初始化：主机名，端口，用户，密码，数据库名，连接数量
    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();

private:
    // 单例模式：私有的构造函数和析构函数
    SqlConnPool();
    ~SqlConnPool();

    int MAX_CONN_;  // 最大连接数
    int useCount_;  // 当前用户数量
    int freeCount_;  // 空闲的用户数量

    std::queue<MYSQL *> connQue_;  // 连接队列
    std::mutex mtx_;  // 互斥锁
    sem_t semId_;  // 信号量
};


#endif // SQLCONNPOOL_H