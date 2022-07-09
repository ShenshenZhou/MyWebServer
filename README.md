## 项目介绍

用C++实现的高性能WebServer，经过webbench压力测试可以实现上万的QPS。

## 环境配置

* Linux（ubuntu18）
* MySQL
* C++11

## 目录结构

```bash
.
├── bin			# 可执行文件
├── build		# Makefile
├── code		# 源代码
│   ├── buffer
│   ├── http
│   ├── log
│   ├── pool
│   ├── server
│   └── timer
├── log			# 日志文件
├── resources	# 静态资源
│   ├── css
│   ├── fonts
│   ├── images
│   ├── js
│   └── video
└── webbench-1.5# 压力测试
```

## 项目功能

* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用标准库容器封装char，实现自动增长的缓冲区；
* 基于小根堆实现的定时器，关闭超时的非活动连接；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；
* 利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能。
