#ifndef _NET_UDP_OK_H_
#define _NET_UDP_OK_H_


#define UDP_SERVER (1)
#define UDP_CLIENT (2)


#define UDP_ERROR (-1)

#define MAX_UDP_BUF_SIZE (128*1024)

typedef struct _UDP_BUF_ARRAY_
{
	long time;
	int send_count;
	char buf[MAX_UDP_BUF_SIZE];
}UDP_BUF_ARRAY;


typedef struct _UDP_SOCKET_
{
	int socket_fd;
	int mode;
	int send_th;
	int recv_th;
	int max_buf_count;
	UDP_BUF_ARRAY * send_buf_ptr;
	UDP_BUF_ARRAY * recv_buf_ptr;
}UDP_SOCKET;

void close_socket(UDP_SOCKET * udp_fd);

#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)

#endif

