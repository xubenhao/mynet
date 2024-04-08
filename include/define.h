#ifndef _MYNET_DEFINE_H
#define _MYNET_DEFINE_H

namespace mynet{

// 阻塞与非阻塞：
// 阻塞下，执行DoConnect时若处于连接建立中，连接断开中，则会阻塞等待相应过程执行结束，方可获取锁，进入过程．
// 非阻塞下，执行DoConnect时若处于连接建立中，连接断开中，立即返回，返回值包含错误信息．
// 同步与异步：
// 同步下，若成功进入过程，则将同步等待到过程结束，再返回．
// 异步下，若成功进入过程，仅仅在发出请求后，即可返回．
#define RET_CODE_OK 					        0
#define RET_CODE_CONN_MTX_GET_FAIL		        1
#define RET_CODE_GET_ADDR_INFO_FAIL		        2
#define RET_CODE_SOCKET_FAIL			        3
#define RET_CODE_CONN_FAIL				        4
#define RET_CODE_CONN_ALREADY			        5
#define RET_CODE_CONN_SUCC				        6
#define RET_CODE_CONNING				        8
#define RET_CODE_DISCONN_MTX_GET_FAIL	        9
#define RET_CODE_DISCONN_ALREADY		        10
#define RET_CODE_DISCONNING				        11
#define RET_CODE_DISCONN_SUCC			        12
#define RET_CODE_STATUS_NOT_CONN		        13
#define RET_CODE_SEND_ERR				        15
#define RET_CODE_INDEX_ERR                      16
// 事件通知
#define CONNECT_EVENT_CONN_TIMEOUT				0
#define CONNECT_EVENT_CONN_FAIL					1
#define CONNECT_EVENT_CONN_SUCC					2
#define CONNECT_EVENT_CONN_SEND_ERR				3
#define CONNECT_EVENT_CONN_RECV_ERR				4
#define CONNECT_EVENT_CONN_RECV_EOF				5
#define CONNECT_EVENT_CONN_RECV_MALLOC_FAIL		6
#define CONNECT_EVENT_DISCONN_SUCC				7   
}
#endif