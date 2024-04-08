#ifndef _MYNET_EPOLL_H
#define _MYNET_EPOLL_H
#include "epoll_table.h"
#include "iomulti.h"
namespace mynet{
class EventBase;
#define INVALID_EPOLL_HANDLE -1
#define INITIAL_NEVENT 32
#define MAX_NEVENT 4096
#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)
struct event_change {
	int32_t fd;
	short old_events;
	uint8_t read_change;
	uint8_t write_change;
	uint8_t close_change;
};

#define EV_CHANGE_ADD     0x01
#define EV_CHANGE_DEL     0x02
#define EV_CHANGE_PERSIST EV_PERSIST
#define EV_CHANGE_ET      EV_ET

class Epoll : public IoMulti
{
public:
    Epoll(EventBase*);
    ~Epoll();
    virtual int32_t Init();
	virtual int Add(int32_t fd, short old, short events, void *fdinfo);
	virtual int Del(int32_t fd, short old, short events, void *fdinfo);
	virtual int Dispatch(struct timeval *);
	virtual void UnInit();
private:
 	int ApplyOneChange(const struct event_change *ch);

private:
	int32_t m_nEpFd = INVALID_EPOLL_HANDLE;
	struct epoll_event *m_lpEvents = nullptr;
	int32_t m_nNum = 0;
};
}

#endif