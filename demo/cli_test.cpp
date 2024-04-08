#include "ifactory.h"
#include "ilog.h"
#include "iclient.h"
#include "define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>  
#include <chrono>
#include <mutex> 
#include <iostream>  
#include <fstream>
#include <endian.h>

std::mutex mtx;  
std::ofstream logFile("cli_logfile.txt", std::ios_base::out);  
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
    Msg2(){
        nLen = CalculateSize();
        nType = 2;
    }
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
class ClientCallback:public mynet::IClientCallback{
public:
    virtual ~ClientCallback(){
        printf("~ClientCallback\n");
    }
    virtual void OnEvent(short events){
        printf("recv event %d\n", events);
    }
    virtual void OnMessage(char* lpMsg, int32_t nLen){
        printf("msg len:%d\n", nLen);
    }
};
int main(){
    mynet::SetLogCb(logcb);
    mynet::FactoryConfig stConfig;
    stConfig.nWorkThreadNum = 1;
    mynet::IFactory* lpFac = mynet::CreateFactory(stConfig);
    lpFac->Start();
    mynet::ClientConfig stCliConfig;
    stCliConfig.nConnTimeout = 4;
    stCliConfig.nPort = 13142;
    strcpy(stCliConfig.strIp, "127.0.0.1");
    stCliConfig.nThreadIndex = 0;

    ClientCallback stCall;
    mynet::IClient* lpCli = lpFac->CreateClient(stCliConfig, &stCall);
    int32_t nRet = lpCli->DoConnect(13142, stCliConfig.strIp, 4, true, true);
    printf("conn ret %d\n", nRet);
    if(nRet != RET_CODE_CONN_SUCC){
        return 0;
    }

    char* lpBuf = (char*)malloc(Msg1::CalculateSize());
    for(int32_t i = 0; i < 10; i++){
        Msg1 stM1;
        stM1.nMsg1 = 11;
        strcpy(stM1.strName, "StrName1"); 
        stM1.Serialize(lpBuf);
        int32_t nSend = lpCli->SendData(lpBuf, Msg1::CalculateSize());
        printf("%d send ret %d\n", i, nSend);
    }

    std::this_thread::sleep_for(std::chrono::seconds(6)); 
    lpCli->DoDisconnect(true, true);
    lpFac->Stop();
    mynet::DestroyFactory(lpFac);
    return 0;
}