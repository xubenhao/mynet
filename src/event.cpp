#include "event.h"
#include "log.h"
#include "event_base.h"
namespace mynet{

Event::Event()
{
	m_nFd =-1;
	m_nEvents = 0;
	m_nRes = 0;		
	m_lpBase = nullptr;
	m_nTimeoutInterval.tv_sec = 0;// 服务于持久性定时存储间隔
	m_nTimeoutInterval.tv_usec = 0;
	m_nTimeout.tv_sec = 0;// 服务于定时时存储下个超时时间点．
	m_nTimeout.tv_usec = 0;
}

void Event::Init(EventBase *base, int32_t fd, short events, void (*callback)(int32_t, short, void *), void *arg)
{
	m_lpBase = base;
	m_nFd = fd;
	m_nEvents = events;
	m_nRes = 0;
	m_stCallback.m_stCbUnion.evcb_callback = callback;// callback类型需要和ev_closure匹配
	m_stCallback.m_lpArg = arg;
	m_stCallback.m_nFlags = EVLIST_INIT;// 具备INIT标志的event_callback存在关联的event
	if (events & EV_PERSIST) {
		m_nTimeoutInterval.tv_sec = 0;
		m_nTimeoutInterval.tv_usec = 0;
		m_stCallback.m_nClosure = EV_CLOSURE_EVENT_PERSIST;
	} else {
		m_stCallback.m_nClosure = EV_CLOSURE_EVENT;
	}
	m_stCallback.m_nPri = 1;
	m_stCallback.m_lpOwner = this;
}
}