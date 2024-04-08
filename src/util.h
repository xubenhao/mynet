#ifndef _MYNET_UTIL_H
#define _MYNET_UTIL_H
#include "std.h"
namespace mynet{
#define EV_SOCK_FMT "%d"
#define EV_SOCK_ARG(x) (x)

#define EVUTIL_SOCKET_ERROR() (errno)
#define EVUTIL_SET_SOCKET_ERROR(errcode)		\
		do { errno = (errcode); } while (0)
#define evutil_socket_geterror(sock) (errno)
#define evutil_socket_error_to_string(errcode) (strerror(errcode))
#define EVUTIL_INVALID_SOCKET -1   


#if EAGAIN == EWOULDBLOCK
#define EVUTIL_ERR_IS_EAGAIN(e) \
	((e) == EAGAIN)
#else
#define EVUTIL_ERR_IS_EAGAIN(e) \
	((e) == EAGAIN || (e) == EWOULDBLOCK)
#endif

/* True iff e is an error that means a read/write operation can be retried. */
#define EVUTIL_ERR_RW_RETRIABLE(e)				\
	((e) == EINTR || EVUTIL_ERR_IS_EAGAIN(e))
/* True iff e is an error that means an connect can be retried. */
#define EVUTIL_ERR_CONNECT_RETRIABLE(e)			\
	((e) == EINTR || (e) == EINPROGRESS)
/* True iff e is an error that means a accept can be retried. */
#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)			\
	((e) == EINTR || EVUTIL_ERR_IS_EAGAIN(e) || (e) == ECONNABORTED)
/* True iff e is an error that means the connection was refused */
#define EVUTIL_ERR_CONNECT_REFUSED(e)					\
	((e) == ECONNREFUSED)


int evutil_open_closeonexec_(const char *pathname, int flags, unsigned mode);
int evutil_socketpair(int family, int type, int protocol, int32_t fd[2]);
int evutil_make_socket_nonblocking(int32_t fd);
int evutil_make_listen_socket_reuseable(int32_t sock);
int evutil_make_listen_socket_reuseable_port(int32_t sock);
int evutil_make_socket_closeonexec(int32_t fd);
int evutil_closesocket(int32_t sock);
int evutil_socket_connect_(int32_t *fd_ptr, const struct sockaddr *sa, int socklen);
int evutil_socket_finished_connecting_(int32_t fd);
void close_epoll_handle(int32_t h);
int evutil_make_internal_pipe_(int32_t fd[2]);

#define MAX_SECONDS_IN_MSEC_LONG \
	(((LONG_MAX) - 999) / 1000)

long evutil_tv_to_msec_(const struct timeval *tv);
int32_t evutil_accept4_(int32_t sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
}
#endif