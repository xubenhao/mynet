#ifndef _MYNET_CONNECT_H
#define _MYNET_CONNECT_H
#include "std.h"
#include "event_callback.h"
#include "event.h"
#include "event_base.h"
#include "evbuffer.h"
#include "iclient.h"
namespace mynet{
#define CONN_STATUS_CLOSED            1
#define CONN_STATUS_CONNECTING        2
#define CONN_STATUS_CONNECTED         3
#define CONN_STATUS_DISCONNECTING     4


#define BEV_EVENT_READING	0x01	
#define BEV_EVENT_WRITING	0x02	
#define BEV_EVENT_EOF		0x10	
#define BEV_EVENT_ERROR		0x20	
#define BEV_EVENT_TIMEOUT	0x40	
#define BEV_EVENT_CONNECTED	0x80	
void ConnReadProcess(int32_t fd, short events, void* arg);
void ConnWriteProcess(int32_t fd, short events, void* arg);
void ConnDisconnProcess(EventCallback*, void* arg);
class Connect {
public:
    Connect();
    virtual ~Connect();
    // 服务于主动连接
    int32_t Init(IClientCallback* lpCallback, EventBase* lpBase, int32_t nConnTimeout);
    // 服务于被动连接
    int32_t Init(int32_t nFd, IClientCallback* lpCallback, EventBase* lpBase);
    // 块缓存区资源释放
    int32_t UnInit();


    // 服务于主动连接：作基础的创建套接字，发出连接请求，向关联base注册event操作
    int32_t DoConnect(int32_t nPort, char (&strIp)[100], int32_t nConnTimeout,
        bool bBlock = true, bool bSyn = false);
    // 作基础的向关联base取消event，关闭套接字操作
    int32_t DoDisconnect(bool bBlock = true, bool bSyn = true);
    
    int32_t SendData(char* lpData, int32_t nLen);
public:
    void ReadEventProcess(int32_t nFd, short nEvents);
    void WriteEventProcess(int32_t nFd, short nEvents);
    void DisconnProcess(EventCallback*);
private:
    void RecvDataProcess(int32_t nFd);

private:
    // 1."发送缓存区"，"接收缓存区"
    // 2."可读事件监控"，可读事件处理
    // 3."可写事件监控"，可写事件处理
    // 4.发送缓存区为空时，自动取消可写事件监控．非空时，自动添加可写事件监控．
    // 5.自动引发"上层的事件回调"
    // 6.自动引发"上层的收包回调"
    // 7.断开连接
    // 8.主动发送数据
    // 9.连接对象释放
	
	Event m_stRead;
	Event m_stWriteForConn;
    Event m_stWrite;
    EventCallback m_stDisconnCallback;
    // 接收缓存区可以处理得简单些
    char *m_lpRecvBuff = nullptr;
    int32_t m_nRecvCapacity = 0;
    int32_t m_nRecvOffset = 0;
    int32_t m_nRecvLength = 0;

	Buffer m_stOutput;
    std::mutex m_stSendMutex;  

    int32_t m_nConnTimeout = 10;
    timeval m_stConnTimeout;
    IClientCallback* m_lpCallback = nullptr;
    EventBase *m_lpBase = nullptr;

    int32_t m_nFd;
    int32_t m_nNextPackLen = -1;
    
    int32_t m_nConnStatus;
    // 连接互斥锁
    std::mutex m_stConnMutex;  

    // 发送缓存的生产者是事件循环线程，消费者也是事件循环线程－－无需加锁保护
    // 接收缓存的生产者是用户线程，消费者事件循环线程－－需加锁保护

    // 作为客户端主动连接的时序控制：
    // 连接建立阶段
    // 数据发送
    // 数据接收与回调
    // 连接关闭阶段
    // 作为服务端被动连接的时序控制：
    // 连接建立，向关联base注册事件监控
    // 数据发送
    // 数据接收与回调
    // 连接关闭阶段
};
}
#endif