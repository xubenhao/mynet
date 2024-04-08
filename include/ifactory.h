#ifndef _MYNET_IFACTORY_H
#define _MYNET_IFACTORY_H
#include <stdlib.h>
namespace mynet
{

class IClientCallback;
class IClient;
class IServerCallback;
class IServer;

struct FactoryConfig{
    int32_t nWorkThreadNum = 0;
};

struct ClientConfig{
    int32_t nPort = -1;
    char strIp[100] = {'\0'};
    int32_t nThreadIndex = 0;
    int32_t nConnTimeout = 6;
};

struct ServerConfig{
    int32_t nPort = -1;
    int32_t nListenThreadIndex = 0;
};

class IFactory{
public:
    IFactory(){}
    virtual ~IFactory(){}
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual IClient* CreateClient(const ClientConfig& stConfig, IClientCallback* lpCallback) = 0;
    virtual void DestroyClient(IClient* lpClient) = 0;
    virtual IServer* CreateServer(const ServerConfig& stConfig, IServerCallback* lpCallback) = 0;
    virtual void DestroyServer(IServer* lpServer) = 0;
};

IFactory* CreateFactory(const FactoryConfig& stConfig);
void DestroyFactory(IFactory* lpFactory);
}
#endif