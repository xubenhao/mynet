#include "client.h"
#include "factory.h"
namespace mynet{
Client::Client(){
}

Client::~Client(){
}

int32_t Client::Init(Factory* lpOwner, const ClientConfig& stConfig, IClientCallback* lpCallback){
    m_lpOwner = lpOwner;
    m_nPort = stConfig.nPort;
    for(int32_t i = 0; i < 100; i++){
        m_strIp[i] = stConfig.strIp[i];
    }
    m_nConnTimeout = stConfig.nConnTimeout;
    m_lpCallback = lpCallback;
    m_lpBase = lpOwner->GetBase(stConfig.nThreadIndex);
    return m_stConn.Init(m_lpCallback, m_lpBase, m_nConnTimeout);
}
void Client::UnInit(){
    m_lpOwner = nullptr;
    m_lpCallback = nullptr;
    m_lpBase = nullptr;
    m_stConn.UnInit();
}

int32_t Client::DoConnect(int32_t nPort, char (&strIp)[100], int32_t nConnTimeout,
    bool bBlock, bool bSyn){
    return m_stConn.DoConnect(nPort, strIp, nConnTimeout, bBlock, bSyn);
}
int32_t Client::DoDisconnect(bool bBlock, bool bSyn){
    return m_stConn.DoDisconnect(bBlock, bSyn);
}
int32_t Client::SendData(char* lpData, int32_t nLen){
    return m_stConn.SendData(lpData, nLen);
}
}

