#ifndef _MYNET_CLIENT_H
#define _MYNET_CLIENT_H
#include "std.h"
#include "iclient.h"
#include "ifactory.h"
#include "connect.h"
namespace mynet{
class Factory;
class Client : public IClient{
public:
    Client();
    ~Client();
    int32_t Init(Factory* lpFactory, const ClientConfig& stConfig, IClientCallback* lpCallback);
    void UnInit();
    int32_t DoConnect(int32_t nPort, char (&strIp)[100], int32_t nConnTimeout,
        bool bBlock = true, bool bSyn = false);
    int32_t DoDisconnect(bool bBlock = true, bool bSyn = true);
    int32_t SendData(char* lpData, int32_t nLen);

private:
    Factory* m_lpOwner = nullptr;
    int32_t m_nPort;
    char m_strIp[100];
    EventBase* m_lpBase = nullptr;
    int32_t m_nConnTimeout;
    IClientCallback* m_lpCallback = nullptr;
    Connect m_stConn;
};
}
#endif