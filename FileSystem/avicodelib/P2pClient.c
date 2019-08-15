#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include  <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>

#include "fic8120ctrl.h"
#include "filesystem.h"
#include "bufmanage.h"
#include "netserver.h"
#include "P2pClient.h"

typedef struct _URL_DNS_ST_
{
	char url[100];
	char ip[20];
	int is_not_get_ip;
}URL_DNS_ST;



typedef struct _NET_P2P_CLIENT
{
	int dvr_svr_socket;
	int svr_dvr_socket;
	int dvr_pc_socket;
	int dvr_svr_socket_port;
	int svr_dvr_socket_port;
	int dvr_pc_socket_port;
	int dvr_svr_client_socket;
	NETREQUEST net_request;
	int is_ready;
	char net_date[MAX_MSG_LEN]; //receive date array.
	int dvr_svr_socket_flag;
	int svr_dvr_socket_flag;
	int dvr_pc_socket_flag;
	int dvr_svr_socket_start_work;
	int is_p2p_connect_time;
	char server_ip[100];
	pthread_t p2p_client_to_svr_th;
	pthread_t p2p_client_to_svr_check_th;
}NET_P2P_CLIENT;

NET_P2P_CLIENT g_p2p_client;

static char p2p_server_ip[100] = "192.168.1.82";
static char p2p_dvr_id[100] ={0};//=  "200905041523321234";
static char p2p_soft_ver[100] ={0};
static pthread_mutex_t p2p_mutex;
static int connect_to_svr = 0;
static int connected_svr = 0;
static int net_ddns_flag = 0;
static int get_dns_flag = 0;
static int dvr_author_flag = 0;
static int ipcam_hex_id = 0;
static int enable_send_broadcast = 0;
static URL_DNS_ST dns_url[MAX_URL_DNS_NUM];


static int read_svr_log_info(char * server_ip);
static int read_dvr_id_log_info(char * dvr_id);
static int read_dvr_ver_info(char * dvr_id);

void ipcam_set_hex_id(int id)
{
	ipcam_hex_id = id;
}

int ipcam_get_hex_id()
{
	return ipcam_hex_id;
}

int init_dns_url()
{
	int i;

	for( i = 0; i < MAX_URL_DNS_NUM; i++)
	{
		int ret;
		char config_value[150] ="";
		char parameter_name[50] = "";
		memset(&dns_url[i],0x00,sizeof(URL_DNS_ST));

		sprintf(parameter_name,"Dns_Url_%d",i);		
		ret= cf_read_value("/mnt/mtd/ipcam_enc_config.txt",parameter_name,config_value);
		if( ret >= 0)	
		{
			strcpy(dns_url[i].url,config_value);
		}

		sprintf(parameter_name,"Dns_Ip_%d",i);
		ret= cf_read_value("/mnt/mtd/ipcam_enc_config.txt",parameter_name,config_value);
		if( ret >= 0)	
		{
			strcpy(dns_url[i].ip,config_value);
		}
	}

	return 1;
}

int get_dns_url_ip(char * svr_url,char * svr_ip)
{	
	int a;
	int i;
	int is_in_url_list = 0;

	for( i = 0; i < MAX_URL_DNS_NUM; i++)
	{
		//DPRINTK("%s  %s\n",dns_url[i].url,svr_url);
		if( strcmp(dns_url[i].url,svr_url) == 0 )
		{
			DPRINTK("Find  url %s in list\n",svr_url);
			is_in_url_list = 1;
			break;
		}
	}	 

	if( is_in_url_list == 1 )
	{
		if( dns_url[i].is_not_get_ip > 2 )
		{
			DPRINTK("Get from list is_not_get_ip=%d %s %s %d\n",dns_url[i].is_not_get_ip,dns_url[i].url,dns_url[i].ip,dns_url[i].is_not_get_ip);
				
			dns_url[i].is_not_get_ip++;

			if( dns_url[i].is_not_get_ip > 10 )
			{
				dns_url[i].is_not_get_ip = 0;				
			}else		
			{
				strcpy(svr_ip,dns_url[i].ip);
				return 1;		
			}
		}
	}
	
	DPRINTK("get_host_name\n");

	get_host_name(svr_url,svr_ip);	
	if( svr_ip[0] == 0 )
	{	
		for(a = 0; a < 8;a++)
		{
			if( svr_ip[a] == (char)NULL )				
				break;

			if( (svr_ip[a] < '0' || svr_ip[a] > '9') 
						&& (svr_ip[a] != '.')  )
			{			
				DPRINTK("url is %s,but can not get ip\n",svr_ip);	
				break;				
			}
		}
	}	

	if( is_in_url_list == 1  && svr_ip[0] == 0  )
	{			
		DPRINTK("Get from list is_not_get_ip=%d %s %s\n",dns_url[i].is_not_get_ip,dns_url[i].url,dns_url[i].ip);
		strcpy(svr_ip,dns_url[i].ip);	
		dns_url[i].is_not_get_ip++;
		return 1;			
	}else
	{
		if( is_in_url_list == 1 )
		{
			dns_url[i].is_not_get_ip = 0;
			strcpy(dns_url[i].ip,svr_ip);
		}
	}


	return 1;
}




int set_p2p_client_socket(int * client_socket,int client_port,int delaytime)
{
	struct  linger  linger_set;
//	int nNetTimeout;
	struct timeval stTimeoutVal;
//	struct sockaddr_in server_addr;
	int i;
	
	if( -1 == (*client_socket =  socket(PF_INET,SOCK_STREAM,0)))
	{
		DPRINTK( "DVR_NET_Initialize socket initialize error\n" );
		//return DVR_ERR_SOCKET_INIT;
		return ERR_NET_SET_ACCEPT_SOCKET;
	}

	//DPRINTK(" client_socket = %d\n",*client_socket);

	i = 1;

	if (setsockopt(*client_socket, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{		
		close( *client_socket );
		*client_socket = 0;
		DPRINTK("setsockopt SO_REUSEADDR error \n");		
		return ERR_NET_SET_ACCEPT_SOCKET;	
	}	

	linger_set.l_onoff = 1;
	linger_set.l_linger = 5;
       if (setsockopt( *client_socket, SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
       {
		close( *client_socket );
		*client_socket = 0;
		DPRINTK( "pNetSocket->sockSrvfd  SO_LINGER  error\n" );
		return ERR_NET_SET_ACCEPT_SOCKET;
	}


	  if( delaytime != 0 )
	  {
		stTimeoutVal.tv_sec = delaytime;
		stTimeoutVal.tv_usec = 0;
		if ( setsockopt( *client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	       {
	      		close( *client_socket );
			*client_socket = 0;
			printf( "setsockopt SO_SNDTIMEO error\n" );
			return ERR_NET_SET_ACCEPT_SOCKET;
		}
			 	
		if ( setsockopt( *client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	       {
	       	close( *client_socket );
			*client_socket = 0;
			printf( "setsockopt SO_RCVTIMEO error\n" );
			return ERR_NET_SET_ACCEPT_SOCKET;
		}
	  }

/*
	memset(&server_addr, 0, sizeof(struct sockaddr_in)); 
	server_addr.sin_family=AF_INET; 
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY); 
	//inet_aton("192.168.1.122", &server_ctrl->server_addr.sin_addr);
	server_addr.sin_port=htons(client_port); 

	if ( -1 == (bind(*client_socket,(struct sockaddr *)(&server_addr),
		sizeof(struct sockaddr))))
	{
		close( *client_socket );
		*client_socket = 0;
		DPRINTK( "DVR_NET_Initialize socket bind error\n" );
		//return DVR_ERR_SOCKET_BIND;
		return ERR_NET_SET_ACCEPT_SOCKET;
	}	
*/

	return 1;
	   
}

int init_p2p_socket()
{
//	int rel = 0;
	static int first = 0;

	if( first == 0 )
	{
		first = 1;
		g_p2p_client.dvr_pc_socket = 0;
		g_p2p_client.dvr_svr_socket = 0;
		g_p2p_client.svr_dvr_socket = 0;
		g_p2p_client.dvr_pc_socket_port = DVR_TO_PC_PORT;
		g_p2p_client.dvr_svr_socket_port = SVR_CONNECT_PORT;
		g_p2p_client.svr_dvr_socket_port = SVR_TO_DVR_PORT;
		g_p2p_client.dvr_pc_socket_flag = 0;
		g_p2p_client.dvr_svr_socket_flag = 0;
		g_p2p_client.svr_dvr_socket_flag = 0;
		g_p2p_client.dvr_svr_socket_start_work = 0;
		g_p2p_client.is_p2p_connect_time = 0;
		g_p2p_client.is_ready = 0;
		pthread_mutex_init(&p2p_mutex,NULL);
	}

	init_dns_url();
	//write_svr_log_info("192.168.1.82");
	//write_dvr_id_log_info("07587589723948");

	read_svr_log_info(p2p_server_ip);
	read_dvr_id_log_info(p2p_dvr_id);
	read_dvr_ver_info(p2p_soft_ver);

	{
		char tmp_buf[30];
		int i = 0;		
		memset(tmp_buf,0x00,30);
		
		tmp_buf[0] = 'B';
		strcpy(&tmp_buf[1],p2p_soft_ver);

		for( i = 0 ;i < strlen(tmp_buf); i++)
		{
			if( tmp_buf[i] == 'C' )
				tmp_buf[i] = 0;
		}

		strcpy(p2p_soft_ver,tmp_buf);		
	}
	

	if( p2p_server_ip[0] == 0 )
		strcpy(p2p_server_ip,"www.fab111.com");
	

	g_p2p_client.is_ready = 1;
	start_p2p_svr_to_dvr_thread();
	

	return 1;
	
}

int p2p_login(char * svr_ip,int svr_port)
{
	NETREQUEST client_request;
	struct sockaddr_in addr;
	int * socket_fd;
	char * send_date;
	int send_num;
	int net_cmd;
	int rel;
	int server_port;
	int http_port;
	char ip_buf[200];
	int a;

	get_server_port(&server_port,&http_port);	
 
	memset(ip_buf,0,200);
			DPRINTK("get_dns_url_ip\n");
	get_dns_url_ip(svr_ip,ip_buf);

	// 没有得到dns.
	if( ip_buf[0] == 0 )
	{
		for(a = 0; a < 8;a++)
		{
			if( svr_ip[a] == (char)NULL )
				break;

			if( (svr_ip[a] < '0' || svr_ip[a] > '9') 
						&& (svr_ip[a] != '.')  )
			{			
				printf("url is %s,but can not get ip\n",svr_ip);
				return -1;
			}
		}
	}

	close_dvr_svr_socket();

	g_p2p_client.is_p2p_connect_time = 0;

	rel = set_p2p_client_socket(&g_p2p_client.dvr_svr_socket,SVR_CONNECT_PORT,30);
	if( rel < 0 )
		return rel;

	//printf("g_p2p_client.dvr_svr_socket = %d\n",g_p2p_client.dvr_svr_socket);

	socket_fd = &g_p2p_client.dvr_svr_socket;
	send_date = g_p2p_client.net_date;
	send_num = MAX_MSG_LEN;
	
	addr.sin_family = AF_INET;
	if( ip_buf[0] == 0 )
      		addr.sin_addr.s_addr = inet_addr(svr_ip);
	else
		addr.sin_addr.s_addr = inet_addr(ip_buf);
       addr.sin_port = htons(svr_port);	

	g_p2p_client.dvr_svr_socket_flag = 1;   

	printf("%c connect\n",ip_buf[0]);
	
	if (connect(*socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
		perror(strerror(errno));
		
             if(errno == EINPROGRESS) 
		{			
             		fprintf(stderr, "connect timeout\n");	
		}  
		
		printf("can't connect\n");
		 
		  close_dvr_svr_socket();
                return -1;
        }

	//printf("g_p2p_client.dvr_svr_socket = %d  %d\n",g_p2p_client.dvr_svr_socket,*socket_fd);
	
        //DPRINTK("connected\n");	

	strcpy(g_p2p_client.server_ip, svr_ip);

	memset(&client_request,0,sizeof(NETREQUEST));
	//client_request.cmd = NET_LOGIN;

	client_request.cmd = NET_LOGIN;
	strcpy(client_request.dvrName ,p2p_dvr_id);	
		
	strcpy(client_request.SuperPassWord ,"dvr");	
	client_request.port1 = server_port;
	client_request.port2 = http_port;

	p2p_set_send_cmd(send_date,&client_request);

	//rel = send(*socket_fd,send_date,send_num, 0 );	

	rel = socket_send_data(*socket_fd,send_date,send_num);

	//printf("g_p2p_client.dvr_svr_socket = %d  %d\n",g_p2p_client.dvr_svr_socket,*socket_fd);
	if( rel < 0 )
	{		
		perror(strerror(errno));
		printf("p2p_login send date error!\n");
		 close_dvr_svr_socket();
		return ERR_NET_DATE;
	}

	rel = socket_read_data(*socket_fd,send_date,send_num);
	if( rel < 0 )
	{		
		printf("p2p_login send date error!\n");
		close_dvr_svr_socket();
		return ERR_NET_DATE;
	}

	p2p_set_time_cmd(send_date);

	net_cmd = *(int *)send_date;

	close_dvr_svr_socket();

	if( net_cmd == ERR_ALREADY_MOST_USER )
	{
		DPRINTK("server is full,no site\n");		
		return ERR_ALREADY_MOST_USER;
	}	

	if( net_cmd == NET_LOGIN_NOT_AUTHORIZATION )
	{
		DPRINTK("Dvr is not authorization!\n");
		write_dvr_not_authorization_info();
		return 1;
	}

	if( net_cmd == NET_LOGIN_SHELL )
	{
		char mycmd[200];
		DPRINTK("Dvr run shell!\n");

		strcpy(mycmd,(char*)(send_date + sizeof(int) + 200));

		DPRINTK("Run cmd: %s!\n",mycmd);

		if( strcmp(mycmd,"reset") == 0 )
		{
			exit(0);
		}else
		{
			command_drv(mycmd);
		}

		return 1;
	}
	
	DPRINTK("login over\n");
	
	return 1;
}



int set_client_udp_socket(int idx, int socket_fd)
{
	struct  linger  linger_set;
//	int nNetTimeout;
	struct timeval stTimeoutVal;
	int keepalive = 1;
	int keepIdle = 30;
	int keepInterval = 3;
	int keepCount = 3;

	
	linger_set.l_onoff = 1;
	linger_set.l_linger = 0;

    if (setsockopt( socket_fd, SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
    {
		printf( "setsockopt SO_LINGER  error\n" );
	}

/*	
     if (setsockopt( socket_fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(int) ) < 0 )
       {
		printf( "setsockopt SO_KEEPALIVE error\n" );
	}

	
	if(setsockopt( socket_fd,SOL_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle)) < -1)
	{
		printf( "setsockopt TCP_KEEPIDLE error\n" );
	}

	
	if(setsockopt( socket_fd,SOL_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval)) < -1)
	{
		printf( "setsockopt TCP_KEEPINTVL error\n" );
	}

	
	if(setsockopt( socket_fd,SOL_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount)) < -1)
	{
		printf( "setsockopt TCP_KEEPCNT error\n" );
	}
	*/

	stTimeoutVal.tv_sec = 5;
	stTimeoutVal.tv_usec = 0;
	if ( setsockopt( socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
       {
		printf( "setsockopt SO_SNDTIMEO error\n" );
	}
		 	
	if ( setsockopt( socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
       {
		printf( "setsockopt SO_RCVTIMEO error\n" );
	}


	return 1;

}



int p2p_udp_login(char * svr_ip,int svr_port)
{
	NETREQUEST client_request;
	struct sockaddr_in addr;
	int  socket_fd;
	char  send_date[1024];
	int send_num;
	int net_cmd;
	int rel;
	char ip_buf[200];
	int a;		
	int ret;
	int server_port;
	int http_port;

 	get_server_port(&server_port,&http_port);	
	memset(ip_buf,0,200);
	
			DPRINTK("get_dns_url_ip\n");
	get_dns_url_ip(svr_ip,ip_buf);

	// 没有得到dns.
	if( ip_buf[0] == 0 )
	{
		for(a = 0; a < 8;a++)
		{
			if( svr_ip[a] == (char)NULL )
				break;

			if( (svr_ip[a] < '0' || svr_ip[a] > '9') 
						&& (svr_ip[a] != '.')  )
			{			
				printf("url is %s,but can not get ip\n",svr_ip);
				return -1;
			}
		}
	}	


	memset(&client_request,0,sizeof(NETREQUEST));
	
	client_request.cmd = NET_LOGIN;
	strcpy(client_request.dvrName ,p2p_dvr_id);	
		
	strcpy(client_request.SuperPassWord ,"dvr");	
	client_request.port1 = server_port;
	client_request.port2 = http_port;	

	p2p_set_send_cmd(send_date,&client_request);	


	socket_fd = socket(PF_INET,SOCK_DGRAM,0);
	
	if( socket_fd == -1 )
	{
		DPRINTK("error!\n");
		return -1;
	}	

	
	
	int len =sizeof(addr);
	addr.sin_family=AF_INET;
	addr.sin_port=htons(svr_port); 
	if( ip_buf[0] == 0 )
      	addr.sin_addr.s_addr = inet_addr(svr_ip);
	else
		addr.sin_addr.s_addr = inet_addr(ip_buf);


	set_client_udp_socket(0,socket_fd);	

	ret = sendto(socket_fd,send_date,512,0,(struct sockaddr*)&addr,len);

	if( ret <= 0 )
	{
		DPRINTK("sendto error!\n");
		ret = -1;
	}else
	{		

		if (recvfrom(socket_fd,send_date,512,0,(struct sockaddr*)&addr,&len) > 0)
		{
			ret = 1;
		}
		else
		{
			ret = -1;
		}				
	}

	close(socket_fd);	
	

	net_cmd = *(int *)send_date;	

	if( net_cmd == ERR_ALREADY_MOST_USER )
	{
		DPRINTK("server is full,no site\n");			
	}

	if( net_cmd == NET_LOGIN_NOT_AUTHORIZATION )
	{
		DPRINTK("Dvr is not authorization!\n");
		write_dvr_not_authorization_info();
		return 1;
	}

	if( net_cmd == NET_LOGIN_SHELL )
	{
		char mycmd[200];
		DPRINTK("Dvr run shell!\n");

		strcpy(mycmd,(char*)(send_date + sizeof(int) + 200));

		DPRINTK("Run cmd: %s!\n",mycmd);

		if( strcmp(mycmd,"reset") == 0 )
		{
			exit(0);
		}else
		{
			command_drv(mycmd);
		}

		return 1;
	}

	if(ret == 1 )	
		DPRINTK("login over\n");
	else
		DPRINTK("login error\n");

	return ret;
}



int p2p_get_mac_id(char * svr_ip,int svr_port)
{
	NETREQUEST client_request;
	struct sockaddr_in addr;
	int * socket_fd;
	char * send_date;
	int send_num;
//	int net_cmd;
	int rel;
	int server_port;
	int http_port;
	char ip_buf[200];
	int a;

	get_server_port(&server_port,&http_port);		
 
	memset(ip_buf,0,200);
			DPRINTK("get_dns_url_ip\n");
	get_dns_url_ip(svr_ip,ip_buf);	

	// 没有得到dns.
	if( ip_buf[0] == 0 )
	{
		for(a = 0; a < 8;a++)
		{
			if( svr_ip[a] == (char)NULL )
				break;

			if( (svr_ip[a] < '0' || svr_ip[a] > '9') 
						&& (svr_ip[a] != '.')  )
			{			
				printf("url is %s,but can not get ip\n",svr_ip);
				return -1;
			}
		}
	}	

	close_dvr_svr_socket();

	g_p2p_client.is_p2p_connect_time = 0;

	rel = set_p2p_client_socket(&g_p2p_client.dvr_svr_socket,SVR_CONNECT_PORT,30);
	if( rel < 0 )
		return rel;

	//printf("g_p2p_client.dvr_svr_socket = %d\n",g_p2p_client.dvr_svr_socket);

	socket_fd = &g_p2p_client.dvr_svr_socket;
	send_date = g_p2p_client.net_date;
	send_num = MAX_MSG_LEN;
	
	addr.sin_family = AF_INET;
       if( ip_buf[0] == 0 )
      		addr.sin_addr.s_addr = inet_addr(svr_ip);
	else
		addr.sin_addr.s_addr = inet_addr(ip_buf);
       addr.sin_port = htons(svr_port);	

	g_p2p_client.dvr_svr_socket_flag = 1;   
	
	if (connect(*socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
               if (errno == EINPROGRESS) 
		{
                        fprintf(stderr, "connect timeout\n");			  
               }                
		  close_dvr_svr_socket();
                return -1;
        }

	//printf("g_p2p_client.dvr_svr_socket = %d  %d\n",g_p2p_client.dvr_svr_socket,*socket_fd);
    

	strcpy(g_p2p_client.server_ip, svr_ip);

	memset(&client_request,0,sizeof(NETREQUEST));
	//client_request.cmd = NET_LOGIN;

	client_request.cmd = NET_DVR_GET_MAC_ID;				
	strcpy(client_request.SuperPassWord ,"nethdcam");	
	client_request.port1 = server_port;
	client_request.port2 = http_port;

	p2p_set_send_cmd(send_date,&client_request);

	//rel = send(*socket_fd,send_date,send_num, 0 );	

	rel = socket_send_data(*socket_fd,send_date,send_num);

	//printf("g_p2p_client.dvr_svr_socket = %d  %d\n",g_p2p_client.dvr_svr_socket,*socket_fd);
	if( rel < 0 )
	{		
		perror(strerror(errno));
		printf("p2p_login send date error!\n");
		 close_dvr_svr_socket();
		return ERR_NET_DATE;
	}

	rel = socket_read_data(*socket_fd,send_date,send_num);
	if( rel < 0 )
	{		
		printf("p2p_login send date error!\n");
		close_dvr_svr_socket();
		return ERR_NET_DATE;
	}

	client_request = get_client_request(send_date);

	close_dvr_svr_socket();

	if( client_request.cmd == NET_DVR_GET_MAC_ID )
	{
		printf("get mac id: %s\n",client_request.dvrName);		

		strcpy(p2p_dvr_id,client_request.dvrName);
		
		write_dvr_id_log_info(p2p_dvr_id);

		DPRINTK("Get new id,so reboot\n");

		SendM2("reboot");
	}else
	{
		return -1;
	}
	
	
	return 1;
}


int get_svr_tcp_udp_port(int * tcp_port,int * udp_port,int * only_udp)
{
	char cmd[350];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
	int i,j;
	char buf[60];


	fp = fopen("/mnt/mtd/dvr_port.txt","r");
	if( fp == NULL )
	{
		printf(" can't open  /mnt/mtd/dvr_port.txt\n");
		goto GET_ERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto GET_ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto GET_ERROR;
	}	

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 5 )
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			goto GET_ERROR;
		}	

		cmd[fileOffset] = '\0';

		i = FindSTR(cmd,"[",0);		
		if( i < 0)		
			goto GET_ERROR;

		j = FindSTR(cmd,"]",0);		
		if( j < 0)		
			goto GET_ERROR;

		rel = GetTmpLink(cmd,buf,i,j,10);
		if( rel < 0)		
			goto GET_ERROR;		

		printf("get tcp_port:%s\n",buf);
		*tcp_port = atoi(buf);

		i = FindSTR(cmd,"[",j);		
		if( i < 0)		
			goto GET_ERROR;

		j = FindSTR(cmd,"]",i);		
		if( j < 0)		
			goto GET_ERROR;

		rel = GetTmpLink(cmd,buf,i,j,10);
		if( rel < 0)		
			goto GET_ERROR;		

		printf("get udp_port:%s\n",buf);
		*udp_port = atoi(buf);	


		i = FindSTR(cmd,"[",j);		
		if( i < 0)		
			goto GET_ERROR;

		j = FindSTR(cmd,"]",i);		
		if( j < 0)		
			goto GET_ERROR;

		rel = GetTmpLink(cmd,buf,i,j,10);
		if( rel < 0)		
			goto GET_ERROR;		

		printf("only use udp:%s\n",buf);
		*only_udp = atoi(buf);	

		
	}

	fclose(fp);
	fp = NULL;

	return 1;

GET_ERROR:
	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}
	
	return  -1;
}



int p2p_login_lock(char * svr_ip,int svr_port)
{
	int ret;
	static int tcp_port = 0,udp_port =0,only_udp=0;

	if( tcp_port == 0 )
	{
		ret = get_svr_tcp_udp_port(&tcp_port,&udp_port,&only_udp);

		if( ret < 0 )
		{
			tcp_port = SVR_LISTEN_PORT;
			udp_port = SVR_LISTEN_UDP_PORT;
			only_udp = 0;
		}

		DPRINTK("tcp=%d udp=%d onlyudp=%d\n",tcp_port,udp_port,only_udp);
	}

	if( only_udp == 1 )
	{
		ret = p2p_udp_login(svr_ip,udp_port);
	}else
	{
		ret =p2p_login(svr_ip,tcp_port);

		if( ret < 0 )
		{
			ret = p2p_udp_login(svr_ip,udp_port);
		}
	}
	

	return ret;
}
void login_to_svr()
{
	connect_to_svr =  1;
}


int p2p_alive(char * svr_ip,int svr_port)
{
	NETREQUEST client_request;
//	struct sockaddr_in addr;
	int * socket_fd;
	char * send_date;
	int send_num;
	int rel;

	socket_fd = &g_p2p_client.dvr_svr_socket;
	send_date = g_p2p_client.net_date;
	send_num = MAX_MSG_LEN;
	

	g_p2p_client.dvr_svr_socket_flag = 1;   

	memset(&client_request,0,sizeof(NETREQUEST));
	client_request.cmd = NET_ALIVE;	

	p2p_set_send_cmd(send_date,&client_request);	

	if( g_p2p_client.dvr_svr_socket_start_work != 1 )
		return ERR_NET_DATE;
	
	rel = socket_send_data(*socket_fd,send_date,send_num);	
	
	if( rel < 0 )
	{
		//close_p2p_socket();
		DPRINTK("p2p_alive send date error!\n");
		return ERR_NET_DATE;
	}	

	
	return 1;
}

int p2p_create_svr_to_dvr_hole(char * svr_ip,int svr_port)
{
	NETREQUEST client_request;
	struct sockaddr_in addr;
	int * socket_fd;
	char * send_date;
	int send_num;
	int rel;

	printf("p2p_create_svr_to_dvr_hole: %s %d\n",svr_ip,svr_port);

	rel = set_p2p_client_socket(&g_p2p_client.svr_dvr_socket,SVR_TO_DVR_PORT,3);
	if( rel < 0 )
		return rel;

	socket_fd = &g_p2p_client.svr_dvr_socket;
	send_date = g_p2p_client.net_date;
	send_num = MAX_MSG_LEN;
	
	addr.sin_family = AF_INET;
       addr.sin_addr.s_addr = inet_addr(svr_ip);
       addr.sin_port = htons(svr_port);	

	g_p2p_client.svr_dvr_socket_flag= 1;   
	
	if (connect(*socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
               if (errno == EINPROGRESS) 
		{
                        fprintf(stderr, "create hole timeout\n");			  
               }      

		close_svr_dvr_socket();
               return -1;
        }
        printf("create hole connected\n");	

	memset(&client_request,0,sizeof(NETREQUEST));
	client_request.cmd = NET_CREATE_HOLE;
	strcpy(client_request.dvrName ,"abc");
	strcpy(client_request.SuperPassWord ,"000000");
	strcpy(client_request.NormalPassWord,"111111");

	p2p_set_send_cmd(send_date,&client_request);

	rel = socket_send_data(*socket_fd,send_date,send_num);
	if( rel < 0 )
	{	
		close_svr_dvr_socket();
		return ERR_NET_DATE;
	}

	printf("p2p_create_svr_to_dvr_hole over\n");
	close_svr_dvr_socket();
	return 1;
}



int p2p_create_dvr_to_pc_hole(char * svr_ip,int svr_port)
{
	NETREQUEST client_request;
	struct sockaddr_in addr;
	int * socket_fd;
	char * send_date;
	int send_num;
	int rel;

	socket_fd = &g_p2p_client.dvr_pc_socket;
	send_date = g_p2p_client.net_date;
	send_num = MAX_MSG_LEN;
	
	addr.sin_family = AF_INET;
       addr.sin_addr.s_addr = inet_addr(svr_ip);
       addr.sin_port = htons(svr_port);	

	g_p2p_client.dvr_pc_socket_flag= 1;   
	
	if (connect(*socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
               if (errno == EINPROGRESS) 
		{
                        fprintf(stderr, "create hole timeout\n");			  
               }      	  
               return -1;
        }
        printf("create hole connected\n");	

	memset(&client_request,0,sizeof(NETREQUEST));
	client_request.cmd = NET_CREATE_HOLE;
	strcpy(client_request.dvrName ,"abc");
	strcpy(client_request.SuperPassWord ,"000000");
	strcpy(client_request.NormalPassWord,"111111");

	p2p_set_send_cmd(send_date,&client_request);

	rel = socket_send_data(*socket_fd,send_date,send_num);
	if( rel < 0 )
	{		
		return ERR_NET_DATE;
	}

	printf("p2p_create_dvr_to_pc_hole over\n");
	return 1;
}


int p2p_set_send_cmd(char * buf,NETREQUEST * client_request)
{
	memset(buf,0,MAX_MSG_LEN);
	*(int *)buf = client_request->cmd;
	strcpy((char *)(buf + sizeof(int)),client_request->dvrName);
	strcpy((char *)(buf + sizeof(int) + 200),client_request->SuperPassWord);
	strcpy((char *)(buf + sizeof(int) + 200 + 10),client_request->NormalPassWord);	
	*(int *)(buf + sizeof(int) + 200 + 10  + 10) = client_request->port1;
	*(int *)(buf + sizeof(int) + 200 + 10  + 10 + sizeof(int)) = client_request->port2;
	
	return 1;
}

time_t FTC_CustomTimeTotime_t2(int year,int month, int day, int hour, int minute,int second)
{
	struct tm tp;
	time_t mtime;	

	year = (year>1900) ? year-1900 : 0;
	month = (month>0) ? month-1 : 0;

	tp.tm_year = year;
	tp.tm_mon = month;
	tp.tm_mday = day;
	tp.tm_hour  = hour;
	tp.tm_min   = minute;
	tp.tm_sec   = second;
	tp.tm_isdst = 0;

	mtime = mktime(&tp);

	//printf(" mtime = %ld\n",mtime);

	//printf("time is %d %d %d %d %d %d  %ld\n",year,month,day,hour,minute,second,mtime);

	return mtime;	
}

void  FTC_time_tToCustomTime2(time_t time,int * year, int * month,int* day,int* hour,int* minute,int* second )
{	
	 struct tm tp;
	
	 gmtime_r(&time,&tp);

	 *year = tp.tm_year + 1900;
	 *month = tp.tm_mon + 1;
	 *day = tp.tm_mday;
	 *hour = tp.tm_hour;
	 *minute = tp.tm_min;
	 *second = tp.tm_sec;		
}

static int enable_set_time = 1;
static int change_timezone_set_time = 0;

int p2p_enable_set_time()
{
	enable_set_time = 1;

	return 1;
}


int time_zone_change()
{
	change_timezone_set_time = 1;

	return 1;
}


int p2p_set_time_cmd(char * buf)
{	
	int ymd;
	int hms;	
	int y,mon,d,h,m,s;
	time_t tmp_time = 0;	
	int time_zone_num;	
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

	if( enable_set_time  == 1 )
	{	
		if(1)
		{
			struct timeval this_time;
			get_sys_time( &this_time, NULL );

			FTC_time_tToCustomTime2(this_time.tv_sec,&y,&mon,&d,&h,&m,&s);


			if( change_timezone_set_time == 0 )
			{
				if( (y*100 +mon)  > 201406 )
				{
					DPRINTK("year=%d %d  > 201406,not set UTC time\n",y,mon);
					return 1;
				}
			}else
			{
				DPRINTK("change_timezone_set_time = %d,so set time\n",change_timezone_set_time);
				change_timezone_set_time = 0;
			}
		}

	
		ymd = *(int *)(buf + sizeof(int) + 200 + 10  + 10 + sizeof(int) + sizeof(int));
		hms = *(int *)(buf + sizeof(int) + 200 + 10  + 10 + sizeof(int) + sizeof(int) + sizeof(int));

		DPRINTK("set time %d %d\n",ymd,hms);

		y = ymd /10000;
		mon =  (ymd%10000)/100;
		d =  ymd%100;

		h = hms /10000;
		m =  (hms%10000)/100;
		s =  hms%100;
		
		
		tmp_time = FTC_CustomTimeTotime_t2(y,mon,d,h,m,s);
		tmp_time += pAllInfo->sys_st.time_zone*3600;

		FTC_time_tToCustomTime2(tmp_time,&y,&mon,&d,&h,&m,&s);

		sys_set_date(y,mon,d);
		sys_set_time(h,m,s);	

		enable_set_time  = 0;
	}
	
	return 1;
}

NETREQUEST get_client_request(char *cmdBuf)
{
	NETREQUEST myRequest;
	memset(&myRequest,0,sizeof(NETREQUEST));
	myRequest.cmd = *(int *)cmdBuf;
	strcpy(myRequest.dvrName,(char *)(cmdBuf + sizeof(int)));
	strcpy(myRequest.SuperPassWord,(char *)(cmdBuf + sizeof(int) + 200));
	strcpy(myRequest.NormalPassWord,(char *)(cmdBuf + sizeof(int) + 200 + 10));	
	myRequest.port1 = *(int *)(cmdBuf + sizeof(int) + 200 + 10  + 10);
	myRequest.port2 = *(int *)(cmdBuf + sizeof(int) + 200 + 10  + 10 + sizeof(int));

	return myRequest;
}


int close_p2p_socket()
{
	if(  g_p2p_client.dvr_svr_socket_flag > 0 )
	{
		//shutdown( g_p2p_client.dvr_svr_socket, SHUT_RDWR );		
		close( g_p2p_client.dvr_svr_socket);
		//g_p2p_client.dvr_svr_socket_flag = 0;
	}

	return 1;

}

int  p2p_client_to_svr_thread(void * value)
{	
	int ret,ret1;
//	struct sockaddr_in adTemp;
//	struct  linger  linger_set;
	int sin_size;
	struct  timeval timeout;
//	int tmp_fd = 0;	
//	struct timeval stTimeoutVal;
//	int listen_fd;
//	struct sockaddr_in server_addr;
	fd_set watch_set,catch_set;
	int client_fd = 0;
	char * send_date;
	int send_num;

	client_fd = g_p2p_client.dvr_svr_socket;
	send_date = g_p2p_client.net_date;
	send_num = MAX_MSG_LEN;

	
	sin_size=sizeof(struct sockaddr_in);

	FD_ZERO( &catch_set);
	FD_SET( client_fd, &catch_set ) ;

	printf(" p2p_client_to_svr_thread start!\n");

	g_p2p_client.dvr_svr_socket_start_work = 1;

	while( g_p2p_client.dvr_svr_socket_start_work == 1)
	{
		FD_ZERO( &watch_set);
		watch_set = catch_set;

		timeout.tv_sec = 0;
		timeout.tv_usec = 10;

		ret  = select( client_fd+1, 
			&watch_set, NULL, NULL, &timeout );

		if (ret == 0)
		{  			
		  	continue;
		}
		
		if ( ret < 0 )
		{
			DPRINTK( "dvr_svr_socket_start_work error user\n" );
			goto ERROR_END;
		}

		if ( ret > 0 && FD_ISSET( client_fd, &watch_set ) )
		{
			ret1 = recv_command(client_fd);
			if( ret1 < 0 )
			{		
				DPRINTK(" read date error!\n");
				goto ERROR_END;
			}
			
		}
		
	}

	 g_p2p_client.dvr_svr_socket_start_work = 0;

	 printf(" p2p_client_to_svr_thread out!\n");

	 return 1;

ERROR_END:
	
	close_dvr_svr_socket();

	g_p2p_client.dvr_svr_socket_start_work = 0;

	printf(" p2p_client_to_svr_thread out!\n");

	return -1;
}

void net_ddns_open_close_flag(int flag)
{
	if( flag == 1 )
		net_ddns_flag = 1;
	else
		net_ddns_flag = 0;

	printf("net_ddns_open_close_flag flag=%d %d\n",flag,net_ddns_flag);
}


int try_get_dns()
{
	char cmd[350];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
	int j;
//	int pid;

	sprintf(cmd,"udhcpc -s /mnt/mtd/simple1.script -n > /tmp/dns.txt");	

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		goto GET_ERROR;
	}

	kill_dhcp();

	fp = fopen("/tmp/dns.txt","r");
	if( fp == NULL )
	{
		printf(" open /tmp/dns.txt error!\n");
		goto GET_ERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto GET_ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto GET_ERROR;
	}

	printf("dns.txt  fileOffset = %ld\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 5 )
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			goto GET_ERROR;
		}	

		cmd[fileOffset] = '\0';

		j = FindSTR(cmd,"adding dns",5);		
		if( j < 0)		
			goto GET_ERROR;			
	}

	fclose(fp);

	return ALLRIGHT;

GET_ERROR:

	if( fp )
		fclose(fp);

	return ERROR;
	
}


int try_get_dns_from_file()
{
	char cmd[350];
	FILE *fp=NULL;
//	long fileOffset = 0;
	int rel;
//	int i,j;
//	int pid;
	

	fp = fopen("/mnt/mtd/dns.txt","r");
	if( fp == NULL )
	{
		printf(" can't find /mnt/mtd/dns.txt !\n");
		goto GET_ERROR1;
	}	

	fclose(fp);
	

	sprintf(cmd,"cp -rf /mnt/mtd/dns.txt  /etc/resolv.conf");	

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		goto GET_ERROR1;
	}

	sprintf(cmd,"cat  /etc/resolv.conf");	

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		goto GET_ERROR1;
	}

	return ALLRIGHT;

GET_ERROR1:
	
	return ERROR;
}

int is_connect_to_server()
{
	char ip_buf[200];
	memset(ip_buf,0,200);

	DPRINTK("p2p_server_ip=%s",p2p_server_ip);
	
	get_dns_url_ip(p2p_server_ip,ip_buf);	

	if( ip_buf[0] == 0 )
		return -1;
	else
		return 1;
	
}


int  p2p_client_to_svr_check_thread(void * value)
{	
	SET_PTHREAD_NAME(NULL);
	int delay_time = SEND_ALIVE_TIME;
	int count = 0;
	int ret;
	int total_connet_num = 0;
	int total_failed =0;
	int day_connect = (1*60*60)/SEND_ALIVE_TIME;
	int day_count = 0;
	int my_count = 0;
	char word_array[40] = "abcdefghijklmnopqrstuvwxyz.1";
	char host_url[50] = "";
	sprintf(host_url,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c",word_array[22],word_array[22],word_array[22],word_array[26],
		word_array[5],word_array[0],word_array[1],word_array[27],word_array[27],word_array[27],word_array[26],
		word_array[2],word_array[14],word_array[12]);	// "www.fab111.com"

	strcpy(host_url,"192.168.1.99");

	DPRINTK(" pid = %d  host_url=%s\n",getpid(),host_url);
	printf(" p2p_client_to_svr_check_thread in!\n");

	

	//count = delay_time - 10;

	while( g_p2p_client.dvr_svr_socket_start_work == 1)
	{
		sleep(1);

		if( (get_dvr_authorization() == 0)  &&  (p2p_dvr_id[0] == 0 ))
		{			
			 delay_time = 15;
			 day_connect = 1;
		
			my_count++;
			if( my_count ==  5 )
				net_ddns_open_close_flag(1);
		}else
		{
			// delay_time = SEND_ALIVE_TIME;
			// day_connect = (1*60*60)/SEND_ALIVE_TIME;
		}

		
		if( net_ddns_flag == 0 )		
		{
			connected_svr = 0;
			connect_to_svr = 1;
			continue;		
		}
		
		count++;	

		if( connect_to_svr > 0 )
		{
			count = delay_time + 1;
			connect_to_svr = 0;

			day_count = day_connect + 1;
		}

		if( count > delay_time)
		{
			count = 0;
			day_count++;

			DPRINTK("connected_svr=%d day_count=%d day_connect=%d\n",
				connected_svr,day_count,day_connect);

			if( connected_svr == 1 )
			{
				if( day_count > day_connect )
				{
					connected_svr = 0;					
				}else
					continue;
			}
			

			//防止正在p2p 连接时p2p_alive 发送数据造成连接出错
		/*	if( g_p2p_client.is_p2p_connect_time == 1 )			
				g_p2p_client.is_p2p_connect_time = 0;
			else					
			{
				if( p2p_server_ip[0] != 0 )
				{
					p2p_login_lock(p2p_server_ip,SVR_LISTEN_PORT);
				}			
			}
			*/

			DPRINTK("p2p_server_ip=%s %s\n",p2p_server_ip,p2p_dvr_id);

			if( p2p_server_ip[0] == 0 )
			{
				strcpy(p2p_server_ip,host_url);
				DPRINTK("p2p_server_ip[0]=0 set p2p_server_ip=%s\n",p2p_server_ip);
			}

			if( p2p_server_ip[0] != 0 )
			{

				// 让机器获得dns.
				/*if( get_dns_flag == 0 )
				{
					ret = try_get_dns_from_file();
					if( ret > 0 )
					{
						printf(" get dns OK\n");
						get_dns_flag = 1;
					}else
					{
						printf("not get dns!\n");
						continue;
					}
				}	*/

				total_connet_num++;
				

				if( p2p_dvr_id[0] == 0 )
				{
					
					//ret = p2p_get_mac_id(p2p_server_ip,SVR_LISTEN_PORT);
					ret = p2p_get_mac_id(host_url,SVR_LISTEN_PORT);
					if( ret > 0 )
					{
						connected_svr = 1;
						connect_to_svr = 1;	
					}else
					{
						connected_svr = 0;
					}					
							
				}else
				{
					ret = p2p_login_lock(p2p_server_ip,SVR_LISTEN_PORT);
					if( ret > 0 )
					{
						if( strcmp(p2p_server_ip,host_url) == 0 )
						{
						}else
						{
						 	p2p_login_lock(host_url,SVR_LISTEN_PORT);
						}
						connected_svr = 1;
						day_count = 0;
					}else
					{
						connected_svr = 0;
						total_failed++;
					}
				}

				printf("total=%d total_failed=%d  rate=%f\n",total_connet_num,total_failed,((float)total_failed)/total_connet_num);
			}
			
			
		}		
	}	

	 printf(" p2p_client_to_svr_check_thread out!\n");

	 return 1;

}

int  start_p2p_svr_to_dvr_thread()
{
	int rel;
/*	rel = pthread_create(&g_p2p_client.p2p_client_to_svr_th,NULL,(void*)p2p_client_to_svr_thread,(void *) 0);
	if ( 0 != rel )
	{
		trace( "create p2p_client_to_svr_thread thread error\n");
		return -1;
	}


	//make sure p2p_client_to_svr_thread is runing.
	while( g_p2p_client.dvr_svr_socket_start_work == 1 )
		usleep(1000);
*/

	g_p2p_client.dvr_svr_socket_start_work = 1;

	rel = pthread_create(&g_p2p_client.p2p_client_to_svr_check_th,NULL,(void*)p2p_client_to_svr_check_thread,(void *) 0);
	if ( 0 != rel )
	{
		trace( "create p2p_client_to_svr_thread thread error\n");
		return -1;
	}

	

	return 1;
}

int stop_p2p_svr_to_dvr_thread()
{
	g_p2p_client.dvr_svr_socket_start_work = 0;

//	pthread_join(g_p2p_client.p2p_client_to_svr_th,NULL);

	pthread_join(g_p2p_client.p2p_client_to_svr_check_th,NULL);

	pthread_mutex_destroy( &p2p_mutex);

	return 1;
}


int recv_command(int client_socket)
{
	char net_date[MAX_MSG_LEN]; //receive date array.
	NETREQUEST client_request;
	int ret1;
	ret1 = socket_read_data(client_socket,net_date,MAX_MSG_LEN);
	if( ret1 < 0 )
	{		
		DPRINTK(" read date error!\n");
		return ERR_NET_DATE;
	}
	
	client_request = get_client_request(net_date);
	
	if( client_request.cmd == NET_CONNECT_TO_SVR_HOLE )
	{

		//防止P2pAlive 函数发送数据，造成 p2p 连接的数据接受出错.
		g_p2p_client.is_p2p_connect_time = 1; 
	
		if( client_request.dvrName[0] == 0 )
			ret1 = p2p_create_svr_to_dvr_hole(g_p2p_client.server_ip,client_request.port1);
		else
			ret1 = p2p_create_svr_to_dvr_hole(client_request.dvrName,client_request.port1);
		if( ret1 < 0 )		
			printf("p2p_create_svr_to_dvr_hole error!\n");	

		if(client_request.dvrName[0] != 0 )
		{			
			memset(&client_request,0,sizeof(NETREQUEST));
			client_request.cmd = NET_CREATE_HOLE_FAILED;			
			p2p_set_send_cmd(net_date,&client_request);		
			ret1 = socket_send_data(g_p2p_client.dvr_svr_socket,net_date,MAX_MSG_LEN);
			if( ret1 < 0 )
			{	
				printf(" socket_send_data error!\n");
				return ERR_NET_DATE;
			}
		}
		
	}

	if( client_request.cmd == NET_DVR_LISTEN_PC )
	{
		g_p2p_client.is_p2p_connect_time = 1; 
		ret1 = p2p_listen_to_pc(SVR_TO_DVR_PORT);
		if( ret1 < 0 )
		{
			printf("p2p_listen_to_pc error!\n");	
		}
	}

	return 1;
	
}


void close_dvr_svr_socket()
{
	if(  g_p2p_client.dvr_svr_socket_flag > 0 )
	{
		//shutdown( g_p2p_client.dvr_svr_socket, SHUT_RDWR );		
		close( g_p2p_client.dvr_svr_socket);
		g_p2p_client.dvr_svr_socket_flag = 0;
		sleep(2);
	}
}

void close_svr_dvr_socket()
{
	if(  g_p2p_client.svr_dvr_socket_flag > 0 )
	{
		//shutdown( g_p2p_client.dvr_svr_socket, SHUT_RDWR );		
		close( g_p2p_client.svr_dvr_socket);
		g_p2p_client.svr_dvr_socket_flag = 0;
		
	}
}

int p2p_listen_to_pc(int port)
{
	int listen_fd = 0;
	int * socket_fd;
	char * send_date;
	int send_num;
//	int tmp_fd;
	int ret;
	fd_set watch_set;
//	struct sockaddr_in adTemp;
//	struct  linger  linger_set;
	int sin_size;
	struct  timeval timeout;
//	NET_COMMAND net_cmd;
	NETREQUEST client_request;
	
	printf("p2p_listen_to_pc:  %d\n",port);

	socket_fd = &g_p2p_client.dvr_svr_socket;
	send_date = g_p2p_client.net_date;
	send_num = MAX_MSG_LEN;

	ret = set_p2p_client_socket(&listen_fd,port,5);
	if( ret < 0 )
		return ret;

	if ( -1 == (listen(listen_fd,1)))
	{		
		DPRINTK( " socket listen error\n" );		
		goto p2p_listen_to_pc_error;
	}

	sin_size=sizeof(struct sockaddr_in);

	FD_ZERO( &watch_set);
	FD_SET( listen_fd, &watch_set ) ;

	memset(&client_request,0,sizeof(NETREQUEST));
	client_request.cmd = NET_DVR_LISTEN_PC;	

	p2p_set_send_cmd(send_date,&client_request);

	ret = socket_send_data(*socket_fd,send_date,send_num);
	if( ret < 0 )
	{			
		goto p2p_listen_to_pc_error;
	}
	

	timeout.tv_sec = LISTEN_TIMEOUT;  // 10 second for timeout
	timeout.tv_usec = 1000;	
	ret  = select( listen_fd+1, 
		&watch_set, NULL, NULL, &timeout );
	
	if ( ret <= 0 )
	{
		DPRINTK( "listen timeout!\n" );
		goto p2p_listen_to_pc_error;
	}

/*	if ( ret > 0 && FD_ISSET( listen_fd, &watch_set) )
	{
		if ( -1 == (tmp_fd = accept( listen_fd,
					(struct sockaddr *)&adTemp, &sin_size)))
		{
					DPRINTK( "DVR_NET_Initialize accept error 2\n" );
					goto p2p_listen_to_pc_error;
		}
	
		//已经达到最大用户数
		if( is_client_max_num() < 0 ) 
		{				
			memset( &net_cmd, 0, sizeof(NET_COMMAND));
			net_cmd.command =USER_TOO_MUCH;
			socket_send_data(tmp_fd, (char*)&net_cmd, 
				sizeof(NET_COMMAND));
			usleep(500000);
			close( tmp_fd );
			printf("cut %s come\n",inet_ntoa(adTemp.sin_addr));
				
		}else
		{
			//接纳用户
			insert_net_user(tmp_fd,adTemp);			
		}
	}*/

	DPRINTK( "listen over!\n" );

	if( listen_fd )
		close(listen_fd);

	return 1;
	
p2p_listen_to_pc_error:

	if( listen_fd )
		close(listen_fd);

	memset(&client_request,0,sizeof(NETREQUEST));
	client_request.cmd = NET_DVR_LISTEN_PC_ERROR;	

	p2p_set_send_cmd(send_date,&client_request);

	ret = socket_send_data(*socket_fd,send_date,send_num);
	

	return -1;		
}




void get_host_name(char * pServerIp,char * buf)
{
	int a;
	struct hostent *myhost;
	char ** pp;
	struct in_addr addr; 
	int is_url = 0;
	
	for(a = 0; a < 8;a++)
	{
		if( pServerIp[a] == (char)NULL )
			break;

		if( (pServerIp[a] < '0' || pServerIp[a] > '9') 
					&& (pServerIp[a] != '.')  )
		{
			is_url = 1;
			
			 printf("p2p pServerIp=%s\n",pServerIp);
			//不知道什么原因解析不了域名。
			myhost = gethostbyname(pServerIp); 

			if( myhost == NULL )
			{
				printf("gethostbyname can't get %s ip\n",pServerIp);
				//herror(pServerIp);
			/*
				printf("use ping to get ip\n");
				get_host_name_by_ping(pServerIp,buf);
				if( buf[0] == 0 )
					printf("get_host_name_by_ping also can't get %s ip!\n",pServerIp);
			*/
				 break;
			}
			 
			printf("host name is %s\n",myhost->h_name);		 
		 
			//for (pp = myhost->h_aliases;*pp!=NULL;pp++)
			//	printf("%02X is %s\n",*pp,*pp);
			
			pp = myhost->h_addr_list;
			while(*pp!=NULL)
			{
				addr.s_addr = *((unsigned int *)*pp);
				printf("address is %s\n",inet_ntoa(addr));
				sprintf(buf,"%s",inet_ntoa(addr));
				pp++;
			}
			
			break;
		}
	}

	if( is_url == 0 )
	{
		strcpy(buf,pServerIp);

		DPRINTK("is not url ,set ip buf %s\n",buf);
	}
}

void kill_ping()
{
	char cmd[200];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
//	int i,j;
	int pid;

	sprintf(cmd,"ps | grep ping|grep -v grep > /tmp/ping.x");

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		return ;
	}

	fp = fopen("/tmp/ping.x","r");
	if( fp == NULL )
	{
		printf(" open /tmp/ping.x error!\n");
		return ;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		return ;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		return ;
	}

	printf(" fileOffset = %ld\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 5 )
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!\n");
			fclose(fp);
			return ;
		}	

		cmd[5] = '\0';

		pid = atoi(cmd);		

		sprintf(cmd,"kill -9 %d",pid);

		printf("cmd: %s\n",cmd);

		rel = command_drv(cmd);
		if( rel < 0 )
		{
			printf("%s error!\n",cmd);
			fclose(fp);
			return ;
		}
	}

	fclose(fp);
	
}

void get_host_name_by_ping(char * pServerIp,char * buf)
{
	char cmd[350];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
	int i,j;
//	int pid;

	sprintf(cmd,"ping -c 1 -q %s -> /tmp/url.x &",pServerIp);	

	//printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		goto GET_ERROR;
	}

	//等待3秒钟
	sleep(3);

	//kill_ping();

	fp = fopen("/tmp/url.x","r");
	if( fp == NULL )
	{
		printf(" open  /tmp/url.x error!\n");
		goto GET_ERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto GET_ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto GET_ERROR;
	}

	//printf(" fileOffset = %d\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 5 )
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			goto GET_ERROR;
		}	

		cmd[fileOffset] = '\0';

		j = FindSTR(cmd,")",5);		
		if( j < 0)		
			goto GET_ERROR;

		i = FindSTR(cmd,"(",5);		
		if( i < 0)		
			goto GET_ERROR;

		rel = GetTmpLink(cmd,buf,i,j,60);
		if( rel < 0)		
			goto GET_ERROR;		

		printf("get ip:%s %s\n",pServerIp,buf);

		
	}

	fclose(fp);

	return ;

GET_ERROR:
	buf[0] = 0;
	return  ;
}


static int read_svr_log_info(char * server_ip)
{
	char buf[200];
	FILE * fp = NULL;
	long fileOffset = 0;
	int rel;
	int i;

	memset(buf,0,200);

	fp = fopen("/mnt/mtd/svr.txt","r");
	if( fp == NULL )
	{
		printf(" open /mnt/mtd/svr.txt error!\n");
		goto read_svr_faild;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto read_svr_faild;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto read_svr_faild;
	}

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 5 )
	{	
		rel = fread(buf,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!\n");
			goto read_svr_faild;
		}	

		buf[fileOffset] = '\0';	
	}else
	{
		goto read_svr_faild;
	}

	fclose(fp);

	for(i = 5; i < fileOffset ; i++)
	{
		if( (buf[i] == 0x0a) || (buf[i] == 0x0d) )
		{
			buf[i] = 0;
		}
	}

	strcpy(server_ip,buf);

	return 1;


read_svr_faild:
	if( fp )
		fclose(fp);

	server_ip[0] = 0;
	
	return -1;
}


static int read_dvr_id_log_info(char * dvr_id)
{
	char buf[1024];
	FILE * fp = NULL;
	long fileOffset = 0;
	int rel;

	memset(buf,0,200);

	fp = fopen("/mnt/mtd/dvr.db","r");
	if( fp == NULL )
	{
		printf(" open /mnt/mtd/dvr.db error!\n");
		goto read_id_faild;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto read_id_faild;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto read_id_faild;
	}

	DPRINTK("fileOffset = %d\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 1023 )
	{	
		rel = fread(buf,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!\n");
			goto read_id_faild;
		}	

		//buf[fileOffset] = '\0';	
	}else
	{
		goto read_id_faild;
	}

	fclose(fp);

	{
		int i;

		for( i = 0; i < 20; i++ )
		{
			dvr_id[i] = buf[100 + i*3];

			if( dvr_id[i] == 0 )
			{
				DPRINTK("Get Dvr Id %s\n",dvr_id);
				break;
			}
		}

		if( i >= 20 )
			goto read_id_faild;
		
	}

	//授权信息。
	if( buf[1010] == DVR_AUTHOR_INFO )
	{
		dvr_author_flag = 1;
	}else
	{
		dvr_author_flag = 0;
	}

	

	//strcpy(dvr_id,buf);

	return 1;


read_id_faild:
	if( fp )
		fclose(fp);

	dvr_id[0] = 0;
	
	return -1;
}



static int read_dvr_ver_info(char * dvr_id)
{
	char buf[1024];
	FILE * fp = NULL;
	long fileOffset = 0;
	int rel;

	dvr_id[0] = 0;
	memset(buf,0,200);

	fp = fopen("/mnt/mtd/version.txt","r");
	if( fp == NULL )
	{
		printf(" open /mnt/mtd/version.txt error!\n");
		goto read_id_faild;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto read_id_faild;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto read_id_faild;
	}

	DPRINTK("fileOffset = %d\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 2 )
	{	
		rel = fread(buf,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!\n");
			goto read_id_faild;
		}	

		buf[fileOffset+1] = 0;	
	}else
	{
		goto read_id_faild;
	}

	fclose(fp);	

	strcpy(dvr_id,buf);

	return 1;


read_id_faild:
	if( fp )
		fclose(fp);

	dvr_id[0] = 0;
	
	return -1;
}


int get_secret_random()
{
	int n;
	static int first = 0;

	if( first == 0 )
	{
		init_random(); 
		first = 1;
	}
	
  	n = rand();

	return n%128;
}

int write_dvr_id_log_info(char * dvr_id)
{
	char buf[1024];
	FILE * fp = NULL;
//	long fileOffset = 0;
	int rel;
	int i;
	int msg_strlen = 0;

	memset(buf,0,1024);

	for(i = 0;i < 1024;i++)
	{
		buf[i] = get_secret_random();
	}

	msg_strlen = strlen(dvr_id);

	for( i = 0; i < msg_strlen; i++ )
	{
		buf[100 + i*3] = dvr_id[i];
	}

	buf[100 + i*3] = 0;

	//授权信息。
	buf[1010] = DVR_AUTHOR_INFO;	

	dvr_author_flag = 1;	

	fp = fopen("/mnt/mtd/dvr.db","wb");
	if( fp == NULL )
	{
		printf(" open /mnt/mtd/dvr.db error!\n");
		goto write_id_faild;
	}

	rel = fwrite(buf,1,sizeof(buf),fp);
	
	if( rel != sizeof(buf) )
	{
		printf("fwrite error!!\n");
		goto write_id_faild;
	}

	fclose(fp);

	Net_cam_mtd_store();

	return 1;


write_id_faild:
	if( fp )
		fclose(fp);
	
	return -1;

}


int write_dvr_not_authorization_info(int flag)
{

	char buf[1024];
	FILE * fp = NULL;
	long fileOffset = 0;
	int rel;

	memset(buf,0,1024);


	if( dvr_author_flag == 0 )
	{
		DPRINTK("Dvr is already not authorization!\n");
		return 1;
	}

	fp = fopen("/mnt/mtd/dvr.db","r");
	if( fp == NULL )
	{
		printf(" open /mnt/mtd/dvr.db error!\n");
		goto write_error;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto write_error;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto write_error;
	}

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 1023 )
	{	
		rel = fread(buf,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!\n");
			goto write_error;
		}	

		
	}else
	{
		goto write_error;
	}

	fclose(fp);

	//授权信息。
	buf[1010] = DVR_AUTHOR_INFO-1;
	dvr_author_flag = 0;	


	fp = fopen("/mnt/mtd/dvr.db","wb");
	if( fp == NULL )
	{
		printf(" open /mnt/mtd/dvr.db error!\n");
		goto write_error;
	}

	rel = fwrite(buf,1,sizeof(buf),fp);
	
	if( rel != sizeof(buf) )
	{
		printf("fwrite error!!\n");
		goto write_error;
	}

	fclose(fp);	

	Net_cam_mtd_store();

	command_drv("reboot");


	return 1;


write_error:
	if( fp )
		fclose(fp);
	
	return -1;

}

int write_svr_log_info(char * server_ip)
{
	char buf[200];
	FILE * fp = NULL;
//	long fileOffset = 0;
	int rel;

	memset(buf,0,200);

	fp = fopen("/mnt/mtd/svr.txt","wb");
	if( fp == NULL )
	{
		printf(" open /mnt/mtd/svr.txt error!\n");
		goto write_id_faild;
	}

	rel = fwrite(server_ip,1,strlen(server_ip)+1,fp);
	
	if( rel != (strlen(server_ip)+1) )
	{
		printf("fwrite error!!\n");
		goto write_id_faild;
	}

	fclose(fp);

	return 1;


write_id_faild:
	if( fp )
		fclose(fp);
	
	return -1;

}

int get_new_mac_id()
{
	write_dvr_id_log_info("");
	
	p2p_dvr_id[0] = 0;
	
	login_to_svr();

	return 1;
}

void get_svr_mac_id(char * server_ip,char * mac_id )
{
	if( server_ip == NULL || mac_id == NULL )
		return;
	strcpy(server_ip,p2p_server_ip);
	strcpy(mac_id,p2p_dvr_id);
}

int get_dvr_authorization()
{
	return dvr_author_flag;
}

int is_conneted_svr()
{
	return connected_svr;
}

void set_watchdog_func()
{
	FILE * fp = NULL;
      int fd;
      int sec = 10;
      char wd = 0;
      int check_time = 0;  
      int ret;

	if( hvr_net_get_listen_thread_work() != 1 )
	{
		DPRINTK("listen thread not work\n");
		return ;
	}

	fp = fopen("/ramdisk/watchdog","r+b");
	if( fp == NULL )
	{
		//DPRINTK("can't  fopen watchdog!\n");
		return;
	}

	
	fseek( fp , 0 , SEEK_SET );
	
	wd = 1;		
	ret = fwrite(&wd,1,1,fp);
	if( ret != 1 )
	{			
		DPRINTK("fwrite watchdog error!\n");			
	}		

	fclose(fp);
	fp = NULL;
		
}


extern char net_dev_name[50];

#define IP_NOT_SAME_TIME_COUNT (60)

int upd_boradcast()
{
	char * host_name = "255.255.255.255";
	char buf[256]="IamIpCam";
	char * pVersion = NULL;
	int so_broadcast;
	char server_ip[100];
	char mac_id[100];
	int mac_hex_id = 0;
	int * pmac_hex_id = NULL;
	ValNetStatic2 * pnet_config = NULL;
	struct sockaddr_in serv_addr;
	int ip_not_same_count = IP_NOT_SAME_TIME_COUNT;

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd == -1) 
	{ 
		perror("Error: socket"); 
		return -1; 
	} 

	get_svr_mac_id(server_ip,mac_id);

	DPRINTK("mac_id = %s\n",mac_id);

	if( mac_id[0] != NULL )
	{
		int i = 0,j = 0;

		for( i = 0; i < strlen(mac_id); i++)
		{
			if( mac_id[i] <= '9' && mac_id[i] >= '0' )
			{
				server_ip[j] = mac_id[i];
				j++;
			}
		}

		server_ip[j] = '\0';
		mac_hex_id = atoi(server_ip);		
	}

	DPRINTK("mac_hex_id = %d\n",mac_hex_id);

	ipcam_set_hex_id(mac_hex_id);
	
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(BORADCAST_PORT); 
	serv_addr.sin_addr.s_addr = inet_addr(host_name); 
	so_broadcast = 1;
	
	if( setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &so_broadcast,sizeof(so_broadcast)) < 0 )
	{
		DPRINTK("SO_BROADCAST ERROR!");
		return -1;
	}

	pmac_hex_id = (int *)&buf[NET_CAM_ID_INFO];
	pnet_config = (ValNetStatic2 *)&buf[NET_CAM_NET_CONFIG];	

	
	while( 1 )
	{			
		buf[CONNECT_POS] = (char)Net_dvr_have_client();
		buf[AUTHORIZATION_POS] = (char)get_dvr_authorization();
		buf[CAM_VIDEO_TYPE] = Net_cam_get_support_max_video() ;	
		buf[NET_CAM_HARDWARE_TYPE] = NET_CAM_HISI_3516E_CHIP;
		buf[NET_USE_ID_CONNECT] = 0;
		pVersion = &buf[NET_CAM_VERSION];
		strcpy(pVersion,p2p_soft_ver);

		if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_1080P )
		{
			if( pVersion[0] == '3' )
				pVersion[0] = '4';

			if( pVersion[0] == 'B' && pVersion[1] == '3' ) 
				 pVersion[1] = '4';			
		}

		if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_480P )
		{
			if( pVersion[0] == '3' )
				pVersion[0] = '5';

			if( pVersion[0] == 'B' && pVersion[1] == '3' ) 
				 pVersion[1] = '5';
		}

		if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_576P )
		{
			if( pVersion[0] == '3' )
				pVersion[0] = '8';

			if( pVersion[0] == 'B' && pVersion[1] == '3' ) 
				 pVersion[1] = '8';
		}

		if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_960P )
		{
			if( pVersion[0] == '3' )
				pVersion[0] = '9';

			if( pVersion[0] == 'B' && pVersion[1] == '3' ) 
				 pVersion[1] = '9';
		}

		if(1)
		{
			unsigned char host_ip[4] = {0};
			char ipcam_ip[50] = "";
			char ipcam_set_ip[50] = "";
			if( ipcam_get_host_ip(net_dev_name,host_ip) < 0 )
			{
				DPRINTK("ipcam_get_host_ip get error! net_dev_name=%s\n",net_dev_name);				
				sleep(2);
				continue;
			}else
			{

				buf[NET_CAM_HAVE_MAC] = 1;
				
				Nvr_get_host_ip(ipcam_set_ip);

				sprintf(ipcam_ip,"%d.%d.%d.%d",host_ip[0],host_ip[1],host_ip[2],host_ip[3]);				
				
				//if( strcmp(ipcam_set_ip,ipcam_ip) != 0 )
				if(0)
				{
					DPRINTK("set_ip:%s  ipcam_ip:%s is not same count=%d reset net!\n",ipcam_set_ip,ipcam_ip,ip_not_same_count);

					reset_net_ip();
	
					sleep(2);		
					ip_not_same_count--;

					if( ip_not_same_count < 0 )
					{
						IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
						char tmp[256];

						//保护机器不设置到一些不可用的IP上。
						
						if( strcmp(net_dev_name,"ra0") == 0 )
						{
							DPRINTK("dev =%s IP is not right.Not change!\n",net_dev_name);
							sprintf(tmp,"echo \"dev=%s ip=%s->%s\" > /mnt/mtd/ip_error.txt",\
								net_dev_name,ipcam_set_ip,ipcam_ip);
							command_process(tmp);
							ipcam_fast_reboot();
						}else
						{
							DPRINTK("dev =%s IP is not right,so change to default ip\n",net_dev_name);
							sprintf(tmp,"echo \"dev=%s ip=%s->%s\" > /mnt/mtd/ip_error.txt",\
								net_dev_name,ipcam_set_ip,ipcam_ip);
							command_process(tmp);
							//Hisi_get_net_init_value(0);
							//save_ipcam_config_st(pAllInfo);						
							ipcam_fast_reboot();
						}
						
					}

					enable_send_broadcast = 0;
					continue;
				}else
				{
					ip_not_same_count = IP_NOT_SAME_TIME_COUNT;

					enable_send_broadcast = 1;
				}
			}
		}

		*pmac_hex_id = mac_hex_id;			

		//SD卡模式下
		/*
		if(dvr_is_recording() == 1 )	
		{
			buf[CONNECT_POS] = 1;
		}			*/
		
		if( Net_dvr_have_client() == 1 )
			buf[CAM_SD_HAVE_PEOPLE] =  6;
		else
			buf[CAM_SD_HAVE_PEOPLE] = 5;		

		buf[NET_USE_ID_CONNECT] = 1;

		if( g_pstcommand->GSt_NET_get_ipcam_net_info!= 0 )
		{			
			g_pstcommand->GSt_NET_get_ipcam_net_info(pnet_config);
		}

		{				
			int i;
			IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	

			for( i = 0; i < 4; i++)
			{
				pnet_config->IpAddress[i] = pAllInfo->net_st.ipv4[i];
				pnet_config->GateWay[i] = pAllInfo->net_st.gw[i];
				pnet_config->SubNet[i] = pAllInfo->net_st.netmask[i];
				pnet_config->DDNS1[i] = pAllInfo->net_st.dns1[i];
				pnet_config->DDNS2[i] = pAllInfo->net_st.dns2[i];
			}
		}
		
		if( Hisi_is_set_net_to_251()==0  &&  ipcam_is_now_dhcp() == 0)
			sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr_in));		

		sleep(2);		

	}

	if( sockfd > 0)
	{
		close(sockfd);
		sockfd = 0;
	}		

	return 1;
}

#define SEND_TO_PC_FLAG "IPCAMTOPC"
#define START_INFO_OFFSET (12)

int upd_to_pc_boradcast()
{
	char * host_name = "255.255.255.255";
	char buf[256]="IamIpCam";
	char * pVersion = NULL;
	int so_broadcast;
	IPCAM_INFO_TO_PC st_ipcam_info;

	memset(&st_ipcam_info,0x00,sizeof(st_ipcam_info));

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd == -1) 
	{ 
		perror("Error: socket"); 
		return -1; 
	} 

	struct sockaddr_in serv_addr; 
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(BORADCAST_TO_PC_PORT); 
	serv_addr.sin_addr.s_addr = inet_addr(host_name); 
	so_broadcast = 1;
	
	if( setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &so_broadcast,sizeof(so_broadcast)) < 0 )
	{
		DPRINTK("SO_BROADCAST ERROR!");
		goto func_err;
	}

	memset(buf,0x00,sizeof(buf));
	strcpy(buf,SEND_TO_PC_FLAG);

	if(1)
	{
		char server_ip[512] = {0};
		char mac_id[512] = {0};
		get_svr_mac_id(server_ip,mac_id);

		if( server_ip[0] == 0 || mac_id[0] == 0 )
		{
			DPRINTK("Can't get udp url:  %s : %s\n",server_ip,mac_id);
			goto func_err;
		}else
		{
			sprintf(st_ipcam_info.udp_connect_url,"%s:%s",server_ip,mac_id);
			DPRINTK("Get udp url:  %s \n",st_ipcam_info.udp_connect_url);
		}		
	}	

	
	while( 1 )
	{					
		st_ipcam_info.cmd = IPCAM_SET_CAM_COMMAND_SEND;

		get_server_port(&st_ipcam_info.server_port,&st_ipcam_info.ie_port);

		memcpy(buf + START_INFO_OFFSET,&st_ipcam_info,sizeof(st_ipcam_info));			

		if( enable_send_broadcast == 1 && Hisi_is_set_net_to_251()==0 &&  ipcam_is_now_dhcp() == 0)
		sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr_in));		

		sleep(2);		

	}

	if( sockfd > 0)
	{
		close(sockfd);
		sockfd = 0;
	}		

	return 1;

func_err:
	
	if( sockfd > 0)
	{
		close(sockfd);
		sockfd = 0;
	}

	return -1;
}

void get_nvr_ver_info(char * pVersion)
{
	strcpy(pVersion,p2p_soft_ver);
}



