#include "std.h"
#include "factory.h"
#include "client.h"
#include "server.h"
#include "thread.h"
namespace mynet{
IFactory* CreateFactory(const FactoryConfig& stConfig){
    Factory *lpFac = new Factory();
    if(lpFac){
        lpFac->Init(stConfig);
    }
    return lpFac;
}

void DestroyFactory(IFactory* lpFactory){
    if(lpFactory){
        lpFactory->Stop();
        delete lpFactory;
    }
}

Factory::Factory(){
}

int32_t Factory::Init(const FactoryConfig& stConfig){
    m_stConfig = stConfig;
    for(int32_t i = 0; i < m_stConfig.nWorkThreadNum; i++){
        Thread* lpThread = new Thread(i, this);
        m_stlpThreads.push_back(lpThread);
    }
}

void Factory::Start(){
    for(int32_t i = 0; i < m_stConfig.nWorkThreadNum; i++){
        m_stlpThreads[i]->Start();
    }
}
void Factory::Stop(){
    for(int32_t i = 0; i < m_stConfig.nWorkThreadNum; i++){
        m_stlpThreads[i]->Stop();
    }
}

void Factory::UnInit(){
    for(auto iter = m_stlpThreads.begin(); iter != m_stlpThreads.end(); iter++){
        delete (*iter);
    }
    m_stlpThreads.clear();
}

IClient* Factory::CreateClient(const ClientConfig& stConfig, IClientCallback* lpCallback){
    std::lock_guard<std::mutex> lock(m_stMutex);
    Client* lpClient = new Client();
    lpClient->Init(this, stConfig, lpCallback);
    m_stlpClients.push_back(lpClient);
    return lpClient;
}
void Factory::DestroyClient(IClient* lpClient){
    {
        std::lock_guard<std::mutex> lock(m_stMutex);
        for(auto iter = m_stlpClients.begin(); iter != m_stlpClients.end(); iter++){
            if(lpClient == *iter){
                m_stlpClients.erase(iter);
                break;
            }
        }
    }
    Client* lpCli = (Client*)lpClient;
    lpCli->UnInit();
    delete lpClient;
}

IServer* Factory::CreateServer(const ServerConfig& stConfig, IServerCallback* lpCallback){
    std::lock_guard<std::mutex> lock(m_stMutex);
    Server* lpServer = new Server();
    lpServer->Init(this, stConfig, lpCallback);
    m_stlpServers.push_back(lpServer);
    return lpServer;
}

void Factory::DestroyServer(IServer* lpServer){
    {
        std::lock_guard<std::mutex> lock(m_stMutex);
        for(auto iter = m_stlpServers.begin(); iter != m_stlpServers.end(); iter++){
            if(lpServer == *iter){
                m_stlpServers.erase(iter);
                break;
            }
        }
    }
    Server* lpSvr = (Server*)lpServer;
    lpSvr->UnInit();
    delete lpSvr;
}

Factory::~Factory()
{
    for(int32_t i = 0; i < m_stlpClients.size(); i++){
        DestroyClient(m_stlpClients[i]);
    }

    for(int32_t i = 0; i < m_stlpServers.size(); i++){
        DestroyServer(m_stlpServers[i]);
    }
}

EventBase* Factory::GetBase(int32_t nThreadIndex){
    if(nThreadIndex < 0 || nThreadIndex >= m_stlpThreads.size()){
        return nullptr;
    }
    return m_stlpThreads[nThreadIndex]->GetBase();
}
int32_t Factory::GetNextThreadIndex(){
    int32_t nIndex = m_nNextThreadIndex;
    m_nNextThreadIndex = m_nNextThreadIndex + 1;
    m_nNextThreadIndex = m_nNextThreadIndex % m_stlpThreads.size();
    return nIndex;
}
}