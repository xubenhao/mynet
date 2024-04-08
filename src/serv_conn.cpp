#include "serv_conn.h"
namespace mynet{
ClientCallback::ClientCallback(){

}
void ClientCallback::Init(int32_t nId, IServerCallback* lpCall){
    m_nId = nId;
    m_lpCallback = lpCall;
}
ClientCallback::~ClientCallback(){

}
void ClientCallback::OnEvent(short nEvents){
    m_lpCallback->OnEvent(m_nId, nEvents);
}
void ClientCallback::OnMessage(char* lpMsg, int32_t nLen){
    return m_lpCallback->OnMessage(m_nId, lpMsg, nLen);
}

ServConn::ServConn(){

}
ServConn::~ServConn(){

}
int32_t ServConn::Init(int32_t nId, int32_t nFd, IServerCallback* lpCallback, EventBase* lpBase){
    m_stCall.Init(nId, lpCallback);
    m_lpBase = lpBase;
    m_nId = nId;
    m_nFd = nFd;
    m_stConn.Init(nFd, &m_stCall, lpBase);
    return 0;
}
void ServConn::UnInit(){
    m_stConn.UnInit();
}
int32_t ServConn::DoDisconnect(bool bBlock, bool bSyn){
    return m_stConn.DoDisconnect(bBlock, bSyn);
}
int32_t ServConn::SendData(char* lpData, int32_t nLen){
    return m_stConn.SendData(lpData, nLen);
}
}