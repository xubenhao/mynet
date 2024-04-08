#ifndef _MYNET_ICLIENT_H
#define _MYNET_ICLIENT_H
#include <stdlib.h>
#include "define.h"
namespace mynet
{
class IClientCallback{
public:
    virtual ~IClientCallback(){}
    virtual void OnEvent(short ) = 0;
    virtual void OnMessage(char* lpMsg, int32_t nLen) = 0;
};

class IClient{
public:
    virtual ~IClient(){}
    virtual int32_t DoConnect(int32_t nPort, char (&strIp)[100], int32_t nConnTimeout,
        bool bBlock = true, bool bSyn = false) = 0;
    virtual int32_t DoDisconnect(bool bBlock = true, bool bSyn = true) = 0;
    virtual int32_t SendData(char* lpData, int32_t nLen) = 0;
};
}
#endif