#include "log.h"
#include "util.h"
namespace mynet{

static void event_logv_(LOGLEVEL severity, const char *errstr, const char *fmt, va_list ap);
static void event_log(LOGLEVEL severity, const char *msg);
static log_cb log_fn = NULL;
void SetLogCb(log_cb lpLog){
	log_fn = lpLog;
}

void event_err(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_ERR, nullptr, fmt, ap);
	va_end(ap);
}

void event_warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_WARN, nullptr, fmt, ap);
	va_end(ap);
}

void event_msg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_MSG, nullptr, fmt, ap);
	va_end(ap);
}

void event_debug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_DEBUG, nullptr, fmt, ap);
	va_end(ap);
}

void event_sock_err(int eval, int32_t sock, const char *fmt, ...)
{
	va_list ap;
	int err = evutil_socket_geterror(sock);
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_ERR, evutil_socket_error_to_string(err), fmt, ap);
	va_end(ap);
}

void event_sock_warn(int32_t sock, const char *fmt, ...)
{
	va_list ap;
	int err = evutil_socket_geterror(sock);
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_WARN, evutil_socket_error_to_string(err), fmt, ap);
}


void event_errx(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_ERR, NULL, fmt, ap);
	va_end(ap);
	//event_exit(eval);
}

void event_warnx(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_WARN, NULL, fmt, ap);
	va_end(ap);
}

void event_msgx(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_MSG, NULL, fmt, ap);
	va_end(ap);
}

void event_debugx_(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	event_logv_(LOGLEVEL::EVENT_LOG_DEBUG, NULL, fmt, ap);
	va_end(ap);
}


static void event_logv_(LOGLEVEL severity, const char *errstr, const char *fmt, va_list ap)
{
	char buf[1024];
	size_t len;
	if (fmt != NULL){
		vsnprintf(buf, sizeof(buf), fmt, ap);
	}
	else{
		buf[0] = '\0';
	}
	if (errstr) {
		len = strlen(buf);
		if (len < sizeof(buf) - 3) {
			snprintf(buf + len, sizeof(buf) - len, ": %s", errstr);
		}
	}
	event_log(severity, buf);
}

static void event_log(LOGLEVEL severity, const char *msg)
{
	if (log_fn)
		log_fn(severity, msg);
	else {
		const char *severity_str;
		switch (severity) {
		case LOGLEVEL::EVENT_LOG_DEBUG:
			severity_str = "debug";
			break;
		case LOGLEVEL::EVENT_LOG_MSG:
			severity_str = "msg";
			break;
		case LOGLEVEL::EVENT_LOG_WARN:
			severity_str = "warn";
			break;
		case LOGLEVEL::EVENT_LOG_ERR:
			severity_str = "err";
			break;
		default:
			severity_str = "???";
			break;
		}
		pthread_t thread_id = pthread_self();  
		(void)fprintf(stderr, "tid_%d [%s] %s\n", thread_id, severity_str, msg);
	}
}
}