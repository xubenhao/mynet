#ifndef _MYNET_EVENT_CALLBACK_H
#define _MYNET_EVENT_CALLBACK_H
#include "std.h"
namespace mynet{
// 闭包类型
#define EV_CLOSURE_EVENT 0
#define EV_CLOSURE_EVENT_PERSIST 2
#define EV_CLOSURE_CB_SELF 3
#define EV_CLOSURE_CB_FINALIZE 4
#define EV_CLOSURE_EVENT_FINALIZE 5
#define EV_CLOSURE_EVENT_FINALIZE_FREE 6

// 状态标志
#define EVLIST_TIMEOUT	    0x01
#define EVLIST_INSERTED	    0x02
#define EVLIST_ACTIVE	    0x08
#define EVLIST_INTERNAL	    0x10
#define EVLIST_ACTIVE_LATER 0x20
#define EVLIST_FINALIZING   0x40
#define EVLIST_INIT	        0x80
#define EVLIST_ALL          0xff
class Event;
class EventCallback{
public:
    EventCallback(Event* lpEvent = nullptr);
    ~EventCallback();
public:
	short m_nFlags;
	uint8_t m_nPri;	/* smaller numbers are higher priority */
	uint8_t m_nClosure;
    union {
		void (*evcb_callback)(int32_t, short, void *);
		void (*evcb_evfinalize)(Event *, void *);
		void (*evcb_selfcb)(EventCallback *, void *);
		void (*evcb_cbfinalize)(EventCallback *, void *);
	} m_stCbUnion;
	void *m_lpArg;
	Event* m_lpOwner;
};
}

#endif