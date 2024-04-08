#ifndef _MYNET_SERVER_H
#define _MYNET_SERVER_H
#include "std.h"
#include "iserver.h"
#include "ifactory.h"
#include "serv_conn.h"
#include "listen.h"
namespace mynet{
class Factory;
class Server : public IServer{
public:
    Server();
    virtual ~Server();
    int32_t Init(Factory* lpFactory, const ServerConfig& stConfig, IServerCallback* lpCallback);
    void UnInit();
    virtual void Start();
    virtual void Stop();
    virtual int32_t DoDisconnect(int32_t nIndex, bool bBlock = true, bool bSyn = true);
    virtual int32_t SendData(int32_t nIndex, char* lpData, int32_t nLen);
    EventBase* GetBase(int32_t nThreadIndex);
    ServConn* GetConnect();
    int32_t GetNextThreadIndex();
    int64_t GetNextId();
    IServerCallback* GetServerCallback();
    void RegisterServConn(int64_t nId, ServConn* lpConn);
private:
    Factory* m_lpOwner = nullptr;
    int32_t m_nPort;
    int32_t m_nListenIndex;
    EventBase* m_lpBase = nullptr;
    IServerCallback* m_lpCallback = nullptr;
    std::map<int64_t, ServConn*> m_stUsingConns;
    std::list<ServConn*> m_stConnPool;
    int64_t m_nNextId = 0;
    Listen m_stListen;

};
}
#endif