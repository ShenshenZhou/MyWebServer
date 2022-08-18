#include "buffer.h"

// 初始化缓冲数组：默认大小，读的位置0，写的位置0
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

// 可读大小，写的位置-读的位置
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 可写大小：缓存大小-写的位置
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 已经读完的空间：就是读的位置
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 缓冲区开始位置到读的位置这一段偏移量
const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

// 更新读指针的位置
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

// 读完数据后更新读指针
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

// 开始写的位置
char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

// 写入数据后更新写的位置
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
} 

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

// Append(buff, len - writable)：buff是临时数组，len-writable表示多余数据的大小
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);  // 确保有空间可以写入数据
    std::copy(str, str + len, BeginWrite());  // 把暂存在临时数组中的数据拷贝缓冲区
    HasWritten(len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 确保有空间可以写入数据
void Buffer::EnsureWriteable(size_t len) {
    if(WritableBytes() < len) {
        MakeSpace_(len);  // 如果可写空间小于数据长度，则创建新的空间
    }
    assert(WritableBytes() >= len);
}

// 分散读数据
ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char buff[65535];  // 临时数组：临时存放缓冲数组中写不下的数据
    struct iovec iov[2];  // 结构体数组：描述一块内存区
    const size_t writable = WritableBytes();  // 可写大小
    // 内存区1：缓冲数组剩余空间
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    // 临时数组
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    // 分散读
    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        // 可写空间大于所读数大小
        writePos_ += len;
    }
    else {
        // 可写空间小于所读数据大小
        writePos_ = buffer_.size();  // 写的位置移动到末尾 表示已经写满
        Append(buff, len - writable);  // 多余的数据写到临时数组中
    }
    return len;
}

// 写数据
ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    readPos_ += len;
    return len;
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

// 创建空间
void Buffer::MakeSpace_(size_t len) {
    if(WritableBytes() + PrependableBytes() < len) {
        // 如果可写空间+已经读完的空间<数据大小 则resize扩容
        buffer_.resize(writePos_ + len + 1);
    } 
    else {
        // 如果>数据大小 移动当前缓冲区的数组到数组头部 然后将数据放入剩余空间中
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}