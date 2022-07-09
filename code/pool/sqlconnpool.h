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
    static SqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

    int MAX_CONN_;  // 最大连接数
    int useCount_;  // 当前用户数量
    int freeCount_;  // 空闲的用户数量

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;  // 互斥锁
    sem_t semId_;  // 信号量
};


#endif // SQLCONNPOOL_H