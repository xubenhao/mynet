#ifndef _MYNET_EVENT_BASE_H
#define _MYNET_EVENT_BASE_H
#include "std.h"
#include "event.h"
#include "event_callback.h"
#include  "iomulti.h"
namespace mynet{
struct EventFdInfo{
	EventFdInfo(){
		m_nRead = 0;
		m_nWrite = 0;
		m_nClose = 0;
	}

	std::list<Event*> m_stEvents; 
	uint16_t m_nRead;
	uint16_t m_nWrite;
	uint16_t m_nClose;
};


#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define EV_CLOSURE_EVENT 0
#define EV_CLOSURE_EVENT_SIGNAL 1
#define EV_CLOSURE_EVENT_PERSIST 2
#define EV_CLOSURE_CB_SELF 3
#define EV_CLOSURE_CB_FINALIZE 4
#define EV_CLOSURE_EVENT_FINALIZE 5
#define EV_CLOSURE_EVENT_FINALIZE_FREE 6

#define EVENT_BASE_COUNT_ACTIVE        1U
#define EVENT_BASE_COUNT_VIRTUAL       2U
#define EVENT_BASE_COUNT_ADDED         4U

#define EVENT_MAX_PRIORITIES 256
#define EVLOOP_NONBLOCK	0x02

#define EVENT_DEL_NOBLOCK 0
#define EVENT_DEL_BLOCK 1
#define EVENT_DEL_AUTOBLOCK 2
#define EVENT_DEL_EVEN_IF_FINALIZING 3
void NotifyDrain(int32_t fd, short what, void *arg);
int Notify(EventBase* base);

struct EventMinHeap{
	Event** lpArrEvent = nullptr;
	int32_t nNum = 0;
	int32_t nCapacity = 0;
};

int32_t PushHeap(EventMinHeap& stHeap, Event* lpEvent);
int32_t PopHeap(EventMinHeap& stHeap);
int32_t PopHeap(EventMinHeap& stHeap, int nIndex);
Event* PeekHeap(EventMinHeap& stHeap);
int32_t FindHeap(EventMinHeap& stHeap, Event* lpEvent);
void FreeHeap(EventMinHeap& stHeap);
class EventBase{
public:
    EventBase();
    ~EventBase();
public:
	int32_t Init();
	void UnInit(int run_finalizers);
	int Loop(int flags);
	int EventAdd(Event* lpEvent, const struct timeval *tv = nullptr, bool bAbs = false);
	int EventDel(Event *lpEvent, int32_t nBlockingFlag = EVENT_DEL_AUTOBLOCK);
	
	void EventActive(Event* lpEvent, int res, short ncalls);
	void EventActiveLater(Event *lpEvent, int res);
	void EventCallbackActive(EventCallback *evcb);
	void EventCallbackActiveLater(EventCallback *evcb);

	void LoopBreak();
	void LoopContinue();
	void LoopTerm();
	void IoActive(int32_t fd, short events);
private:
	void TimeoutNext(struct timeval *lpTv);
	int MakeNotifiable();
	void ProcessActive();
public:
	IoMulti *evsel = nullptr;// 负责套接字event自动分发
	void *evbase = nullptr;// 辅助结构
    // 事件循环控制变量
    int32_t event_gotterm = 0;
	int32_t event_break = 0;
	int32_t event_continue = 0;

    //　实时状态 
	int32_t event_running_priority = 0;
	int32_t running_loop = 0;
	
	struct timeval tv_cache;
    // 运行此event_base的线程的线程id
	std::thread::id th_owner_id;
    // 递归互斥锁
	std::recursive_mutex th_base_lock;
    // 等待着数量
	int32_t current_event_waiters = 0;
    // 指向当前正被回调处理的event_callback
	EventCallback *current_event = nullptr;

	// 通知机制
	int32_t is_notify_pending = 0;
	int32_t th_notify_fd[2] = {-1, -1};
	// 内部event，服务于及时从io阻塞等待唤醒
	Event th_notify;
	int (*th_notify_fn)(EventBase *base) = nullptr;
	
	std::list<Event*> once_events;
	// 用map管理套接字event
	std::map<int32_t, EventFdInfo*> io;
	// 用heap管理定时event
	EventMinHeap timeheap;
	// 可以将每个优先级的激活的event_callback加入到链式结构
	std::list<EventCallback*> activequeues[2];
	std::list<EventCallback*> active_later_queue;
};
}

#endif