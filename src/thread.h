#ifndef _MYNET_THREAD_H
#define _MYNET_THREAD_H
#include "std.h"
#include "event_base.h"
namespace mynet {
class Factory;
class Thread {
public:
    Thread(int32_t nIndex, Factory* lpOwner);
    virtual ~Thread();
    void Start();
    void Stop();
    void Fun();
    EventBase* GetBase();
private:
    int32_t m_nIndex;
    Factory* m_lpOwner;
    std::thread* m_lpThread;
    EventBase m_stBase;
};
}

#endif