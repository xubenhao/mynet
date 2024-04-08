#include "thread.h"
namespace mynet{
Thread::Thread(int32_t nIndex, Factory* lpOwner){
    m_nIndex = nIndex;
    m_lpOwner = lpOwner;
    m_lpThread = nullptr;
}
Thread::~Thread(){
    if(m_lpThread){
        m_lpThread->join();
        delete m_lpThread;
    }
    m_stBase.UnInit(1);
}
void Thread::Start(){
    m_stBase.Init();
    m_lpThread = new std::thread(&Thread::Fun, this);
}
void Thread::Stop(){
    m_stBase.LoopBreak();
}
void Thread::Fun(){
    m_stBase.Loop(1);
}
EventBase* Thread::GetBase(){
    return &m_stBase;
}
}