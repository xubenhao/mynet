#include "util.h"
#include "log.h"
namespace mynet{
int evutil_open_closeonexec_(const char *pathname, int flags, unsigned mode) {
	int fd;
	fd = open(pathname, flags, (mode_t)mode);
	if (fd < 0)
		return -1;
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

int evutil_socketpair(int family, int type, int protocol, int32_t fd[2]) {
	return socketpair(family, type, protocol, fd);
}

int evutil_make_socket_nonblocking(int32_t fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        event_warn("fcntl(%d, F_GETFL)", fd);
        return -1;
    }
    if (!(flags & O_NONBLOCK)) {
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            event_warn("fcntl(%d, F_SETFL)", fd);
            return -1;
        }
    }
	return 0;
}

int evutil_make_listen_socket_reuseable(int32_t sock) {
	int one = 1;
	return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*) &one, (socklen_t)sizeof(one));
}
int evutil_make_listen_socket_reuseable_port(int32_t sock) {
	int one = 1;
	return setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (void*) &one, (socklen_t)sizeof(one));
}

int evutil_make_socket_closeonexec(int32_t fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFD, NULL)) < 0) {
		event_warn("fcntl(%d, F_GETFD)", fd);
		return -1;
	}
	if (!(flags & FD_CLOEXEC)) {
		if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
			event_warn("fcntl(%d, F_SETFD)", fd);
			return -1;
		}
	}
	return 0;
}

int evutil_closesocket(int32_t sock) {
	return close(sock);
}
int evutil_socket_connect_(int32_t *fd_ptr, const struct sockaddr *sa, int socklen) {
	int made_fd = 0;
	if (*fd_ptr < 0) {
		if ((*fd_ptr = socket(sa->sa_family, SOCK_STREAM, 0)) < 0)
			goto err;
		made_fd = 1;
		if (evutil_make_socket_nonblocking(*fd_ptr) < 0) {
			goto err;
		}
	}
	if (connect(*fd_ptr, sa, socklen) < 0) {// 非阻塞套接字发起连接
		int e = evutil_socket_geterror(*fd_ptr);
		if (EVUTIL_ERR_CONNECT_RETRIABLE(e))
			return 0;// 需要异步获取成功或失败的结果
		if (EVUTIL_ERR_CONNECT_REFUSED(e))
			return 2;// 立即被拒绝．立即失败．
		goto err;// 立即失败
	} else {
		return 1;// 立即成功
	}
err:
	if (made_fd) {
		evutil_closesocket(*fd_ptr);
		*fd_ptr = -1;
	}
	return -1;
}

/*  Check whether a socket on which we called connect() is done connecting.
	Return 1 for connected, 0 for not yet, -1 for error.
    In the error case, set the current socket errno to the error that happened during the connect operation. */
int evutil_socket_finished_connecting_(int32_t fd) {
	int e;
	socklen_t elen = sizeof(e);
	// -1 遭遇异步错误
	// 0  需要继续等待
	// 1  异步连接完成
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&e, &elen) < 0)// 一旦获取了错误信息。再获取就获取不到了。必须保存下来。
		return -1;
	if (e) {
		if (EVUTIL_ERR_CONNECT_RETRIABLE(e))
			return 0;// 依然需要继续等待
		EVUTIL_SET_SOCKET_ERROR(e);// 遭遇异步错误
		return -1;
	}
	return 1;
}

void close_epoll_handle(int32_t h){
	close(h);
}

static int evutil_fast_socket_nonblocking(int32_t fd) {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		event_warn("fcntl(%d, F_SETFL)", fd);
		return -1;
	}
	return 0;
}

static int evutil_fast_socket_closeonexec(int32_t fd) {
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
		event_warn("fcntl(%d, F_SETFD)", fd);
		return -1;
	}
}

int evutil_make_internal_pipe_(int32_t fd[2]) {
	if (pipe(fd) == 0) {
		if (evutil_fast_socket_nonblocking(fd[0]) < 0 || evutil_fast_socket_nonblocking(fd[1]) < 0 
			|| evutil_fast_socket_closeonexec(fd[0]) < 0 || evutil_fast_socket_closeonexec(fd[1]) < 0) {
			close(fd[0]);
			close(fd[1]);
			fd[0] = fd[1] = -1;
			return -1;
		}
		return 0;
	} else {
		event_warn("%s: pipe", __func__);
	}

	fd[0] = fd[1] = -1;
	return -1;
}


#define MAX_SECONDS_IN_MSEC_LONG \
	(((LONG_MAX) - 999) / 1000)

long evutil_tv_to_msec_(const struct timeval *tv)
{
	if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
		return -1;
	return (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
}

int32_t evutil_accept4_(int32_t sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
	int32_t result;
	result = accept(sockfd, addr, addrlen);
	if (result < 0)
		return result;
	if (flags & SOCK_CLOEXEC) {
		if (evutil_fast_socket_closeonexec(result) < 0) {
			evutil_closesocket(result);
			return -1;
		}
	}
	if (flags & SOCK_NONBLOCK) {
		if (evutil_fast_socket_nonblocking(result) < 0) {
			evutil_closesocket(result);
			return -1;
		}
	}
	return result;
}
}