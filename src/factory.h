#ifndef _MYNET_FACTORY_H
#define _MYNET_FACTORY_H
#include "std.h"
#include "ifactory.h"
namespace mynet{
class Thread;
class Client;
class Server;
class EventBase;
class Factory : public IFactory
{
public:
    Factory();
    virtual ~Factory();
    virtual int32_t Init(const FactoryConfig& stConfig);
    virtual void Start();
    virtual void Stop();
    virtual void UnInit();
    virtual IClient* CreateClient(const ClientConfig& stConfig, IClientCallback* lpCallback);
    virtual void DestroyClient(IClient* lpClient);
    virtual IServer* CreateServer(const ServerConfig& stConfig, IServerCallback* lpCallback);
    virtual void DestroyServer(IServer* lpServer);
    EventBase* GetBase(int32_t nThreadIndex);
    int32_t GetNextThreadIndex();
private:
    int32_t m_nNextThreadIndex = 0;
    FactoryConfig m_stConfig;
    std::vector<Thread*> m_stlpThreads;
    std::vector<IClient*> m_stlpClients;
    std::vector<IServer*> m_stlpServers;
    std::mutex m_stMutex;                    // 互斥锁，保护共享数据  
    //std::condition_variable m_stConVar;      // 条件变量，用于等待或通知  
};
}
#endif