#include "ifactory.h"
#include "ilog.h"
#include "iserver.h"
#include "define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>  
#include <chrono>
#include <mutex> 
#include <iostream>  
#include <fstream>
std::mutex mtx;  
std::ofstream logFile("svr_logfile.txt", std::ios_base::out);  
void logcb(mynet::LOGLEVEL nLevel, const char *msg){
    //if(nLevel == mynet::LOGLEVEL::EVENT_LOG_ERR){
    	pthread_t thread_id = pthread_self(); 
        std::lock_guard<std::mutex> lock(mtx);  
        logFile << "tid:" << (uint32_t)thread_id << " level: " << (int32_t)nLevel << ": msg: " << msg << std::endl; 
    //}
}

struct Msg1{
    int32_t nLen;
    int32_t nType;
    int32_t nMsg1;
    char strName[100];
    Msg1(){
        nLen = CalculateSize();
        nType = 1;
    }
    static int32_t CalculateSize(){
        return 112;
    }
    void Serialize(char* lpOut){
        int32_t nTmpLen = htobe32(nLen);
        memcpy(lpOut, (char*)&nTmpLen, 4);
        int32_t nTmpType = htobe32(nType);
        memcpy(lpOut+4, (char*)&nTmpType, 4);
        
        int32_t nMsg = htobe32(nMsg1);
        memcpy(lpOut+8, (char*)&nMsg, 4);
        memcpy(lpOut+12, strName, 100);
    }
    Msg1* DeSerialize(char* lpIn){
        Msg1* lpM = new Msg1();
        int32_t nTmpLen = *(int32_t*)lpIn;
        lpM->nLen = be32toh(nTmpLen);
        int32_t nTmpType = *(int32_t*)((char*)lpIn+4);
        lpM->nType = be32toh(nTmpType);

        int32_t nMsg = *(int32_t*)(lpIn+8);
        lpM->nMsg1 = be32toh(nMsg);
        memcpy(lpM->strName, lpIn+12, 100);
        return lpM;
    }
};
struct Msg2{
    int32_t nLen;
    int32_t nType;
    int32_t nMsg2;
    char strName1[100];
    char strName2[100];
    static int32_t CalculateSize(){
        return 212;
    }
    void Serialize(char* lpOut){
        int32_t nTmpLen = htobe32(nLen);
        memcpy(lpOut, (char*)&nTmpLen, 4);
        int32_t nTmpType = htobe32(nType);
        memcpy(lpOut+4, (char*)&nTmpType, 4);

        int32_t nMsg = htobe32(nMsg2);
        memcpy(lpOut+8, (char*)&nMsg, 4);
        memcpy(lpOut+12, strName1, 100);
        memcpy(lpOut+112, strName2, 100);
    }
    Msg2* DeSerialize(char* lpIn){
        Msg2* lpM = new Msg2();
        int32_t nTmpLen = *(int32_t*)lpIn;
        lpM->nLen = be32toh(nTmpLen);
        int32_t nTmpType = *(int32_t*)(lpIn+4);
        lpM->nType = be32toh(nTmpType);

        int32_t nMsg = *(int32_t*)(lpIn+8);
        lpM->nMsg2 = be32toh(nMsg);
        memcpy(lpM->strName1, lpIn+12, 100);
        memcpy(lpM->strName2, lpIn+112, 100);
        return lpM;
    }
};

class ServerCallback:public mynet::IServerCallback{
public:
    ServerCallback(){
    }
    virtual ~ServerCallback(){
        printf("~ServerCallback\n");
    }
    virtual void OnEvent(int32_t nIndex, short events){
        printf("index_%d,events_%d\n", nIndex, events);
    }
    virtual void OnMessage(int32_t nIndex, char* lpMsg, int32_t nLen){
        printf("index_%d,len_%d\n", nIndex, nLen);
        m_lpSvr->SendData(nIndex, lpMsg, nLen);
    }
public:
    mynet::IServer* m_lpSvr = nullptr;
};

int main(){
    mynet::SetLogCb(logcb);
    mynet::FactoryConfig stConfig;
    stConfig.nWorkThreadNum = 1;
    mynet::IFactory* lpFac = mynet::CreateFactory(stConfig);
    lpFac->Start();
   
    mynet::ServerConfig stSvrConfig;
    stSvrConfig.nPort = 13142;
    stSvrConfig.nListenThreadIndex = 0;
    
    ServerCallback stSvrCallback;
    mynet::IServer* lpSvr = lpFac->CreateServer(stSvrConfig, &stSvrCallback);
    stSvrCallback.m_lpSvr = lpSvr;
    lpSvr->Start();
    
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(6)); 
    }
    lpSvr->Stop();
    lpFac->Stop();
    mynet::DestroyFactory(lpFac);
    return 0;
}