#include "connect.h"
#include "log.h"
#include "util.h"
namespace mynet{
void ConnReadProcess(int32_t fd, short events, void* arg){
    Connect* lpConn = (Connect*)arg;
    lpConn->ReadEventProcess(fd, events);
}
void ConnWriteProcess(int32_t fd, short events, void* arg){
    Connect* lpConn = (Connect*)arg;
    lpConn->WriteEventProcess(fd, events);
}
void ConnDisconnProcess(EventCallback* lpCall, void* arg){
	Connect* lpConn = (Connect*)arg;
	lpConn->DisconnProcess(lpCall);
}
Connect::Connect(){

}
Connect::~Connect(){
	m_stSendMutex.lock();
	m_stOutput.Reset();
	m_stSendMutex.unlock();
	if(m_lpRecvBuff){
		free(m_lpRecvBuff);
		m_nRecvOffset = 0;
		m_nRecvOffset = 0;
		m_nRecvCapacity = 0;
		m_lpRecvBuff = nullptr;
	}
}
// 作为客户端初始化
int32_t Connect::Init(IClientCallback* lpCallback, EventBase* lpBase, int32_t nConnTimeout){
    m_nConnTimeout = nConnTimeout;
    m_lpCallback = lpCallback;
    m_lpBase = lpBase;
    m_nFd = -1;
    m_lpRecvBuff = nullptr;
    m_nRecvCapacity = 0;
	m_nRecvOffset = 0;
	m_nRecvLength = 0;
	m_nConnStatus = CONN_STATUS_CLOSED;
    return 0;
}
// 作为服务端被动连接的初始化
int32_t Connect::Init(int32_t nFd, IClientCallback* lpCallback, EventBase* lpBase){
    m_lpCallback = lpCallback;
    m_lpBase = lpBase;
    m_nFd = nFd;
    m_lpRecvBuff = nullptr;
    m_nRecvCapacity = 0;
	m_nRecvOffset = 0;
	m_nRecvLength = 0;
	m_nConnStatus = CONN_STATUS_CONNECTED;
	m_stRead.Init(m_lpBase, m_nFd, EV_READ|EV_PERSIST, ConnReadProcess, this);
	m_stWrite.Init(m_lpBase, m_nFd, EV_WRITE|EV_PERSIST, ConnWriteProcess, this);
    m_stWriteForConn.Init(m_lpBase, m_nFd, EV_WRITE|EV_PERSIST, ConnWriteProcess, this);
	m_lpBase->EventAdd(&m_stRead);
	m_lpCallback->OnEvent(CONNECT_EVENT_CONN_SUCC);
    return 0;
}
int32_t Connect::UnInit(){
    DoDisconnect(true, true);
    return 0;
}

int32_t Connect::DoConnect(int32_t nPort, char (&strIp)[100], int32_t nConnTimeout,
	bool bBlock, bool bSyn){
	// 先解决过程进入
	if(bBlock){
		m_stConnMutex.lock();
	}
	else{
		bool bRet = m_stConnMutex.try_lock();
		if(bRet == false){
			return RET_CODE_CONN_MTX_GET_FAIL;
		}
	}

	// 连接锁会保证至多同时只有一个连接建立过程，连接断开过程存在．
	if(m_nConnStatus == CONN_STATUS_CONNECTED){
		m_stConnMutex.unlock();
		return RET_CODE_CONN_ALREADY;
	}
	m_nConnStatus = CONN_STATUS_CONNECTING;
    // 创建套接字
    struct sockaddr_storage ss;
	int32_t ss_len;
    struct addrinfo *ai = NULL;
	struct addrinfo hints;
	char strport[NI_MAXSERV] = {'\0'};
	int ai_result;
	int32_t ret = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	snprintf(strport, sizeof(strport), "%d", nPort);
	if ((ai_result = getaddrinfo(strIp, strport, &hints, &ai)) != 0) {
        event_errx("fun %s,line %d,getaddrinfo fail ret %d errno %d", __func__, __LINE__, ai_result, errno);
		m_nConnStatus = CONN_STATUS_CLOSED;
		m_stConnMutex.unlock();
		return RET_CODE_GET_ADDR_INFO_FAIL;
	}
	if (!ai){
		m_nConnStatus = CONN_STATUS_CLOSED;
		m_stConnMutex.unlock();
		return RET_CODE_GET_ADDR_INFO_FAIL;
	}
	if (ai->ai_addrlen > sizeof(ss)) {
		freeaddrinfo(ai);
		m_nConnStatus = CONN_STATUS_CLOSED;
		m_stConnMutex.unlock();
		return RET_CODE_GET_ADDR_INFO_FAIL;
	}
	memcpy(&ss, ai->ai_addr, ai->ai_addrlen);
	ss_len = (int32_t)ai->ai_addrlen;
	freeaddrinfo(ai);

    int32_t fd = socket(ss.ss_family, SOCK_STREAM|SOCK_NONBLOCK, 0);
	if (fd < 0){
        event_errx("fun %s:socket fail errno %d", __func__, errno);
		m_nConnStatus = CONN_STATUS_CLOSED;
        m_stConnMutex.unlock();
		return RET_CODE_SOCKET_FAIL;
    }

    // 发出到目标地址的连接请求
    int32_t r = evutil_socket_connect_(&fd, (const sockaddr*)&ss, ss_len);
	if (r < 0){
        evutil_closesocket(fd);
		m_nConnStatus = CONN_STATUS_CLOSED;
        m_stConnMutex.unlock();
		return RET_CODE_CONN_FAIL;
    }
    // 非-1时，会在异步事件处理中执行连接成功，连接失败处理
    m_nFd = fd;
    m_stConnTimeout.tv_sec = m_nConnTimeout;
    m_stConnTimeout.tv_usec = 0;

	// event对象应该初始化．每次添加或移除前不能再次初始化．
	m_stRead.Init(m_lpBase, m_nFd, EV_READ|EV_PERSIST, ConnReadProcess, this);
	m_stWrite.Init(m_lpBase, m_nFd, EV_WRITE|EV_PERSIST, ConnWriteProcess, this);
    m_stWriteForConn.Init(m_lpBase, m_nFd, EV_WRITE|EV_PERSIST, ConnWriteProcess, this);
    // 需向关联base注册可写事件
    m_lpBase->EventAdd(&m_stWriteForConn, &m_stConnTimeout, false);
    if(bSyn){
		// 同步下，发出连接请求后，需同步等待到连接完成或失败
		while(true){
			if(m_nConnStatus == CONN_STATUS_CLOSED){
				return RET_CODE_CONN_FAIL;
			}
			else if(m_nConnStatus == CONN_STATUS_CONNECTED){
				return RET_CODE_CONN_SUCC;	
			}

			std::this_thread::sleep_for(std::chrono::microseconds(10));  
		}
	}else{
		// 异步下，发出连接请求后，借助异步回调实现结果通知
		return RET_CODE_CONNING;
	}
}

void Connect::WriteEventProcess(int32_t nFd, short nEvents){
	int32_t res = 0;
	short what = BEV_EVENT_WRITING;
	int32_t atmost = -1;
	if (nEvents == EV_TIMEOUT) {// 只有连接阶段，可写事件会包含超时监控．
		// 连接超时下，关闭连接
		m_lpBase->EventDel(&m_stWriteForConn);
		m_nConnStatus = CONN_STATUS_CLOSED;
		m_stConnMutex.unlock();
		m_lpCallback->OnEvent(CONNECT_EVENT_CONN_TIMEOUT);
		return;
	}

	// 连接过程收到可写事件．此时需处理连接成功，连接失败．
	if (m_nConnStatus == CONN_STATUS_CONNECTING) {
		int32_t c = evutil_socket_finished_connecting_(nFd);
		if (c == 0){
			return;// 继续等待
		}
		if (c < 0) {
			m_lpBase->EventDel(&m_stWriteForConn);
			m_nConnStatus = CONN_STATUS_CLOSED;
			m_stConnMutex.unlock();
			m_lpCallback->OnEvent(CONNECT_EVENT_CONN_FAIL);
			return;
		} else {
			m_nConnStatus = CONN_STATUS_CONNECTED;
    		m_lpBase->EventAdd(&m_stRead);
			m_lpBase->EventDel(&m_stWriteForConn);
			m_stConnMutex.unlock();
			m_lpCallback->OnEvent(CONNECT_EVENT_CONN_SUCC);
			return;
		}
	}
	
	// 非连接过程，收到可写事件，可视为是执行异步发送的时机
	m_stSendMutex.lock();
	while(true){
		char* lpBuf = m_stOutput.GetBuff();
		int32_t nLen = m_stOutput.GetLength();
		if(lpBuf == nullptr){
			m_lpBase->EventDel(&m_stWrite);// 一旦异步发送中缓存内容发送完毕，需移除可写监控
			m_stSendMutex.unlock();
			return;
		}
		ssize_t n = write(m_nFd, lpBuf, nLen);
		if (res == -1) {
			m_stOutput.Reset();
			m_stSendMutex.unlock();
			int err = evutil_socket_geterror(fd);
			if (EVUTIL_ERR_RW_RETRIABLE(err))
			{// 表示遭遇EAGAIN，此时可以结束事件处理
				return;
			}
			// 发送过程遭遇异步错误．此时需内部断开连接．
			event_errx("fun %s:write fail errno %d", __func__, errno);
			m_lpBase->EventDel(&m_stRead);
			m_lpBase->EventDel(&m_stWrite);
			if(m_lpRecvBuff){
				free(m_lpRecvBuff);
				m_nRecvOffset = 0;
				m_nRecvOffset = 0;
				m_nRecvCapacity = 0;
				m_lpRecvBuff = nullptr;
			}
			
			m_nConnStatus = CONN_STATUS_CLOSED;
			m_lpCallback->OnEvent(CONNECT_EVENT_CONN_SEND_ERR);
			return;
		}
		else{
			m_stOutput.Drain(n);
			event_msgx("fun %s line %d outbuf drain %d", __func__, __LINE__, n);
		}
	}
	m_stSendMutex.unlock();
}

// 只在连接成功建立后，注册可读
void Connect::ReadEventProcess(int32_t nFd, short nEvents){
	int32_t howmuch = 4 * 1024;// 控制单独最多读入4k
    // 从套接字读入新数据，新数据读入接收缓存区，计算缓存区剩余空间
	if(m_lpRecvBuff == nullptr){
		m_lpRecvBuff = (char*)malloc(4*1024);
		m_nRecvCapacity = 4*1024;
		m_nRecvOffset = 0;
		m_nRecvLength = 0;
	}
    int32_t nLeft = m_nRecvCapacity - m_nRecvOffset - m_nRecvLength;
	ssize_t n = read(nFd, m_lpRecvBuff+m_nRecvOffset+m_nRecvLength, nLeft);
	ssize_t res = 0;
	if (n == -1) {
		int err = evutil_socket_geterror(nFd);
		if (EVUTIL_ERR_RW_RETRIABLE(err))
			return;// 正常结束
		// 接收过程遭遇异步错误．此时需内部断开连接．
		event_errx("fun %s:recv fail errno %d", __func__, err);
		m_lpBase->EventDel(&m_stRead);
		m_lpBase->EventDel(&m_stWrite);
		m_nConnStatus = CONN_STATUS_CLOSED;
		close(nFd);
		m_stSendMutex.lock();
		m_stOutput.Reset();
		m_stSendMutex.unlock();
		if(m_lpRecvBuff){
			free(m_lpRecvBuff);
			m_nRecvOffset = 0;
			m_nRecvOffset = 0;
			m_nRecvCapacity = 0;
			m_lpRecvBuff = nullptr;
		}
		m_lpCallback->OnEvent(CONNECT_EVENT_CONN_RECV_ERR);
		return;
	} 
	else if (n == 0) {
		// 接收过程遭遇EOF
		event_errx("the remote has close, now we close");
		m_lpBase->EventDel(&m_stRead);
		m_lpBase->EventDel(&m_stWrite);
		m_stSendMutex.lock();
		m_stOutput.Reset();
		m_stSendMutex.unlock();
		if(m_lpRecvBuff){
			free(m_lpRecvBuff);
			m_nRecvOffset = 0;
			m_nRecvOffset = 0;
			m_nRecvCapacity = 0;
			m_lpRecvBuff = nullptr;
		}
		m_nConnStatus = CONN_STATUS_CLOSED;
		close(nFd);
		m_lpCallback->OnEvent(CONNECT_EVENT_CONN_RECV_EOF);
		return;
	}else{
		m_nRecvLength += n;
		RecvDataProcess(nFd);
	}
}
void Connect::RecvDataProcess(int32_t nFd){
	while(true){
		if(m_nRecvLength >= 4){
			m_nNextPackLen = *(uint32_t*)(m_lpRecvBuff+m_nRecvOffset);
			m_nNextPackLen = be32toh(m_nNextPackLen);
			if(m_nRecvLength >= m_nNextPackLen){
				m_lpCallback->OnMessage(m_lpRecvBuff+m_nRecvOffset, m_nNextPackLen);
				m_nRecvOffset += m_nNextPackLen;
				m_nRecvLength -= m_nNextPackLen;
			}
			else{
				// 继续累计
				// 不足一个包部分需位于缓存起始部分，缓存必须足够容纳此包
				if(m_nRecvCapacity < m_nNextPackLen){
					int32_t nNewCap = m_nNextPackLen / (4*1024);
					if(m_nNextPackLen % (4*1024) != 0){
						nNewCap++;
					}
					nNewCap = nNewCap * 4 * 1024;
					char* lpNewBuf = (char*)malloc(nNewCap);
					if(lpNewBuf == nullptr){
						event_errx("recv data malloc %d fail", nNewCap);
						m_lpBase->EventDel(&m_stRead);
						m_lpBase->EventDel(&m_stWrite);
						m_stSendMutex.lock();
						m_stOutput.Reset();
						m_stSendMutex.unlock();
						if(m_lpRecvBuff){
							free(m_lpRecvBuff);
							m_nRecvOffset = 0;
							m_nRecvOffset = 0;
							m_nRecvCapacity = 0;
							m_lpRecvBuff = nullptr;
						}
						m_nConnStatus = CONN_STATUS_CLOSED;
						close(nFd);
						m_lpCallback->OnEvent(CONNECT_EVENT_CONN_RECV_EOF);
						return;
					}
					memmove(lpNewBuf, m_lpRecvBuff+m_nRecvOffset, m_nRecvLength);
					free(m_lpRecvBuff);	
					m_lpRecvBuff = lpNewBuf;
					m_nRecvOffset = 0;
					m_nRecvCapacity = nNewCap;
				}else{
					memmove(m_lpRecvBuff, m_lpRecvBuff+m_nRecvOffset, m_nRecvLength);
					m_nRecvOffset = 0;
				}
				return;
			}
		}else{
			return;// 继续累计
		}
	}
}

// 阻塞与非阻塞：
// 阻塞下，执行DoDisconnect时若处于连接建立中，连接断开中，则会阻塞等待相应过程执行结束，方可获取锁，进入过程．
// 非阻塞下，执行DoDisconnect时若处于连接建立中，连接断开中，立即返回，返回值包含错误信息．
// 同步与异步：
// 同步下，若成功进入过程，则将同步等待到过程结束，再返回．
// 异步下，若成功进入过程，仅仅在发出请求后，即可返回．
int32_t Connect::DoDisconnect(bool bBlock, bool bSyn){
	// 过程进入
	if(bBlock){
		m_stConnMutex.lock();
	}
	else{
		bool bRet = m_stConnMutex.try_lock();
		if(bRet == false){
			return RET_CODE_DISCONN_MTX_GET_FAIL;
		}
	}

	if(m_nConnStatus == CONN_STATUS_CLOSED){
		m_stConnMutex.unlock();
		return RET_CODE_DISCONN_ALREADY;
	}
	m_nConnStatus = CONN_STATUS_DISCONNECTING;
	// 采用向关联base投递一个高优先级回调处理请求的方式，实现主动关闭连接的处理
	m_stDisconnCallback.m_lpArg = this;
	m_stDisconnCallback.m_lpOwner = nullptr;
	m_stDisconnCallback.m_nClosure = EV_CLOSURE_CB_SELF;
	m_stDisconnCallback.m_nFlags = 0;
	m_stDisconnCallback.m_nPri = 0;
	m_stDisconnCallback.m_stCbUnion.evcb_selfcb = ConnDisconnProcess;
    m_lpBase->EventCallbackActive(&m_stDisconnCallback);
	if(bSyn){
		while(true){
			if(m_nConnStatus == CONN_STATUS_CLOSED){
				break;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(10));  
		}
		return RET_CODE_DISCONN_SUCC;
	}else{
		return RET_CODE_DISCONNING;
	}
}

void Connect::DisconnProcess(EventCallback*){
	m_lpBase->EventDel(&m_stRead);
	m_lpBase->EventDel(&m_stWrite);
	m_nConnStatus = CONN_STATUS_CLOSED;
	m_stSendMutex.lock();
	m_stOutput.Reset();
	m_stSendMutex.unlock();
	if(m_lpRecvBuff){
		free(m_lpRecvBuff);
		m_nRecvOffset = 0;
		m_nRecvOffset = 0;
		m_nRecvCapacity = 0;
		m_lpRecvBuff = nullptr;
	}
	close(m_nFd);
	m_stConnMutex.unlock();
	m_lpCallback->OnEvent(CONNECT_EVENT_DISCONN_SUCC);
}

int32_t Connect::SendData(char* lpData, int32_t nLen){
	if(m_nConnStatus != CONN_STATUS_CONNECTED){
		return RET_CODE_STATUS_NOT_CONN;
	}
	int32_t ret = 0;
	int32_t send_ret = 0;
	m_stSendMutex.lock();
	ret = m_stOutput.PutData(lpData, nLen);
	event_debugx_("senddata ret %d,now size %d", ret, m_stOutput.GetSize());
	if(ret == 0){
		m_lpBase->EventAdd(&m_stWrite);
		send_ret = RET_CODE_OK;
	}else {
		send_ret = RET_CODE_SEND_ERR;
	}
	m_stSendMutex.unlock();
    return send_ret;
}

}