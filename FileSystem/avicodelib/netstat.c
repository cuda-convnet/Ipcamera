/* vi: set sw=4 ts=4: */
/*
 * Mini netstat implementation(s) for busybox
 * based in part on the netstat implementation from net-tools.
 *
 * Copyright (C) 2002 by Bart Visscher <magick@linux-fan.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * 2002-04-20
 * IPV6 support added by Bart Visscher <magick@linux-fan.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//#include "busybox.h"
//#include "pwd_.h"

#define MAX_SAVE_TCP_STATUS_NUM (100)

#define NETSTAT_CONNECTED	0x01
#define NETSTAT_LISTENING	0x02
#define NETSTAT_NUMERIC		0x04
#define NETSTAT_TCP			0x10
#define NETSTAT_UDP			0x20
#define NETSTAT_RAW			0x40
#define NETSTAT_UNIX		0x80

static int flags = NETSTAT_CONNECTED |
			NETSTAT_TCP | NETSTAT_UDP | NETSTAT_RAW | NETSTAT_UNIX;

#define PROGNAME_WIDTHs PROGNAME_WIDTH1(PROGNAME_WIDTH)
#define PROGNAME_WIDTH1(s) PROGNAME_WIDTH2(s)
#define PROGNAME_WIDTH2(s) #s

#define PRG_HASH_SIZE 211

enum {
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_TIME_WAIT,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_LISTEN,
    TCP_CLOSING			/* now a valid state */
};

static const char * const tcp_state[] =
{
    "",
    "ESTABLISHED",
    "SYN_SENT",
    "SYN_RECV",
    "FIN_WAIT1",
    "FIN_WAIT2",
    "TIME_WAIT",
    "CLOSE",
    "CLOSE_WAIT",
    "LAST_ACK",
    "LISTEN",
    "CLOSING"
};

typedef enum {
    SS_FREE = 0,		/* not allocated                */
    SS_UNCONNECTED,		/* unconnected to any socket    */
    SS_CONNECTING,		/* in process of connecting     */
    SS_CONNECTED,		/* connected to socket          */
    SS_DISCONNECTING		/* in process of disconnecting  */
} socket_state;

#define SO_ACCEPTCON    (1<<16)	/* performed a listen           */
#define SO_WAITDATA     (1<<17)	/* wait data to read            */
#define SO_NOSPACE      (1<<18)	/* no space to write            */


static unsigned long  max_txq = 0;
static int have_rstp_user = 0;
static int have_rstp_user_check_num = 0;

static int tcp_port[MAX_SAVE_TCP_STATUS_NUM] = {0};
static int tcp_txq[MAX_SAVE_TCP_STATUS_NUM] = {0};
static int tcp_rxq[MAX_SAVE_TCP_STATUS_NUM] = {0};
static int tcp_same_time[MAX_SAVE_TCP_STATUS_NUM] = {0};
static int tcp_search_no[MAX_SAVE_TCP_STATUS_NUM] = {0};
static int totol_search_no = 0;
static int tcp_data_available = 0;

int tcp_status_insert_array_func(int port,int txq,int rxq,int search_no)
{
	int i = 0;
	for( i = 0; i < MAX_SAVE_TCP_STATUS_NUM;i++)
	{
		if( tcp_port[i] == port )
		{
			if( (tcp_txq[i] == txq)  &&  (tcp_rxq[i] == rxq) )
			{
				if( (tcp_txq[i] == 0)  &&  (tcp_rxq[i] == 0) )
				{			
					
				}else
					tcp_same_time[i]++;
			}else
			{
				tcp_same_time[i] = 0;
			}

			tcp_txq[i] = txq;
			tcp_rxq[i] = rxq;
			tcp_search_no[i] = search_no;

			if( tcp_same_time[i] < 50 )
			{
				tcp_data_available = 1;
			}
			

			return 1;
		}
	}

	for( i = 0; i < MAX_SAVE_TCP_STATUS_NUM;i++)
	{
		if( tcp_port[i] == 0 )
		{
			tcp_port[i] = port;
			tcp_txq[i] = txq;
			tcp_rxq[i] = rxq;
			tcp_search_no[i] = search_no;
			tcp_same_time[i] = 0;
			return 1;
		}
	}

	return -1;	
}

int get_tcp_have_client(int search_no)
{
	//clear old connect
	int i = 0;
	for( i = 0; i < MAX_SAVE_TCP_STATUS_NUM;i++)
	{
		if( tcp_port[i] != 0 )
		{			
			if((search_no - tcp_search_no[i]) > 10 )
			{
				//printf("clear tcp port:%d\n",tcp_port[i]);
				tcp_port[i] = 0;
			}			
		}
	}

	for( i = 0; i < MAX_SAVE_TCP_STATUS_NUM;i++)
	{
		if( tcp_port[i] != 0 )
		{			
			//printf("[%d] %d-%d  %d  %d\n",tcp_port[i],tcp_txq[i],tcp_rxq[i],tcp_same_time[i], tcp_search_no[i]);
			if( tcp_same_time[i] < 10 )
				return 1;
		}
	}

	return 0;	
}

static char *itoa(unsigned int i)
{
	/* 21 digits plus null terminator, good for 64-bit or smaller ints */
	static char local[22];
	char *p = &local[21];
	*p-- = '\0';
	do {
		*p-- = '0' + i % 10;
		i /= 10;
	} while (i > 0);
	return p + 1;
}

static void tcp_do_one(int lnr, const char *line)
{
	char local_addr[64], rem_addr[64];
	const char *state_str;
	char more[512];
	int num, local_port, rem_port, d, state, timer_run, uid, timeout;
	struct sockaddr_in localaddr, remaddr;
	unsigned long rxq, txq, time_len, retr, inode;
	int have_connect = 0;
	int i;

	if (lnr == 0)
		return;

	memset(rem_addr,0x00,64);

	more[0] = '\0';
	num = sscanf(line,
				 "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
				 &d, local_addr, &local_port,
				 rem_addr, &rem_port, &state,
				 &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);	


	if (strlen(local_addr) > 8) {
#ifdef CONFIG_FEATURE_IPV6
		sscanf(local_addr, "%08X%08X%08X%08X",
			   &in6.s6_addr32[0], &in6.s6_addr32[1],
			   &in6.s6_addr32[2], &in6.s6_addr32[3]);
		inet_ntop(AF_INET6, &in6, addr6, sizeof(addr6));
		inet_pton(AF_INET6, addr6, (struct sockaddr *) &localaddr.sin6_addr);
		sscanf(rem_addr, "%08X%08X%08X%08X",
			   &in6.s6_addr32[0], &in6.s6_addr32[1],
			   &in6.s6_addr32[2], &in6.s6_addr32[3]);
		inet_ntop(AF_INET6, &in6, addr6, sizeof(addr6));
		inet_pton(AF_INET6, addr6, (struct sockaddr *) &remaddr.sin6_addr);
		localaddr.sin6_family = AF_INET6;
		remaddr.sin6_family = AF_INET6;
#endif
	} else {
		sscanf(local_addr, "%X",
			   &((struct sockaddr_in *) &localaddr)->sin_addr.s_addr);
		sscanf(rem_addr, "%X",
			   &((struct sockaddr_in *) &remaddr)->sin_addr.s_addr);
		((struct sockaddr *) &localaddr)->sa_family = AF_INET;
		((struct sockaddr *) &remaddr)->sa_family = AF_INET;
	}

	//printf("line:%s  %s  %d  state=%d\n",line,inet_ntoa(remaddr.sin_addr),rem_port,state);	

	tcp_data_available = 0;
	
	if( state == 1 )
	{		
		//printf("line:%ld-%ld  %d %s  %d  state=%d\n",txq,rxq,rem_port,inet_ntoa(remaddr.sin_addr),rem_port,state);	
		if( strcmp(inet_ntoa(remaddr.sin_addr),"127.0.0.1") != 0 )
		{	
			tcp_status_insert_array_func(rem_port,txq,rxq,totol_search_no);
			//have_rstp_user_check_num++;
		}
	}

	if (num < 10) {
		printf("warning, got bogus tcp line.");
		return;
	}
	state_str = tcp_state[state];
	if ((rem_port && (flags&NETSTAT_CONNECTED)) ||
		(!rem_port && (flags&NETSTAT_LISTENING)))
	{

	//	printf("tcp   %6ld %6ld %-23s %-23s %-12s\n",
	//		   rxq, txq, local_addr, rem_addr, state_str);

		
		if( max_txq < txq  && tcp_data_available == 1)
		{			
			max_txq  = txq;
		}
	

	}
}



static void dev_do_one(int lnr, const char *line)
{
	char local_addr[64], rem_addr[64];
	const char *state_str;
	char more[512];
	int num, local_port, rem_port, d, state, timer_run, uid, timeout;	

	more[0] = '\0';
	num = sscanf(line,
				 "%d  %s %d %d %64[0-9A-Fa-f]\n",
				 &d, more, &local_port,&rem_port,local_addr);

	printf("line:%s\n",line);

	if( strcmp(more,"eth0") == 0 )
	{
		max_txq = 1;
	}
	
}



#define _PATH_PROCNET_UDP "/proc/net/udp"
#define _PATH_PROCNET_UDP6 "/proc/net/udp6"
#define _PATH_PROCNET_TCP "/proc/net/tcp"
#define _PATH_PROCNET_TCP6 "/proc/net/tcp6"
#define _PATH_PROCNET_RAW "/proc/net/raw"
#define _PATH_PROCNET_RAW6 "/proc/net/raw6"
#define _PATH_PROCNET_UNIX "/proc/net/unix"
#define _PATH_PROCNET_DEV "/proc/net/dev_mcast"

static void do_info(const char *file, const char *name, void (*proc)(int, const char *))
{
	char buffer[8192];
	int lnr = 0;
	FILE *procinfo;

	procinfo = fopen(file, "r");
	if (procinfo == NULL) {
		if (errno != ENOENT) {
			perror(file);
		} else {
		printf("no support for `%s' on this system.", name);
		}
	} else {
		do {
			if (fgets(buffer, sizeof(buffer), procinfo))
				(proc)(lnr++, buffer);
		} while (!feof(procinfo));
		fclose(procinfo);
	}
}

/*
 * Our main function.
 */



unsigned long  netstat_main()
{
	max_txq =0;
	have_rstp_user_check_num = 0;
	
	do_info(_PATH_PROCNET_TCP,"AF INET (tcp)",tcp_do_one);

	have_rstp_user_check_num = get_tcp_have_client(totol_search_no);

	if( have_rstp_user_check_num > 0 )	
		have_rstp_user = 1;
	else
		have_rstp_user = 0;


	totol_search_no++;

	//printf("have_rstp_user_check_num = %d\n",have_rstp_user_check_num);

	return max_txq;
}

unsigned long nettset_dev()
{
	max_txq = 0;
	do_info(_PATH_PROCNET_DEV,"AF INET (tcp)",dev_do_one);
	return max_txq;
}

int netstat_have_rstp_user()
{
	//printf("have_rstp_user=%d\n",have_rstp_user);
	return have_rstp_user;
}

