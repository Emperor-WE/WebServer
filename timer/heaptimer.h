#ifndef _WEB_SERVER_HREAP_TIMER_H_
#define _WEB_SERVER_HREAP_TIMER_H_

#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional>
#include <chrono>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "../log/log.h"

typedef std::function<void(int)> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;


struct TimerNode
{
    int id;                     //  fd
    TimeStamp expires;          // 超时时间点
    TimeoutCallBack cb;         // 回调function<void()>

    bool operator<(const TimerNode& t)
    {
        return expires < t.expires;
    }
    bool operator>(const TimerNode& t)
    {
        return expires > t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer()
    {
        heap_.reserve(256);
    }  // 保留（扩充）容量

    ~HeapTimer()
    { 
        clear();
    }
    
    void adjust(int id, int newExpires);
    void add(int id, int timeOut, const TimeoutCallBack& cb);
    void doWork(int id);
    void clear();
    void tick();
    void pop();
    int GetNextTick();

private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t i, size_t n);
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    // key:id value:vector的下标
    std::unordered_map<int, size_t> ref_;   // id对应的在heap_中的下标，方便用heap_的时候查找
};

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

    void do_work(int id);

public:
    static int *u_pipefd;
    HeapTimer m_heap;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(int fd);



#endif 