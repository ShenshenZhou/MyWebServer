#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

// HTTP响应类 将响应封装成HttpResponse对象
class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    // HTTP响应初始化
    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
     // 创建响应
    void MakeResponse(Buffer& buff);
    // 解除内存映射
    void UnmapFile();
    // 返回映射的文件
    char* File();
    // 返回文件长度信息
    size_t FileLen() const;
    // 追加打开文件资源失败的错误信息并返回
    void ErrorContent(Buffer& buff, std::string message);
    // 返回响应状态码
    int Code() const { return code_; }

private:
    void AddStateLine_(Buffer &buff);// 添加响应行
    void AddHeader_(Buffer &buff); // 添加响应头
    void AddContent_(Buffer &buff);// 添加响应体

    void ErrorHtml_(); // 错误网页路径：判断路径是否在给定的响应状态码对应的路径中
    std::string GetFileType_();// 获取文件类型

    int code_;  // 响应状态码
    bool isKeepAlive_;  // 是否保持连接 

    std::string path_;  // 资源的路径
    std::string srcDir_;  // 资源的目录
    
    char* mmFile_;  // 文件内存映射指针
    struct stat mmFileStat_;  // 文件的状态信息

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  // 后缀-类型
    static const std::unordered_map<int, std::string> CODE_STATUS;  // 状态码-描述
    static const std::unordered_map<int, std::string> CODE_PATH;  // 状态码-路径
};


#endif //HTTP_RESPONSE_H