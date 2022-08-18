#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

// HTTP请求类 将请求封装成HttpRequest对象
class HttpRequest {
public:
    // 解析状态
    enum PARSE_STATE {
        REQUEST_LINE,  // 正在解析首行
        HEADERS,  // 投头
        BODY,  // 体
        FINISH,  // 完成   
    };

    // HTTP状态码
    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,// 获取到请求
        BAD_REQUEST,// 错误的请求
        NO_RESOURSE,// 没有资源
        FORBIDDENT_REQUEST,// 禁止请求
        FILE_REQUEST,// 文件请求
        INTERNAL_ERROR, // 内部错误
        CLOSED_CONNECTION,// 连接关闭
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    // 初始化HTTP请求状态
    void Init();
    // 解析函数
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;// 获取Post表单
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;// 是否保持连接

private:
    bool ParseRequestLine_(const std::string& line);// 解析请求首行
    void ParseHeader_(const std::string& line); // 解析请求头
    void ParseBody_(const std::string& line);// 解析请求体

    void ParsePath_();// 解析请求路径
    void ParsePost_();// 解析post请求
    void ParseFromUrlencoded_();// 解析表单数据
    // 验证用户
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;  // 解析的状态
    std::string method_, path_, version_, body_;  // 方法 路径 协议版本 请求体
    std::unordered_map<std::string, std::string> header_;  // 请求头
    std::unordered_map<std::string, std::string> post_;  // post请求表单数据

    static const std::unordered_set<std::string> DEFAULT_HTML;  // 默认网页
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;// 用户注册登录网页路径
    static int ConverHex(char ch);  // 转换成十六进制
};


#endif //HTTP_REQUEST_H