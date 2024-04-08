#ifndef _MYNET_SERV_CONN_H
#define _MYNET_SERV_CONN_H
#include "std.h"
#include "iserver.h"
#include "iclient.h"
#include "ifactory.h"
#include "connect.h"
namespace mynet{
class Factory;
class ClientCallback: public IClientCallback{
public:
    ClientCallback();
    void Init(int32_t nId, IServerCallback* lpCall);
    virtual ~ClientCallback();
    virtual void OnEvent(short );
    virtual void OnMessage(char* lpMsg, int32_t nLen);
public:
    int32_t m_nId = -1;
    IServerCallback* m_lpCallback = nullptr;
};

class ServConn {
public:
    ServConn();
    ~ServConn();
    int32_t Init(int32_t nId, int32_t nFd, IServerCallback* lpCallback, EventBase* lpBase);
    void UnInit();
    int32_t DoDisconnect(bool bBlock = true, bool bSyn = true);
    int32_t SendData(char* lpData, int32_t nLen);

private:
    ClientCallback m_stCall;
    EventBase* m_lpBase = nullptr;
    Connect m_stConn;
    int32_t m_nId = -1;
    int32_t m_nFd = -1;
};
}
#endif