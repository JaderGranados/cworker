#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "queue/jobqueue.h"
#include "result/result_store.h"

typedef struct TcpServer TcpServer;

TcpServer *tcp_server_create(int port, JobQueue *queue, ResultStore *result_store);
int tcp_server_start(TcpServer *server);
void tcp_server_shutdown(TcpServer *server);
void tcp_server_destroy(TcpServer *server);

#endif // TCPSERVER_H
