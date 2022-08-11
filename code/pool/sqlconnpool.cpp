#include "sqlconnpool.h"
using namespace std;

// 构造函数：初始化用户数量和空闲连接数量
SqlConnPool::SqlConnPool() {
    useCount_ = 0;
    freeCount_ = 0;
}

// 单例模式：定义一个实例
SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

// 初始化
void SqlConnPool::Init(const char* host, int port,
            const char* user,const char* pwd, const char* dbName,
            int connSize = 10) {  // 默认最大连接数量为10
    
    assert(connSize > 0);
    // 循环创建10个数据库对象，push到连接队列中
    for (int i = 0; i < connSize; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);  // 初始化信号量为10
}

// 获取连接
MYSQL* SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    // 如果连接队列为空，说明连接全部被使用
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);  // 信号量>0 往下执行，=0，在此等待不为0
    {
        // 取出队列第一个连接
        lock_guard<mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

// 释放连接
void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(sql);  // 添加连接到队列中
    sem_post(&semId_);  // 信号量+1
}

// 关闭数据库连接池
void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);
    // 如果非空，就循环取出所有连接
    while(!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    // 为空，则直接关闭数据库
    mysql_library_end();        
}

// 得到空闲连接的数量
int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}

// 析构函数
SqlConnPool::~SqlConnPool() {
    ClosePool();
}
