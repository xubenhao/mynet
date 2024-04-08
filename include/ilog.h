#ifndef _MYNET_ILOG_H
#define _MYNET_ILOG_H

namespace mynet{
enum class LOGLEVEL{
    EVENT_LOG_ERR = 0,
    EVENT_LOG_WARN,
    EVENT_LOG_MSG,
    EVENT_LOG_DEBUG
};

typedef void (*log_cb)(LOGLEVEL nLevel, const char *msg);
void SetLogCb(log_cb lpLog);
}
#endif