#ifndef _MYNET_EVENT_H
#define _MYNET_EVENT_H
#include "std.h"
#include "event_callback.h"
namespace mynet{
class EventBase;
// 监控的事件信息
#define EV_TIMEOUT	0x01
#define EV_READ		0x02
#define EV_WRITE	0x04

#define EV_PERSIST	0x10
#define EV_ET		0x20
#define EV_FINALIZE     0x40
#define EV_CLOSED	0x80

class Event{
public:
	Event();
	~Event(){}
	void Init(EventBase *base, int32_t fd, short events, void (*callback)(int32_t, short, void *), void *arg);
public:
	struct EventCallback m_stCallback;
	int32_t m_nFd;
	short m_nEvents;
	short m_nRes;		
	struct EventBase *m_lpBase;
	struct timeval m_nTimeoutInterval;// 服务于持久性定时存储间隔
	struct timeval m_nTimeout;// 服务于定时时存储下个超时时间点．
};
}
#endif