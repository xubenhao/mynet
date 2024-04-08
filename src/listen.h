#ifndef _MYNET_LISTEN_H
#define _MYNET_LISTEN_H
#include "event.h"
namespace mynet{
class Server;
class EventBase;
void ListenReadProcess(int32_t fd, short events, void* arg);
class Listen {
public:
    Listen();
    ~Listen();
    int32_t Init(Server* lpServer, EventBase* lpBase, int32_t nPort);
    void UnInit();
    int32_t Start();
    int32_t Stop();
public:
    void ReadEventProcess(int32_t nFd, short nEvents);
private:
    Server* m_lpOwner = nullptr;
    int32_t m_nPort = 0;
    EventBase* m_lpBase = nullptr;
    Event m_stListen;
    int32_t m_nFd = -1;
};
}
#endif