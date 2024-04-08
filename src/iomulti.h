#ifndef _MYNET_IO_MULTI_H
#define _MYNET_IO_MULTI_H
#include "std.h"
namespace mynet{
class EventBase;
#define EV_FEATURE_ET 0x01
#define EV_FEATURE_O1 0x02
#define EV_FEATURE_FDS 0x04
#define EV_FEATURE_EARLY_CLOSE 0x08

class IoMulti{
public:
    IoMulti(EventBase *lpBase){
		m_stFeatures = 0;
		m_nFdInfoLen = 0;
		m_lpBase = nullptr;
	}
    virtual ~IoMulti() {}
	virtual int32_t Init() = 0;
	virtual int Add(int32_t fd, short old, short events, void *fdinfo) = 0;
	virtual int Del(int32_t fd, short old, short events, void *fdinfo) = 0;
	virtual int Dispatch(struct timeval *) = 0;
	virtual void UnInit() = 0;
protected:
    std::string m_strName;
	int32_t m_stFeatures;
	size_t m_nFdInfoLen;
	EventBase* m_lpBase;
};

}
#endif