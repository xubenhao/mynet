# mynet

#### Introduce
Personal implementation of C++ open-source network library．

#### Software architecture
1. Structural diagram

![Structural diagram](https://foruda.gitee.com/images/1712475087930791242/9aa99396_13993444.png "屏幕截图")

2. Event based automatic distribution mechanism

![automatic distribution mechanism](https://foruda.gitee.com/images/1712475620967974857/7b17fd68_13993444.png "屏幕截图")

3. Multi priority distribution queue, delayed distribution queue

![distribution queue](https://foruda.gitee.com/images/1712475862800498143/afba04a0_13993444.png "屏幕截图")

The priority of internal events serving notification mechanisms is 0, while the priority of external events is 1.
When the distributed event_callback is centrally processed, if a higher priority event_callback is activated, 
the processing can be completed by calling back the current event_callback and entering the next time loop, 
so that the higher priority event_callback can be processed in a timely manner

4. Proactively distribute event_callback to submit callback tasks to worker threads

![Proactively distribute event_callback](https://foruda.gitee.com/images/1712476468072085716/f3d4d96a_13993444.png "屏幕截图")

5. Efficient cache management of communication objects

5.1. Using variable size blocks carrying management information as the basic caching unit

![variable size blocks](https://foruda.gitee.com/images/1712476672616771182/7419f63f_13993444.png "屏幕截图")

5.2. A cache area composed of a chain queue of variable size blocks

![cache area](https://foruda.gitee.com/images/1712476860378125472/7c4e4d48_13993444.png "屏幕截图")

5.3. Reusability of blocks

For blocks that need to be released due to consumption, cache is used instead of release for management.

![Cache of blocks](https://foruda.gitee.com/images/1712477088131554284/67548ae4_13993444.png "屏幕截图")

When releasing a block, based on the block capacity, release it to the container of the block specified in the cache
When a new block is needed, it is first fetched from the cache based on the required capacity. If it cannot be retrieved, a new block is dynamically allocated.

5.4. Connection management of connection objects
Using a mutex lock to implement the connection establishment process on the connection object, and the connection disconnection process can only have one concurrency at most.

a. Connection process

![Connection process](https://foruda.gitee.com/images/1712477863238957850/8cd44ca2_13993444.png "屏幕截图")

b. Disconnect process

![Disconnect process](https://foruda.gitee.com/images/1712478038360039641/41ddd2b0_13993444.png "屏幕截图")

c. Disconnect delivery and respond quickly

Set the priority of manually distributing event_callback to 0, and use the multi priority distribution queue of event_callback. 
This can make the current event_callback callback processing end and start the next cycle, thereby quickly processing high priority event_callback distributions.

5.5. Efficient lock management for connecting objects

a. Serialization of connection establishment and disconnection through connection locks

b. Readable event handling, packet receiving callback lockless processing

Because readable events and packet collection callbacks are only triggered on a single worker thread and are serialized through connection establishment and disconnection. 
The packet collection process and its callbacks can be implemented as lockless.

c. Implementing concurrent management of sending cache area through sending locks

The user thread performs sending, while the worker thread can write events and perform asynchronous sending, 
acting as the producers and consumers of the sending cache. We use sending mutexes for concurrency management

5.6. Efficient IO reuse

a. Using epoll as the IO multiplexer, it performs better than select and poll in managing large-scale event monitoring．

b. Only register connection objects when necessary to write events to event_base．

b.1. The connection establishment process is registered with event_base to achieve asynchronous processing of connection results．

b.2. When the user thread writes new data to the send cache, we register it with event_base to achieve asynchronous sending of data in writable events．

b.3. In asynchronous sending, when the sending cache is determined to be empty, automatically remove writable events to reduce unnecessary event distribution．

5.7. Simple and easy-to-use

a. Implemented in C++.

b. Manage resources in a factory mode.

c. The interface definition is clear, please refer to the user manual for details.

#### system requirements 

1. Supports c++11
2. Supports CMake
3. Linux system

#### Installation Tutorial

1. Execute cmake in mynet/build/
2. Execute: make under mynet/build
3. Execute under mynet/build/demo:/ srv_test enable server
4. Execute under mynet/build/demo:/ cli_test enable client

#### Instructions for use

1.  Client demo

```
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
```

2.  Server demo

```
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
```


#### Participate in contributions

1.  Fork warehouse
2.  Create a new Feat_xxx branch
3.  Submit Code
4.  Create a new Pull Request


#### Subsequent pending matters

1. Improve unit testing for various scenarios
2. Supporting epoll for ET mode event distribution
3. Create standardized and clear usage documents
