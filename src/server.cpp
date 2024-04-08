#include "server.h"
#include "factory.h"
namespace mynet{

Server::Server(){

}
Server::~Server(){

}
int32_t Server::Init(Factory* lpFactory, const ServerConfig& stConfig, IServerCallback* lpCallback){
    m_lpOwner = lpFactory;
    m_nPort = stConfig.nPort;
    m_lpCallback = lpCallback;
    m_lpBase = m_lpOwner->GetBase(stConfig.nListenThreadIndex);
    m_stListen.Init(this, m_lpBase, stConfig.nPort);
    return 0;
}
void Server::UnInit(){
    // 断开并回收所有连接对象｀
    for(auto iter = m_stUsingConns.begin(); iter != m_stUsingConns.end(); iter++){
        iter->second->UnInit();
        free(iter->second);
    }
    m_stUsingConns.clear();
    for(auto iter = m_stConnPool.begin(); iter != m_stConnPool.end(); iter++){
        free(*iter);
    }
    m_stConnPool.clear();
    m_stListen.UnInit();
}
void Server::Start(){
    // 启动Listen
    m_stListen.Start();
}
void Server::Stop(){
    // 停止Listen
    m_stListen.Stop();
}
int32_t Server::DoDisconnect(int32_t nIndex, bool bBlock, bool bSyn){
    // 断开指定连接
    auto iter = m_stUsingConns.find(nIndex);
    if(iter != m_stUsingConns.end()){
        int32_t nRet = iter->second->DoDisconnect(bBlock, bSyn);
        m_stUsingConns.erase(iter->first);
        return nRet;
    }
    else{
        return RET_CODE_INDEX_ERR;
    }
}
int32_t Server::SendData(int32_t nIndex, char* lpData, int32_t nLen){
    // 发送数据
    auto iter = m_stUsingConns.find(nIndex);
    if(iter != m_stUsingConns.end()){
        return iter->second->SendData(lpData, nLen);
    }
    else{
        return RET_CODE_INDEX_ERR;
    }
}
EventBase* Server::GetBase(int32_t nThreadIndex){
    return m_lpOwner->GetBase(nThreadIndex);
}
ServConn* Server::GetConnect(){
    bool bEmpty = m_stConnPool.empty();
    ServConn* lpConn = nullptr;
    if(bEmpty == false){
        lpConn = m_stConnPool.front();
        m_stConnPool.pop_front();
    }else{
        lpConn = new ServConn();
    }
    return lpConn;
}
int32_t Server::GetNextThreadIndex(){
    return m_lpOwner->GetNextThreadIndex();
}
int64_t Server::GetNextId(){
    return m_nNextId++;
}
IServerCallback* Server::GetServerCallback(){
    return m_lpCallback;
}
void Server::RegisterServConn(int64_t nId, ServConn* lpConn){
    m_stUsingConns.insert(std::pair<int64_t, ServConn*>(nId, lpConn));
}
}
