#include "epoll.h"
#include "log.h"
#include "util.h"
#include "event_base.h"
namespace mynet{
Epoll::Epoll(EventBase* base):IoMulti(base)
{
    m_strName = "epoll";
    m_stFeatures = EV_FEATURE_ET|EV_FEATURE_O1|EV_FEATURE_EARLY_CLOSE;
    m_nFdInfoLen = 0;
	m_lpBase = base;
}

Epoll::~Epoll(){
	UnInit();
}

int32_t Epoll::Init(){
    int32_t epfd = INVALID_EPOLL_HANDLE;
	if ((epfd = epoll_create(32000)) == INVALID_EPOLL_HANDLE) {
        if (errno != ENOSYS)
            event_warn("epoll_create");
        return -1;
    }
    evutil_make_socket_closeonexec(epfd);
	epoll_event* lpEvents = (epoll_event*)malloc(INITIAL_NEVENT * sizeof(epoll_event));
	if (lpEvents == nullptr) {
		close_epoll_handle(epfd);
		return -1;
	}
	m_nNum = INITIAL_NEVENT;
	m_lpEvents = lpEvents;
	m_nEpFd = epfd;
	return 0;
}

int Epoll::Add(int32_t fd, short old, short events, void *p){
	struct event_change ch;
	ch.fd = fd;// 套接字
	ch.old_events = old;// 添加前此套接字上已经注册的事件集合
	ch.read_change = ch.write_change = ch.close_change = 0;
	if (events & EV_WRITE)
		ch.write_change = EV_CHANGE_ADD | (events & EV_ET);// 本次新增事件类型
	if (events & EV_READ)
		ch.read_change = EV_CHANGE_ADD | (events & EV_ET);
	if (events & EV_CLOSED)
		ch.close_change = EV_CHANGE_ADD | (events & EV_ET);
	return ApplyOneChange(&ch);
}

int Epoll::Del(int32_t fd, short old, short events, void *fdinfo){
	struct event_change ch;
	ch.fd = fd;
	ch.old_events = old;
	ch.read_change = ch.write_change = ch.close_change = 0;
	if (events & EV_WRITE)
		ch.write_change = EV_CHANGE_DEL | (events & EV_ET);
	if (events & EV_READ)
		ch.read_change = EV_CHANGE_DEL | (events & EV_ET);
	if (events & EV_CLOSED)
		ch.close_change = EV_CHANGE_DEL | (events & EV_ET);
	return ApplyOneChange(&ch);
}

static const char * change_to_string(int change)
{
	change &= (EV_CHANGE_ADD|EV_CHANGE_DEL);
	if (change == EV_CHANGE_ADD) {
		return "add";
	} else if (change == EV_CHANGE_DEL) {
		return "del";
	} else if (change == 0) {
		return "none";
	} else {
		return "???";
	}
}

static const char * epoll_op_to_string(int op)
{
	return op == EPOLL_CTL_ADD?"ADD":
	    op == EPOLL_CTL_DEL?"DEL":
	    op == EPOLL_CTL_MOD?"MOD":
	    "???";
}

#define PRINT_CHANGES(op, events, ch, status)  \
	"Epoll %s(%d) on fd %d " status ". "       \
	"Old events were %d; "                     \
	"read change was %d (%s); "                \
	"write change was %d (%s); "               \
	"close change was %d (%s)",                \
	epoll_op_to_string(op),                    \
	events,                                    \
	ch->fd,                                    \
	ch->old_events,                            \
	ch->read_change,                           \
	change_to_string(ch->read_change),         \
	ch->write_change,                          \
	change_to_string(ch->write_change),        \
	ch->close_change,                          \
	change_to_string(ch->close_change)

int Epoll::ApplyOneChange(const struct event_change *ch)
{
	struct epoll_event epev;
	int op, events = 0;
	int idx;
	idx = EPOLL_OP_TABLE_INDEX(ch);// 表索引
	op = epoll_op_table[idx].op;// 查表得到操作
	events = epoll_op_table[idx].events;// 得到操作参数
	if (!events) {
		assert(op == 0);
		return 0;
	}
	if ((ch->read_change|ch->write_change|ch->close_change) & EV_CHANGE_ET)
		events |= EPOLLET;// 额外添加ET属性
	memset(&epev, 0, sizeof(epev));
	epev.data.fd = ch->fd;// 注册时提供的额外数据是套接字描述符
	epev.events = events;
	if (epoll_ctl(m_nEpFd, op, ch->fd, &epev) == 0) {
		event_debug(PRINT_CHANGES(op, epev.events, ch, "okay"));
		return 0;
	}
	switch (op) {
	case EPOLL_CTL_MOD:
		if (errno == ENOENT) {
			if (epoll_ctl(m_nEpFd, EPOLL_CTL_ADD, ch->fd, &epev) == -1) {
				event_warn("Epoll MOD(%d) on %d retried as ADD; that failed too", (int)epev.events, ch->fd);
				return -1;
			} else {
				event_debug("Epoll MOD(%d) on %d retried as ADD; succeeded.", (int)epev.events, ch->fd);
				return 0;
			}
		}
		break;
	case EPOLL_CTL_ADD:
		if (errno == EEXIST) {
			if (epoll_ctl(m_nEpFd, EPOLL_CTL_MOD, ch->fd, &epev) == -1) {
				event_warn("Epoll ADD(%d) on %d retried as MOD; that failed too", (int)epev.events, ch->fd);
				return -1;
			} else {
				event_debug("Epoll ADD(%d) on %d retried as MOD; succeeded.", (int)epev.events, ch->fd);
				return 0;
			}
		}
		break;
	case EPOLL_CTL_DEL:
		if (errno == ENOENT || errno == EBADF || errno == EPERM) {
			event_debug("Epoll DEL(%d) on fd %d gave %s: DEL was unnecessary.", (int)epev.events, ch->fd, strerror(errno));
			return 0;
		}
		break;
	default:
		break;
	}
	event_warn(PRINT_CHANGES(op, epev.events, ch, "failed"));
	return -1;
}

int Epoll::Dispatch(struct timeval *tv){
	int i, res;
	long timeout = -1;
	if (tv != NULL) {
		timeout = evutil_tv_to_msec_(tv);
		if (timeout < 0 || timeout > MAX_EPOLL_TIMEOUT_MSEC) {
			timeout = MAX_EPOLL_TIMEOUT_MSEC;
		}
	}
	
	m_lpBase->th_base_lock.unlock();
	res = epoll_wait(m_nEpFd, m_lpEvents, m_nNum, timeout);// 等待io事件
	m_lpBase->th_base_lock.lock();
	if (res == -1) {
		if (errno != EINTR) {
			event_warn("epoll_wait");
			return (-1);// 遭遇异常
		}
		return (0);// 被中断唤醒
	}
	if(res){
		event_debug("%s: epoll_wait reports %d", __func__, res);
	}
	assert(res <= m_nNum);
	for (i = 0; i < res; i++) {// 逐个处理每个产生的事件
		int what = (m_lpEvents+i)->events;// 事件产生的原因
		short ev = 0;
		// 将特定系统特定io复用器的事件转化库的标准事件
		if (what & EPOLLERR) {// 这是产生了套接字错误
			ev = EV_READ | EV_WRITE;// 既可读，又可写
		} else if ((what & EPOLLHUP) && !(what & EPOLLRDHUP)) {
			ev = EV_READ | EV_WRITE;
		} else {
			if (what & EPOLLIN)
				ev |= EV_READ;
			if (what & EPOLLOUT)
				ev |= EV_WRITE;
			if (what & EPOLLRDHUP)
				ev |= EV_CLOSED;
		}
		if (!ev)
			continue;
		m_lpBase->IoActive((m_lpEvents+i)->data.fd, ev | EV_ET);
	}
	if (res == m_nNum && m_nNum < MAX_NEVENT) {
		int32_t nNewNum = m_nNum * 2;
		epoll_event *lpEEvents = (epoll_event*)realloc(m_lpEvents, nNewNum * sizeof(struct epoll_event));
		if (lpEEvents) {
			m_lpEvents = lpEEvents;
			m_nNum = nNewNum;
		}
	}
	return (0);
}

void Epoll::UnInit(){
	if (m_lpEvents){
		free(m_lpEvents);
		m_lpEvents = nullptr;
	}
	if (m_nEpFd != INVALID_EPOLL_HANDLE){
		close_epoll_handle(m_nEpFd);
		m_nEpFd = INVALID_EPOLL_HANDLE;
	}
}
}