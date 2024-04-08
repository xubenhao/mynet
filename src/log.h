#ifndef _MYNET_LOG_H
#define _MYNET_LOG_H
#include "std.h"
#include "ilog.h"
namespace mynet
{
void event_err(int eval, const char *fmt, ...);
void event_warn(const char *fmt, ...);
void event_msg(const char *fmt, ...);
void event_debug(const char *fmt, ...);
void event_sock_err(int eval, int32_t sock, const char *fmt, ...);
void event_sock_warn(int32_t sock, const char *fmt, ...);

void event_errx(const char *fmt, ...);
void event_warnx(const char *fmt, ...);
void event_msgx(const char *fmt, ...);
void event_debugx_(const char *fmt, ...);
}
#endif