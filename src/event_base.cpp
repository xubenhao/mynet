#include "event_base.h"
#include "epoll.h"
#include "log.h"
#include "util.h"

namespace mynet{
int32_t PushHeap(EventMinHeap& stHeap, Event* lpEvent){
	if(stHeap.nNum == stHeap.nCapacity){
		int32_t nNewCap = stHeap.nCapacity * 2;
		if(nNewCap < 100){
			nNewCap = 100;
		}
		Event** lpNewArr = (Event**)malloc(nNewCap * sizeof(Event*));
		if(lpNewArr == nullptr){
			return -1;
		}
		for(int32_t i = 0; i < nNewCap; i++){
			if(i < stHeap.nCapacity){
				*(lpNewArr+i) = *(stHeap.lpArrEvent+i);
			}else{
				*(lpNewArr+i) = nullptr;
			}
		}

		stHeap.lpArrEvent = lpNewArr;
		stHeap.nCapacity = nNewCap;
	}

	int32_t nCurIndex = stHeap.nNum;
	*(stHeap.lpArrEvent+stHeap.nNum) = lpEvent;
	stHeap.nNum++;

	// index-2*index+1,2*index+2
	// 需不断对新节点和其父节点进行迭代分析
	while(nCurIndex != 0){
		int32_t nParentIndex = nCurIndex / 2;
		if(nCurIndex % 2 == 0){
			nParentIndex--;
		}
		// 若父节点的超期时间点小于等于孩子节点，则停止迭代．否则，交换．
		Event* lpParent = *(stHeap.lpArrEvent+nParentIndex);
		Event* lpChild = *(stHeap.lpArrEvent+nCurIndex);
		if(lpParent->m_nTimeout.tv_sec < lpChild->m_nTimeout.tv_sec
			|| lpParent->m_nTimeout.tv_usec <= lpChild->m_nTimeout.tv_usec){
			break;
		}else{
			// 交换
			*(stHeap.lpArrEvent+nParentIndex) = lpChild;
			*(stHeap.lpArrEvent+nCurIndex) = lpParent;
			nCurIndex = nParentIndex;
		}
	}
	return 0;
}
int32_t PopHeap(EventMinHeap& stHeap){
	if(stHeap.nNum == 0){
		return -1;
	}
	// 交换首尾
	Event* lpFirst = stHeap.lpArrEvent[0];
	Event* lpLast = stHeap.lpArrEvent[stHeap.nNum-1];
	stHeap.lpArrEvent[0] = lpLast;
	stHeap.lpArrEvent[stHeap.nNum-1] = lpFirst;
	stHeap.nNum--;// 实现尾部元素移除
	// 从首元素开始，比较父节点p和左右孩子中较小者q．
	// 若p小于等于q，停止迭代．否则，交换．继续迭代．
	int32_t nCurIndex = 0;
	while(2*nCurIndex+1 < stHeap.nNum){
		int32_t nLeftChild = 2*nCurIndex+1;
		int32_t nMinChild = nLeftChild;
		if(nLeftChild+1 < stHeap.nNum){
			int32_t nRightChild = nLeftChild+1;
			Event* lpLeft = stHeap.lpArrEvent[nLeftChild];
			Event* lpRight = stHeap.lpArrEvent[nRightChild];
			if(lpRight->m_nTimeout.tv_sec < lpLeft->m_nTimeout.tv_sec
				|| lpRight->m_nTimeout.tv_usec < lpLeft->m_nTimeout.tv_usec){
				nMinChild = nRightChild;
			}
		}
		Event* lpP = stHeap.lpArrEvent[nCurIndex];
		Event* lpQ = stHeap.lpArrEvent[nMinChild];
		if(lpP->m_nTimeout.tv_sec < lpQ->m_nTimeout.tv_sec
			|| lpP->m_nTimeout.tv_usec <= lpQ->m_nTimeout.tv_usec){
			break;
		}else{
			stHeap.lpArrEvent[nCurIndex] = lpP;
			stHeap.lpArrEvent[nMinChild] = lpQ;
			nCurIndex = nMinChild;
		}
	}
	return 0;
}
int32_t PopHeap(EventMinHeap& stHeap, int nIndex){
	if(nIndex < 0 || nIndex >= stHeap.nNum){
		return -1;
	}
	// 先将nIndex位置元素修改为键最小的
	stHeap.lpArrEvent[nIndex]->m_nTimeout = stHeap.lpArrEvent[0]->m_nTimeout;
	stHeap.lpArrEvent[nIndex]->m_nTimeout.tv_usec--;
	if(stHeap.lpArrEvent[nIndex]->m_nTimeout.tv_usec < 0){
		stHeap.lpArrEvent[nIndex]->m_nTimeout.tv_usec += 1000000;
		stHeap.lpArrEvent[nIndex]->m_nTimeout.tv_sec--;
	}
	// 再依次与父元素迭代，直到到达顶部
	int32_t nCurIndex = nIndex;
	while(nCurIndex != 0){
		// 交换
		int32_t nParentIndex = nCurIndex / 2;
		if(nCurIndex % 2 == 0){
			nParentIndex--;
		}

		Event* lpTmp = stHeap.lpArrEvent[nParentIndex];
		stHeap.lpArrEvent[nParentIndex] = stHeap.lpArrEvent[nCurIndex];
		stHeap.lpArrEvent[nCurIndex] = lpTmp;
	}
	// 再移除顶部元素
	return PopHeap(stHeap);
}
Event* PeekHeap(EventMinHeap& stHeap){
	if(stHeap.nNum == 0){
		return nullptr;
	}else{
		return stHeap.lpArrEvent[0];
	}
}
int32_t FindHeap(EventMinHeap& stHeap, Event* lpEvent){
	for(int32_t i = 0; i < stHeap.nNum; i++){
		if(stHeap.lpArrEvent[i] == lpEvent){
			return i;
		}
	}
	return -1;
}
void FreeHeap(EventMinHeap& stHeap){
	if(stHeap.lpArrEvent){
		free(stHeap.lpArrEvent);
		stHeap.lpArrEvent = nullptr;
		stHeap.nNum = 0;
		stHeap.nCapacity = 0;
	}
}

// 利用event_base实现事件循环．事件循环构成网络库工作线程核心逻辑
EventBase::EventBase(){
}

EventBase::~EventBase(){
    
}

int32_t EventBase::Init(){
	evsel = new Epoll(this);
	if(evsel == nullptr){
		return -1;
	}
	int32_t nRet = evsel->Init();
	if(nRet < 0){
		UnInit(1);
		return nRet;		
	}
	nRet = MakeNotifiable();
	if(nRet < 0){
		UnInit(1);
		return nRet;
	}
	return 0;
}

int EventBase::MakeNotifiable() {
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	void (*cb)(int32_t, short, void *);
	int (*notify)(EventBase *);
	if (th_notify_fn != NULL) {
		return 0;
	}
	if (evutil_make_internal_pipe_(th_notify_fd) == 0) {
		event_debug("notify pipe ok,fd[0] is %d,fd[1] is %d", th_notify_fd[0], th_notify_fd[1]);
		notify = Notify;
		cb = NotifyDrain;
	} else {
		return -1;
	}
	th_notify_fn = notify;
	// 服务于及时唤醒的event的初始化
	th_notify.Init(this, th_notify_fd[0], EV_READ|EV_PERSIST, cb, this);
	th_notify.m_stCallback.m_nFlags |= EVLIST_INTERNAL;
	th_notify.m_stCallback.m_nPri = 0;
	EventAdd(&th_notify, NULL, 0);
	th_base_lock.unlock();
	return 0;
}

void NotifyDrain(int32_t fd, short what, void *arg)
{
	EventBase* lpBase = (EventBase*)arg;
	unsigned char buf[1024];
	EventBase *base = (EventBase*)arg;
	int32_t ret = 0;
	while (ret = read(fd, (char*)buf, sizeof(buf)) > 0){
		event_debug("notify read to %d ret %d", fd, ret);
	}
	base->th_base_lock.lock();
	base->is_notify_pending = 0;
	base->th_base_lock.unlock();
}

int Notify(EventBase* base)
{
	char buf[1];
	int r;
	buf[0] = (char) 0;
	r = write(base->th_notify_fd[1], buf, 1);
	event_debug("notify write to %d ret %d", base->th_notify_fd[1], r);
	return (r < 0 && ! EVUTIL_ERR_IS_EAGAIN(errno)) ? -1 : 0;
}

void EventBase::UnInit(int run_finalizers){
	// 此时事件循环预期已经停止
	size_t n_deleted=0;
	if (th_notify_fd[0] != -1) {
		evutil_closesocket(th_notify_fd[0]);
		if (th_notify_fd[1] != -1)
			evutil_closesocket(th_notify_fd[1]);
		th_notify_fd[0] = -1;
		th_notify_fd[1] = -1;
	}
	if(run_finalizers){
		for(int32_t i = 0; i < 2; i++){
			std::list<EventCallback*>& stQueue = activequeues[i];
			while(stQueue.empty() == false){
				auto& ele = stQueue.front();
				if ((ele->m_nFlags & EVLIST_FINALIZING)) {
					switch (ele->m_nClosure) {
					case EV_CLOSURE_EVENT_FINALIZE:
					case EV_CLOSURE_EVENT_FINALIZE_FREE: {
						ele->m_stCbUnion.evcb_evfinalize(ele->m_lpOwner, ele->m_lpArg);
						if (ele->m_nClosure == EV_CLOSURE_EVENT_FINALIZE_FREE)
							free(ele->m_lpOwner);
						break;
					}
					case EV_CLOSURE_CB_FINALIZE:
						ele->m_stCbUnion.evcb_cbfinalize(ele, ele->m_lpArg);
						break;
					default:
						break;
					}
				}
			}
		}

		std::list<EventCallback*>& stQueue = active_later_queue;
		while(stQueue.empty() == false){
			auto& ele = stQueue.front();
			if ((ele->m_nFlags & EVLIST_FINALIZING)) {
				switch (ele->m_nClosure) {
				case EV_CLOSURE_EVENT_FINALIZE:
				case EV_CLOSURE_EVENT_FINALIZE_FREE: {
					ele->m_stCbUnion.evcb_evfinalize(ele->m_lpOwner, ele->m_lpArg);
					if (ele->m_nClosure == EV_CLOSURE_EVENT_FINALIZE_FREE)
						free(ele->m_lpOwner);
					break;
				}
				case EV_CLOSURE_CB_FINALIZE:
					ele->m_stCbUnion.evcb_cbfinalize(ele, ele->m_lpArg);
					break;
				default:
					break;
				}
			}
		}
	}
	for (auto it = once_events.begin(); it != once_events.end(); ) {  
        Event* lpEvent = *it;  
        it = once_events.erase(it);  
		free(lpEvent);
    }
	evsel->UnInit();
}

int EventBase::EventAdd(Event* lpEvent, const struct timeval *tv, bool bAbs)
{
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	event_debug("event_add: event: %p (fd %d), %s%s%s%scall %p", 
		lpEvent, EV_SOCK_ARG(lpEvent->m_nFd), lpEvent->m_nEvents & EV_READ ? "EV_READ " : " ", 
		lpEvent->m_nEvents & EV_WRITE ? "EV_WRITE " : " ", 
		lpEvent->m_nEvents & EV_CLOSED ? "EV_CLOSED " : " ", 
		tv ? "EV_TIMEOUT " : " ", lpEvent->m_stCallback.m_stCbUnion.evcb_callback);
	// 已经处于异步释放的event禁止后续操作
	if (lpEvent->m_stCallback.m_nFlags & EVLIST_FINALIZING) {// 对于已经移除等待异步通知的event。等待通知期间不应再次与base建立任何关联。通知触发时就是安全释放event时。
		return (-1);
	}

	bool bNotify = false;
	// 一个event对象只允许添加一次
	if(lpEvent->m_stCallback.m_nFlags & (EVLIST_TIMEOUT|EVLIST_INSERTED|EVLIST_ACTIVE|EVLIST_ACTIVE_LATER)){
		return (-1);
	}
	
	// 套接字event初次添加处理
	if ((lpEvent->m_nEvents & (EV_READ|EV_WRITE|EV_CLOSED))) 
	{
		auto iter = io.find(lpEvent->m_nFd);
		EventFdInfo* lpInfo = nullptr;
		if(iter == io.end()){
			lpInfo = new EventFdInfo();
			if(lpInfo == nullptr){
				return -1;
			}
			io.insert(std::pair<int32_t, EventFdInfo*>(lpEvent->m_nFd, lpInfo));
		}else{
			lpInfo = iter->second;
		}
		
		int nread = lpInfo->m_nRead, nwrite = lpInfo->m_nWrite, nclose = lpInfo->m_nClose, retval = 0;
		short res = 0;
		short old = 0;
		if (nread)
			old |= EV_READ;
		if (nwrite)
			old |= EV_WRITE;
		if (nclose)
			old |= EV_CLOSED;
		if (lpEvent->m_nEvents & EV_READ) {
			nread++;
			if(nread == 1){
				res |= EV_READ;
			}
		}
		if (lpEvent->m_nEvents & EV_WRITE) {
			nwrite++;
			if(nwrite == 1){
				res |= EV_WRITE;
			}
		}
		if (lpEvent->m_nEvents & EV_CLOSED) {
			nclose++;
			if(nclose == 1){
				res |= EV_CLOSED;
			}
		}

		auto ele = lpInfo->m_stEvents.back();
		if(ele && (ele->m_nEvents&EV_ET) != (lpEvent->m_nEvents&EV_ET)){
			event_warnx("Tried to mix edge-triggered and non-edge-triggered" " events on fd %d", (int)lpEvent->m_nFd);
			return -1;
		}
		
		if (res) {// 存在从无到有的监控事件
			if (evsel->Add(lpEvent->m_nFd, old, (lpEvent->m_nEvents & EV_ET) | res, nullptr) == -1)
				return (-1);
			bNotify = true;
		}
		
		lpInfo->m_nRead = nread;
		lpInfo->m_nWrite = nwrite;
		lpInfo->m_nClose = nclose;
		lpInfo->m_stEvents.push_back(lpEvent);
		lpEvent->m_stCallback.m_nFlags |= EVLIST_INSERTED;
	}
	
	// 定时event初次添加处理
	if (tv != NULL) {// 依赖定时机制分发
		// 持久性定时需设置超期时间点，超时间隔
		// 一次性定时需设置超期时间点
		if(bAbs){
			lpEvent->m_nTimeout = *tv;
		}else{
			// 先获取并缓存当前实时时间点
			struct timespec ts;
			if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
				return -1;
			tv_cache.tv_sec = ts.tv_sec;
			tv_cache.tv_usec = ts.tv_nsec / 1000;
			lpEvent->m_nTimeout.tv_sec = tv_cache.tv_sec + tv->tv_sec;
			lpEvent->m_nTimeout.tv_usec = tv_cache.tv_usec + tv->tv_usec;
			if(lpEvent->m_nTimeout.tv_usec > 1000000){
				lpEvent->m_nTimeout.tv_sec++;
				lpEvent->m_nTimeout.tv_usec -= 1000000;
			}
		}

		// 持久性定时，需提前设置好event对象的间隔参数
		timeval stPoint = lpEvent->m_nTimeout;
		// 插入最小堆
		PushHeap(timeheap, lpEvent);
		Event* lpMin = PeekHeap(timeheap);
		if(lpMin == lpEvent){
			bNotify = true;
		}

		lpEvent->m_stCallback.m_nFlags |= EVLIST_TIMEOUT;
	}
	//event_debug("event_add: event_%p notify_%d", lpEvent, bNotify);
	if (bNotify && running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return 0;
		is_notify_pending = 1;
		return th_notify_fn(this);
	}
	return 0;
}

int EventBase::EventDel(Event *lpEvent, int32_t nBlockingFlag)
{
	std::unique_lock<std::recursive_mutex> lock(th_base_lock); 
	bool bNotify = false;
	event_debug("event_del: %p (fd %d), callback %p", 
		lpEvent, lpEvent->m_nFd, lpEvent->m_stCallback.m_stCbUnion.evcb_callback);
	// 对已经处于异步释放阶段的event，再次执行移除，无效果．
	if (lpEvent->m_stCallback.m_nFlags & EVLIST_FINALIZING) {
		event_debug("event_del when finalizing");
		return 0;
	}
	
	if (lpEvent->m_stCallback.m_nFlags & EVLIST_TIMEOUT) {
		int32_t nIndex = FindHeap(timeheap, lpEvent);
		PopHeap(timeheap, nIndex);
		lpEvent->m_stCallback.m_nFlags &= ~(EVLIST_TIMEOUT);
	}
	if (lpEvent->m_stCallback.m_nFlags & EVLIST_ACTIVE){
		lpEvent->m_stCallback.m_nFlags &= ~(EVLIST_ACTIVE);
		activequeues[lpEvent->m_stCallback.m_nPri].remove(&(lpEvent->m_stCallback));
	}
	else if (lpEvent->m_stCallback.m_nFlags & EVLIST_ACTIVE_LATER){
		lpEvent->m_stCallback.m_nFlags &= ~(EVLIST_ACTIVE_LATER);
		active_later_queue.remove(&(lpEvent->m_stCallback));
	}
		
	if (lpEvent->m_stCallback.m_nFlags & EVLIST_INSERTED) {
		lpEvent->m_stCallback.m_nFlags &= ~(EVLIST_INSERTED);
		auto iter = io.find(lpEvent->m_nFd);
		if(iter != io.end()){
			EventFdInfo* lpInfo = iter->second;
			lpInfo->m_stEvents.remove(lpEvent);
			// 需要从容器移除
			int nread, nwrite, nclose;
			short res = 0, old = 0;
			nread = lpInfo->m_nRead;
			nwrite = lpInfo->m_nWrite;
			nclose = lpInfo->m_nClose;
			if (nread)
				old |= EV_READ;
			if (nwrite)
				old |= EV_WRITE;
			if (nclose)
				old |= EV_CLOSED;
			if (lpEvent->m_nEvents & EV_READ) {
				if (--nread == 0)
					res |= EV_READ;
			}
			if (lpEvent->m_nEvents & EV_WRITE) {
				if (--nwrite == 0)
					res |= EV_WRITE;
			}
			if (lpEvent->m_nEvents & EV_CLOSED) {
				if (--nclose == 0)
					res |= EV_CLOSED;
			}
			if (res) {
				if (evsel->Del(lpEvent->m_nFd, old, (lpEvent->m_nEvents & EV_ET) | res, nullptr) == -1) {
					event_debug("evsel->Del Fail, errno %d", errno);
				} else {
					event_debug("evsel->Del Success");
					lpInfo->m_nRead = nread;
					lpInfo->m_nWrite = nwrite;
					lpInfo->m_nClose = nclose;
					lpInfo->m_stEvents.remove(lpEvent);
					bNotify = true;
				}
			}
		}
	}
	if (bNotify && running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return 0;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
	
	while (nBlockingFlag != EVENT_DEL_NOBLOCK && current_event == &(lpEvent->m_stCallback) && th_owner_id != std::this_thread::get_id()) {
		current_event_waiters++;
		lock.unlock();
		while(true){
			if(current_event != &(lpEvent->m_stCallback)){
				break;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(4));  
		}
		lock.lock();
	}
	return 0;
}

void EventBase::EventActive(Event* lpEvent, int res, short ncalls){
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	event_debug("event_active: %p (fd %d), res %d, callback %p, flags %d", 
		lpEvent, lpEvent->m_nFd, (int)res, &(lpEvent->m_stCallback), lpEvent->m_stCallback.m_nFlags);
	if (lpEvent->m_stCallback.m_nFlags & EVLIST_FINALIZING) {// 对于已经移除等待异步回调释放的event。此后不应被使用。自然也不应在被手动分发。
		event_debug("event_active: this event is finalizing");
		return;
	}
	switch (lpEvent->m_stCallback.m_nFlags & (EVLIST_ACTIVE|EVLIST_ACTIVE_LATER)) {
	default:
	case EVLIST_ACTIVE|EVLIST_ACTIVE_LATER:
		assert(0);
		break;
	case EVLIST_ACTIVE:
		lpEvent->m_nRes |= res;// ev_res记录了event得到分发的原因
		return;
	case EVLIST_ACTIVE_LATER:
		lpEvent->m_nRes |= res;
		active_later_queue.remove(&(lpEvent->m_stCallback));
		activequeues[lpEvent->m_stCallback.m_nPri].push_back(&(lpEvent->m_stCallback));
		break;
	case 0:
		lpEvent->m_nRes = res;
		activequeues[lpEvent->m_stCallback.m_nPri].push_back(&(lpEvent->m_stCallback));
		lpEvent->m_stCallback.m_nFlags |= EVLIST_ACTIVE;
		break;
	}
	if (lpEvent->m_stCallback.m_nPri < event_running_priority)
		event_continue = 1;// 这是更高优先级的event被分发后，快速结束本轮处理，快速进入下一轮。
	 if(running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
	return;
}
void EventBase::EventActiveLater(Event *lpEvent, int res){
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	event_debug("event_active_later: %p (fd %d), res %d, callback %p", 
		lpEvent, lpEvent->m_nFd, (int)res, &(lpEvent->m_stCallback));
	if (lpEvent->m_stCallback.m_nFlags & EVLIST_FINALIZING) {// 对于已经移除等待异步回调释放的event。此后不应被使用。自然也不应在被手动分发。
		return;
	}
	switch (lpEvent->m_stCallback.m_nFlags & (EVLIST_ACTIVE|EVLIST_ACTIVE_LATER)) {
	default:
	case EVLIST_ACTIVE|EVLIST_ACTIVE_LATER:
		assert(0);
		break;
	case EVLIST_ACTIVE:
		lpEvent->m_stCallback.m_nFlags |= res;// ev_res记录了event得到分发的原因
		return;
	case EVLIST_ACTIVE_LATER:
		lpEvent->m_stCallback.m_nFlags |= res;
		break;
	case 0:
		lpEvent->m_stCallback.m_nFlags = res;
		active_later_queue.push_back(&(lpEvent->m_stCallback));
		lpEvent->m_stCallback.m_nFlags |= EVLIST_ACTIVE_LATER;
		break;
	}
	if (lpEvent->m_stCallback.m_nPri < event_running_priority)
		event_continue = 1;// 这是更高优先级的event被分发后，快速结束本轮处理，快速进入下一轮。
		
	 if(running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
	return;
}

void EventBase::EventCallbackActive(EventCallback *evcb){
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	assert(evcb->m_lpOwner == nullptr);
	switch (evcb->m_nFlags & (EVLIST_ACTIVE|EVLIST_ACTIVE_LATER)) {
	default:
	case EVLIST_ACTIVE|EVLIST_ACTIVE_LATER:
		assert(0);
		break;
	case EVLIST_ACTIVE:
		return;
	case EVLIST_ACTIVE_LATER:
		active_later_queue.remove(evcb);
		activequeues[evcb->m_nPri].push_back(evcb);
		evcb->m_nFlags &= ~(EVLIST_ACTIVE_LATER);
		evcb->m_nFlags |= EVLIST_ACTIVE;
		break;
	case 0:
		activequeues[evcb->m_nPri].push_back(evcb);
		evcb->m_nFlags |= EVLIST_ACTIVE;
		break;
	}
	if (evcb->m_nPri < event_running_priority)
		event_continue = 1;// 这是更高优先级的event被分发后，快速结束本轮处理，快速进入下一轮。
		
	 if(running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
}

void EventBase::EventCallbackActiveLater(EventCallback *evcb){
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	assert(evcb->m_lpOwner == nullptr);
	switch (evcb->m_nFlags & (EVLIST_ACTIVE|EVLIST_ACTIVE_LATER)) {
	default:
	case EVLIST_ACTIVE|EVLIST_ACTIVE_LATER:
		assert(0);
		break;
	case EVLIST_ACTIVE:
		return;
	case EVLIST_ACTIVE_LATER:
		break;
	case 0:
		active_later_queue.push_back(evcb);
		evcb->m_nFlags |= EVLIST_ACTIVE_LATER;
		break;
	}
	if (evcb->m_nPri < event_running_priority)
		event_continue = 1;// 这是更高优先级的event被分发后，快速结束本轮处理，快速进入下一轮。
		
	 if(running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
}

void EventBase::LoopBreak()
{
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	event_break = 1;
	if(running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
}
// 4.事件循环控制
void EventBase::LoopContinue()
{
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	event_continue = 1;
	if(running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
}
void EventBase::LoopTerm(){
	std::lock_guard<std::recursive_mutex> lock(th_base_lock);
	event_gotterm = 1;
	if(running_loop && th_owner_id != std::this_thread::get_id()){
		//event_debug("event_add: notify happen");
		if (is_notify_pending)
			return;
		is_notify_pending = 1;
		th_notify_fn(this);
	}
}

int EventBase::Loop(int flags){
	struct timeval tv;
	struct timeval *tv_p;
	int res, done, retval = 0;
	th_base_lock.lock();
	if (running_loop) {
		event_warnx("%s: reentrant invocation.  Only one event_base_loop can run on each event_base at once.", __func__);
		th_base_lock.unlock();
		return -1;
	}
	running_loop = 1;
	tv_cache.tv_sec = 0;
	tv_cache.tv_usec = 0;
	
	done = 0;
	th_owner_id = std::this_thread::get_id();  
	event_gotterm = event_break = 0;
	while (!done) {
		//event_warnx("loop again", __func__);
		event_continue = 0;
		if (event_gotterm) {
			break;
		}
		if (event_break) {
			break;
		}
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		// 先获取并缓存当前实时时间点
		struct timespec ts;
		if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
			return -1;
		tv_cache.tv_sec = ts.tv_sec;
		tv_cache.tv_usec = ts.tv_nsec / 1000;
		if (activequeues[0].empty() && activequeues[1].empty() && active_later_queue.empty()) {
			Event* lpev = PeekHeap(timeheap);
			if(lpev && (lpev->m_nTimeout.tv_sec > tv_cache.tv_sec || lpev->m_nTimeout.tv_usec > tv_cache.tv_usec)){
				tv.tv_sec = lpev->m_nTimeout.tv_sec - tv_cache.tv_sec;
				tv.tv_usec = lpev->m_nTimeout.tv_usec - tv_cache.tv_usec;
				if(tv.tv_usec < 0){
					tv.tv_sec--;
					tv.tv_usec += 1000000;
				}
			}			
		} else {
			tv.tv_sec = 0;
			tv.tv_usec = 0;
		}

		// 将later队列内每个节点移除，加入相应active队列
		while(active_later_queue.empty() == false){
			auto ele = active_later_queue.front();
			ele->m_nFlags = (ele->m_nFlags & ~EVLIST_ACTIVE_LATER);
			ele->m_nFlags |= EVLIST_ACTIVE;
			activequeues[ele->m_nPri].push_back(ele);
			active_later_queue.pop_front();
		}
		
		res = evsel->Dispatch(&tv);// 借助io复用器等待事件&分发事件
		if (res == -1) {
			event_debug("%s: dispatch returned unsuccessfully.", __func__);
			retval = -1;
			goto done;
		}

		if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
			return -1;
		tv_cache.tv_sec = ts.tv_sec;
		tv_cache.tv_usec = ts.tv_nsec / 1000;
		while(true){
			auto ele = PeekHeap(timeheap);
			if(ele == nullptr ||
				(ele->m_nTimeout.tv_sec > tv_cache.tv_sec || ele->m_nTimeout.tv_usec > tv_cache.tv_usec)){
				break;
			}
			EventDel(ele);
			event_debug("timeout_process: event: %p, call %p", ele, ele->m_stCallback.m_stCbUnion.evcb_callback);
			EventActive(ele, EV_TIMEOUT, 1);
		}

		if (activequeues[0].empty() == false || activequeues[1].empty() == false) {
			ProcessActive();
		}
	}
	event_debug("%s: asked to terminate loop.", __func__);
done:
	tv_cache.tv_sec = 0;
	tv_cache.tv_usec = 0;
	running_loop = 0;
	th_base_lock.unlock();
	return 0;
}

void EventBase::ProcessActive(){
	event_continue = 0;
	for(int32_t i = 0; i < 2; i++){
		std::list<EventCallback*>& stList = activequeues[i];
		if(stList.empty()){
			continue;
		}

		while(stList.empty() == false){
			auto ele = stList.front();
			if (ele->m_nFlags & EVLIST_INIT) {
				if (ele->m_lpOwner->m_nEvents & EV_PERSIST || ele->m_nFlags & EVLIST_FINALIZING){
					stList.pop_front();
					ele->m_nFlags &= ~(EVLIST_ACTIVE);
				}
				else{
					EventDel(ele->m_lpOwner);
				}
				event_debug("event_process_active: event: %p, %s%s%scall %p", 
					ele->m_lpOwner, ele->m_lpOwner->m_nRes & EV_READ ? "EV_READ " : " ", 
					ele->m_lpOwner->m_nRes & EV_WRITE ? "EV_WRITE " : " ", 
					ele->m_lpOwner->m_nRes & EV_CLOSED ? "EV_CLOSED " : " ", 
					ele->m_stCbUnion.evcb_callback);
			} else {
				stList.pop_front();
				event_debug("event_process_active: event_callback %p, closure %d, call %p", 
					ele, ele->m_nClosure, ele->m_stCbUnion.evcb_callback);
			}
			
			current_event = ele;
			current_event_waiters = 0;
			event_running_priority = ele->m_nPri;
			switch (ele->m_nClosure) {
			case EV_CLOSURE_EVENT_PERSIST:
			{
				void (*evcb_callback)(int32_t, short, void *);
				int32_t evcb_fd;
				short evcb_res;
				void *evcb_arg;
				// 持久性定时event
				if (ele->m_lpOwner->m_nTimeoutInterval.tv_sec || ele->m_lpOwner->m_nTimeoutInterval.tv_usec) {
					struct timeval run_at, relative_to, delay, now;
					struct timespec ts;
					clock_gettime(CLOCK_MONOTONIC, &ts);
					tv_cache.tv_sec = ts.tv_sec;
					tv_cache.tv_usec = ts.tv_nsec / 1000;
					delay = ele->m_lpOwner->m_nTimeoutInterval;
					if (ele->m_lpOwner->m_nRes & EV_TIMEOUT) {// 被分发的原因是超时
						relative_to = ele->m_lpOwner->m_nTimeout;// 基于超时时间点重新计算下次的超时时间点
					} else {
						relative_to = tv_cache;// 并非因超时而被分发的．基于当前重新计算下次超时时间点．
					}
					run_at.tv_sec = relative_to.tv_sec + ele->m_lpOwner->m_nTimeoutInterval.tv_sec;
					run_at.tv_usec = relative_to.tv_usec + ele->m_lpOwner->m_nTimeoutInterval.tv_usec;
					if(run_at.tv_usec > 1000000){
						run_at.tv_sec++;
						run_at.tv_usec -= 1000000;
					}
					// 需重新添加．先移除，再添加
					EventDel(ele->m_lpOwner);
					EventAdd(ele->m_lpOwner, &run_at, 1);
				}
				else{
					// 仅仅服务于套接字的持久event
					// event分发时，处理前均不会移除．也无需再次添加．
				}
				evcb_callback = ele->m_stCbUnion.evcb_callback;
				evcb_fd = ele->m_lpOwner->m_nFd;
				evcb_res = ele->m_lpOwner->m_nRes;
				evcb_arg = ele->m_lpArg;
				th_base_lock.unlock();
				(evcb_callback)(evcb_fd, evcb_res, evcb_arg);
			}
				break;
			case EV_CLOSURE_EVENT: {
				// 非持久event
				// 无论是作为套接字，定时，还是复合．分发前必然移除．
				void (*evcb_callback)(int32_t, short, void *);
				short res;
				evcb_callback = ele->m_stCbUnion.evcb_callback;
				res = ele->m_lpOwner->m_nRes;
				th_base_lock.unlock();
				evcb_callback(ele->m_lpOwner->m_nFd, res, ele->m_lpArg);
			}
			break;
			case EV_CLOSURE_CB_SELF: {
				// 分发处理前会移除
				void (*evcb_selfcb)(EventCallback *, void *) = ele->m_stCbUnion.evcb_selfcb;
				th_base_lock.unlock();
				evcb_selfcb(ele, ele->m_lpArg);
			}
			break;
			case EV_CLOSURE_EVENT_FINALIZE:
			case EV_CLOSURE_EVENT_FINALIZE_FREE: {
				// 此类event，在被分发时event已经移除了．				
				void (*evcb_evfinalize)(Event *, void *);
				int evcb_closure = ele->m_nClosure;
				current_event = NULL;// 这样的event_callback关联的event已经没了
				evcb_evfinalize = ele->m_stCbUnion.evcb_evfinalize;
				th_base_lock.unlock();
				evcb_evfinalize(ele->m_lpOwner, ele->m_lpArg);// 这个回调提供给外部一个时间点告知此关联的event此时可以安全释放了。外部可完成event关联资源释放。
				if (evcb_closure == EV_CLOSURE_EVENT_FINALIZE_FREE)
					free(ele->m_lpOwner);// 内部完成event释放。
			}
			break;
			case EV_CLOSURE_CB_FINALIZE: {
				// 此类event_callback不存在依附的event．被分发时，event_callback直接进入激活队列．
				void (*evcb_cbfinalize)(EventCallback *, void *) = ele->m_stCbUnion.evcb_cbfinalize;
				current_event = NULL;
				th_base_lock.unlock();
				evcb_cbfinalize(ele, ele->m_lpArg);
			}
			break;
			default:
				assert(0);
			}
			th_base_lock.lock();
			current_event = NULL;
			event_running_priority = -1;
			if (event_break || event_continue || event_gotterm)
				return;
		}
	}
}

void EventBase::IoActive(int32_t fd, short events) {
	auto iter = io.find(fd);
	if(iter != io.end()){
		auto lists = iter->second->m_stEvents;
		for(auto it = lists.begin(); it != lists.end(); it++){
			if((*it)->m_nEvents & (events & ~EV_ET)){
				EventActive(*it, (*it)->m_nEvents & events, 1);
			}
		}
	}
}
}