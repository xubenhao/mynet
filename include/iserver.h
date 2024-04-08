#ifndef _MYNET_ISERVER_H
#define _MYNET_ISERVER_H
#include <stdlib.h>
#include "define.h"
namespace mynet
{
class IServerCallback{
public:
    IServerCallback(){}
    virtual ~IServerCallback(){}
    virtual void OnEvent(int32_t nIndex, short ) = 0;
    virtual void OnMessage(int32_t nIndex, char* lpMsg, int32_t nLen) = 0;
};

class IServer{
public:
    IServer(){}
    virtual ~IServer(){}
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual int32_t DoDisconnect(int32_t nIndex, bool bBlock = true, bool bSyn = true) = 0;
    virtual int32_t SendData(int32_t nIndex, char* lpData, int32_t nLen) = 0;
};
}
#endif