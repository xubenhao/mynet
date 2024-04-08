#include "event_callback.h"

namespace mynet{
EventCallback::EventCallback(Event* lpEvent){
    m_nFlags = 0;
	m_nPri = 0;	
	m_nClosure = EV_CLOSURE_EVENT;
	m_lpArg = nullptr;
	m_lpOwner = nullptr;
}

EventCallback::~EventCallback(){
    
}
}