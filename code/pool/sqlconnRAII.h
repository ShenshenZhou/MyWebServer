#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"

// RAII:资源在对象构造初始化 资源在对象析构时释放
// 然后在使用时创建类的一个对象作为局部变量，这项函数结束后就会自动销毁
class SqlConnRAII {
public:
    // 构造函数初始化资源
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }
    // 析构函数释放资源
    ~SqlConnRAII() {
        if(sql_) { connpool_->FreeConn(sql_); } // 这个释放就是把连接重回放回池子
    }
    
private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};

#endif //SQLCONNRAII_H