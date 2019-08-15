/******************************************************************************

   Copyright (C), 2007-2008, hangzhou, zju, 306Room.

  ******************************************************************************
   File Name : checkIP.c
   Version : Initial Draft
   Author : qixiangbing
   Created : 2008/7/2
   Last Modified :
   Description : check the ip whether conflict
   Function List :
   arpping
   check_ip
   main
   read_interface
   History :
   1.Date : 2008/7/2
   Author : qixiangbing
   Modification: Created file

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include "checkip.h"

#define LOG_EMERG "EMERGENCY!"
#define LOG_ALERT "ALERT!"
#define LOG_CRIT "critical!"
#define LOG_WARNING "warning"
#define LOG_ERR "error"
#define LOG_INFO "info"
#define LOG_DEBUG "debug"
#define LOG(level, str, args...) do { printf("%s, ", level); \
 printf(str, ## args); \
 printf("\n"); } while(0)
#define DEBUG(level, str, args...) LOG(level, str, ## args)

/* miscellaneous defines */
#define MAC_BCAST_ADDR (unsigned char *) "\xff\xff\xff\xff\xff\xff"
#define ETH_INTERFACE "eth0"
/* globals */


int read_interface(char *interface, int *ifindex, u_int32_t *addr, unsigned char *arp)
{
 int fd = -1;
 struct ifreq ifr;
 struct sockaddr_in *our_ip;

 memset(&ifr, 0, sizeof(struct ifreq));
 if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) 
 {
	 ifr.ifr_addr.sa_family = AF_INET;
	 strcpy(ifr.ifr_name, interface);

	 /*this is not execute*/
	 if (addr) 
	 {  
		 if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) 
		 {
			 our_ip = (struct sockaddr_in *) &ifr.ifr_addr;
			 *addr = our_ip->sin_addr.s_addr;
			// DEBUG(LOG_INFO, "%s (our ip) = %s", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
		 } else {
			 LOG(LOG_ERR, "SIOCGIFADDR failed, is the interface up and configured?: %s",  
			 strerror(errno));
			 goto err;
		 }
	 }

	 if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) 
	 {
		// DEBUG(LOG_INFO, "adapter index %d", ifr.ifr_ifindex);
		 *ifindex = ifr.ifr_ifindex;
	 } else {
		 LOG(LOG_ERR, "SIOCGIFINDEX failed!: %s", strerror(errno));
		  goto err;
	 }
	 if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
	 {
		 memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
		 //DEBUG(LOG_INFO, "adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x",
		 //arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
	 } else {
		 LOG(LOG_ERR, "SIOCGIFHWADDR failed!: %s", strerror(errno));
		goto err;
	 }
 }
 else 
 {
 	LOG(LOG_ERR, "socket failed!: %s", strerror(errno));
 	goto err;
 }
 close(fd);
 	return 0;
err:
	if( fd >= 0 )
		 close(fd);
	return -1;
}

/* args: yiaddr - what IP to ping
  * ip - our ip
  * mac - our arp address
  * interface - interface to use
  * retn: 1 addr free
  * 0 addr used
  * -1 error  
  */   

/* FIXME: match response against chaddr */
int arpping(u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interface)
{

 int timeout = 2;
 int optval = 1;
 int s; /* socket */
 int rv = 1; /* return value */
 struct sockaddr addr; /* for interface name */
 struct arpMsg arp;
 fd_set fdset;
 struct timeval tm;
 time_t prevTime;


 if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
 LOG(LOG_ERR, "Could not open raw socket");
 return -1;
 }

 if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
 LOG(LOG_ERR, "Could not setsocketopt on raw socket");
 close(s);
 return -1;
 }

 /* send arp request */
 memset(&arp, 0, sizeof(arp));
 memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6); /* MAC DA */
 memcpy(arp.ethhdr.h_source, mac, 6);  /* MAC SA */
 arp.ethhdr.h_proto = htons(ETH_P_ARP);  /* protocol type (Ethernet) */
 arp.htype = htons(ARPHRD_ETHER);  /* hardware type */
 arp.ptype = htons(ETH_P_IP);   /* protocol type (ARP message) */
 arp.hlen = 6;     /* hardware address length */
 arp.plen = 4;     /* protocol address length */
 arp.operation = htons(ARPOP_REQUEST);  /* ARP op code */
 //*((u_int *) arp.sInaddr) = ip;   /* source IP address */
 memcpy(((u_int *) arp.sInaddr),&ip ,sizeof(ip));
 memcpy(arp.sHaddr, mac, 6);   /* source hardware address */
 //*((u_int *) arp.tInaddr) = yiaddr;  /* target IP address */
 memcpy(((u_int *) arp.tInaddr),&yiaddr ,sizeof(yiaddr));
 
 //printf("arp.sInaddr=[%d.%d.%d.%d]\n",arp.sInaddr[0],arp.sInaddr[1],arp.sInaddr[2],arp.sInaddr[3]);
 //printf("arp.tInaddr=[%d.%d.%d.%d]\n",arp.tInaddr[0],arp.tInaddr[1],arp.tInaddr[2],arp.tInaddr[3]);

 memset(&addr, 0, sizeof(addr));
 strcpy(addr.sa_data, interface);
 if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
  rv = 0;
 
 /* wait arp reply, and check it */
 tm.tv_usec = 0;
 time(&prevTime);
 while (timeout > 0) {
  FD_ZERO(&fdset);
  FD_SET(s, &fdset);
  tm.tv_sec = timeout;
  if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0) {
   DEBUG(LOG_ERR, "Error on ARPING request: %s", strerror(errno));
   if (errno != EINTR) rv = 0;
  } else if (FD_ISSET(s, &fdset)) {
   if (recv(s, &arp, sizeof(arp), 0) < 0 ) rv = 0;
   
  //printf("arp.operation=[%d]--[%d]\n",arp.operation,htons(ARPOP_REPLY));

  // printf("arp.sInaddr=[%ld]--[%ld]\n",*((u_int *) arp.sInaddr),yiaddr); 

   if (arp.operation == htons(ARPOP_REPLY) && 
       bcmp(arp.tHaddr, mac, 6) == 0 && 
       *((u_int *) arp.sInaddr) == yiaddr) {
    DEBUG(LOG_INFO, "Valid arp reply receved for this address");
    rv = 0;
    break;
   }
  }
  timeout -= time(NULL) - prevTime;
  time(&prevTime);
 }
 close(s);
 DEBUG(LOG_INFO, "%salid arp replies for this address", rv ? "No v" : "V");  
 return rv;
}


int ipcam_get_host_ip(char * dev,unsigned char * host_ip)
{
	struct server_config_t source_config;
	struct in_addr temp;

	 if (read_interface(dev, &source_config.ifindex,&source_config.server, source_config.arp) < 0)
	 {	 	
		DEBUG(LOG_INFO, "get %s info error!\n", dev);
		return -1;
	 }		

	 memcpy(((u_int *) host_ip),&source_config.server ,sizeof(source_config.server));

	return 1;
}


int check_ip_attack(char * dev , char * ip)
{
	struct server_config_t source_config;
	struct in_addr temp;

	 if (read_interface(dev, &source_config.ifindex,&source_config.server, source_config.arp) < 0)
	 {	 	
		DEBUG(LOG_INFO, "get %s info error!\n", dev);
		return -1;
	 }		

 	if (arpping(inet_addr(ip), source_config.server, source_config.arp, dev) == 0) 
 	{
 		 temp.s_addr = inet_addr(ip);
 		 LOG(LOG_INFO, "%s belongs to someone, reserving it for %ld seconds",  
 		 inet_ntoa(temp), source_config.conflict_time);	
 		 DEBUG(LOG_INFO, "IP:%s conflict", ip);
 		 return 0;
 	} else 
 	{
 		 DEBUG(LOG_INFO, "IP:%s can use", ip);
 		return 1;
 	}	
	
}
