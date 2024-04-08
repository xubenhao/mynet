#include "listen.h"
#include "log.h"
#include "util.h"
#include "event_base.h"
#include "server.h"
namespace mynet{
void ListenReadProcess(int32_t fd, short events, void* arg){
    Listen* lpListen = (Listen*)arg;
    lpListen->ReadEventProcess(fd, events);
}

void Listen::ReadEventProcess(int32_t nFd, short nEvents){
	int err;
	void *user_data;
	while (1) {
		struct sockaddr_storage ss;
		socklen_t socklen = sizeof(ss);
		int32_t accept4_flags = 0;
		accept4_flags |= SOCK_NONBLOCK;
		accept4_flags |= SOCK_CLOEXEC;
		int32_t new_fd = evutil_accept4_(nFd, (struct sockaddr*)&ss, &socklen, accept4_flags);
		if (new_fd < 0)
			break;
		if (socklen == 0) {
			evutil_closesocket(new_fd);
			continue;
		}
		
		int32_t nThreadIndex = m_lpOwner->GetNextThreadIndex();
		EventBase* lpBase = m_lpOwner->GetBase(nThreadIndex);
		// 这里需要产生被动连接．注册事件监控．触发上层回调．
		ServConn* lpConn = m_lpOwner->GetConnect();
		int64_t nId = m_lpOwner->GetNextId();
		m_lpOwner->RegisterServConn(nId, lpConn);
		event_debug("accept ret %d, thread index %d, base %p, conn %p", 
			new_fd, nThreadIndex, lpBase, lpConn);
		lpConn->Init(nId, new_fd, m_lpOwner->GetServerCallback(), m_lpBase);
	}
	err = evutil_socket_geterror(fd);
	if (EVUTIL_ERR_ACCEPT_RETRIABLE(err)) {
		return;
	}else{
		// 错误处理
		m_lpBase->EventDel(&m_stListen);
		close(m_nFd);
		return;
	}
}
Listen::Listen(){

}
Listen::~Listen(){
}
int32_t Listen::Init(Server* lpServer, EventBase* lpBase, int32_t nPort){
    m_lpOwner = lpServer;
    m_nPort = nPort;
    m_lpBase = lpBase;
    return 0;
}
void Listen::UnInit(){
}
int32_t Listen::Start(){
	int32_t fd;
    struct sockaddr_storage ss;
	struct sockaddr *sa = (struct sockaddr *)&ss;
    int32_t ss_len;
    struct addrinfo *ai = NULL;

	int ai_result;
    struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
    char strport[100] = {'\0'};
	snprintf(strport, sizeof(strport), "%d", m_nPort);
	if ((ai_result = getaddrinfo("0.0.0.0", strport, &hints, &ai)) != 0) {
        event_errx("fun %s:getaddrinfo fail errno %d", __func__, errno);
		return -1;
	}
	if (!ai){
        event_errx("fun %s:getaddrinfo fail errno %d", __func__, errno);
        return -1;
    }
		
	if (ai->ai_addrlen > sizeof(ss)) {
        event_errx("fun %s:getaddrinfo fail errno %d", __func__, errno);
		freeaddrinfo(ai);
		return -1;
	}
	memcpy(&ss, ai->ai_addr, ai->ai_addrlen);
	ss_len = ai->ai_addrlen;
	freeaddrinfo(ai);
    fd = socket(ss.ss_family, SOCK_STREAM|SOCK_NONBLOCK, 0);
	if (fd == -1){
        event_errx("fun %s:socket fail errno %d", __func__, errno);
		return -1;
    }

	int32_t on = 1;
    //if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&on, sizeof(on))<0){
    //    event_errx("fun %s:setsockopt fail errno %d", __func__, errno);
    //    evutil_closesocket(fd);
	//	return -1;
    //}
    if (evutil_make_listen_socket_reuseable(fd) < 0){
        event_errx("fun %s:socket reuseable fail errno %d", __func__, errno);
        evutil_closesocket(fd);
		return -1;
    }
	
    if (bind(fd, sa, ss_len) < 0){
        event_errx("fun %s:bind fail errno %d", __func__, errno);
        evutil_closesocket(fd);
		return -1;
    }

	listen(fd, 128);
    m_stListen.Init(m_lpBase, fd, EV_READ|EV_PERSIST, ListenReadProcess, this);
    m_lpBase->EventAdd(&m_stListen);
    m_nFd = fd;
    return 0;
}
int32_t Listen::Stop(){
    evutil_closesocket(m_nFd);
    m_lpBase->EventDel(&m_stListen);
    return 0;
}
}
