 #include <sys/types.h>
 
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 
 #include <sys/ioctl.h>
 #include <net/if.h>
 #include <netinet/if_ether.h>
 #include <net/if_arp.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <errno.h>
 #include <ctype.h>
 #include <fcntl.h>
 #include <string.h>
 #include <unistd.h>
 
 #define DEV_NAME "eth0"
 //#define SIOCDARP    0x8953        /* delete ARP table entry    */
 //#define SIOCGARP    0x8954        /* get ARP table entry        */
 //#define SIOCSARP    0x8955        /* set ARP table entry        */
 
 static int sd;
 
 /* Delete an entry from the ARP cache. */
 static int arp_del(char *ip)
 {
     struct arpreq arpreq;
     struct sockaddr_in *sin;
     struct in_addr ina;
     int rc;
 
     printf("del arp entry for IP : %s\n", ip);
 
     /*you must add this becasue some system will return "Invlid argument"
        because some argument isn't zero */
     memset(&arpreq, 0, sizeof(struct arpreq));
 
     sin = (struct sockaddr_in *) &arpreq.arp_pa;
     memset(sin, 0, sizeof(struct sockaddr_in));
     sin->sin_family = AF_INET;
     ina.s_addr = inet_addr(ip);
     memcpy(&sin->sin_addr, (char *) &ina, sizeof(struct in_addr));
 
     strcpy(arpreq.arp_dev, DEV_NAME);
     rc = ioctl(sd, SIOCDARP, &arpreq);
     if (rc < 0)
     {
         printf("%s\n", "del arp error...");
         return -1;
     } else
         printf("%s\n", "del arp success...");
 
     return 0;
 }
 
 void get_hw_addr(u_char * buf, char *str)
 {
     int i;
     unsigned int p[6];
 
     i = sscanf(str, "%x:%x:%x:%x:%x:%x", &p[0], &p[1], &p[2],
                &p[3], &p[4], &p[5]);
 
     if (i != 6)
     {
         printf("%s\n", "error parsing MAC");
         exit(1);
     }
 
     for (i = 0; i < 6; i++)
         buf[i] = p[i];
 }
 
 /* Set an entry in the ARP cache. */
 static int arp_set(char *ip, char *mac)
 {
     struct arpreq arpreq;
     struct sockaddr_in *sin;
     struct in_addr ina;
     int flags;
     int rc;
 
     printf("set arp entry for IP:%s\tMAC:%s\n", ip, mac);
 
     /*you must add this becasue some system will return "Invlid argument"
        because some argument isn't zero */
     memset(&arpreq, 0, sizeof(struct arpreq));
     sin = (struct sockaddr_in *) &arpreq.arp_pa;
     memset(sin, 0, sizeof(struct sockaddr_in));
     sin->sin_family = AF_INET;
     ina.s_addr = inet_addr(ip);
     memcpy(&sin->sin_addr, (char *) &ina, sizeof(struct in_addr));
 
     get_hw_addr((unsigned char *) arpreq.arp_ha.sa_data, mac);
     strcpy(arpreq.arp_dev, DEV_NAME);
 
     flags = ATF_PERM | ATF_COM; //note, must set flag, if not,you will get error
 
     arpreq.arp_flags = flags;
 
     rc = ioctl(sd, SIOCSARP, &arpreq);
     if (rc < 0)
     {
         printf("%s\n", "set arp error...");
         return -1;
     } else
         printf("%s\n", "set arp successfully");
 
     return 0;
 }
 
 static int arp_get(char *ip)
 {
     struct arpreq arpreq;
     struct sockaddr_in *sin;
     struct in_addr ina;
     unsigned char *hw_addr;
     int rc;
 
     printf("Find arp entry for IP : %s\n", ip);
 
     /*you must add this becasue some system will return "Invlid argument"
        because some argument isn't zero */
 
     memset(&arpreq, 0, sizeof(struct arpreq));
 
     sin = (struct sockaddr_in *) &arpreq.arp_pa;
     memset(sin, 0, sizeof(struct sockaddr_in));
     sin->sin_family = AF_INET;
     ina.s_addr = inet_addr(ip);
     memcpy(&sin->sin_addr, (char *) &ina, sizeof(struct in_addr));
 
     strcpy(arpreq.arp_dev, DEV_NAME);
     rc = ioctl(sd, SIOCGARP, &arpreq);
     if (rc < 0)
     {
         printf("%s\n", "Entry not available in cache...");
         return -1;
     } else
     {
         printf("%s\n", "entry has been successfully retreived");
         hw_addr = (unsigned char *) arpreq.arp_ha.sa_data;
 
         printf("HWAddr found : %02x:%02x:%02x:%02x:%02x:%02x\n",
                hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4],
                hw_addr[5]);
     }
     return 0;
 }
 
 int main(int argc, char **argv)
 {
     sd = socket(AF_INET, SOCK_DGRAM, 0);
     if (sd < 0)
     {
         perror("socket() error\n");
         exit(1);
     }
 
     if (strcmp(argv[1], "add") == 0)
         arp_set(argv[2], argv[3]);
     else if (strcmp(argv[1], "del") == 0)
         arp_del(argv[2]);
     else if (strcmp(argv[1], "get") == 0)
         arp_get(argv[2]);
 
     close(sd);
     return 0;
 }









































































































































































