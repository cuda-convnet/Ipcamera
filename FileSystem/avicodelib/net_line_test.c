#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <fcntl.h>
#include  <errno.h>
#include  <sys/ioctl.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include  <linux/if.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>





typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned long long u64;

#include  <linux/sockios.h>
#include  <linux/ethtool.h>

int detect_mii(int skfd, char *ifname)
{
        struct ifreq ifr;
        u16 *data, mii_val;
        unsigned phy_id;

        /* Get the vitals from the interface. */
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0)
        {
                fprintf(stderr, "SIOCGMIIPHY on %s failed: %s£Ün", ifname,
                strerror(errno));
                (void) close(skfd);
                return 2;
        }

        data = (u16 *)(&ifr.ifr_data);
        phy_id = data[0];
        data[1] = 1;


        if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0)
        {
                fprintf(stderr, "SIOCGMIIREG on %s failed: %s£Ün", ifr.ifr_name,
                strerror(errno));
                return 2;
        }

        mii_val = data[3];

        return(((mii_val & 0x0016) == 0x0004) ? 0 : 1);
}

int detect_ethtool(int skfd, char *ifname)
{
        struct ifreq ifr;
        struct ethtool_value edata;

        memset(&ifr, 0, sizeof(ifr));
        edata.cmd = ETHTOOL_GLINK;

        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
        ifr.ifr_data = (char *) &edata;

        if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1)
        {
                printf("ETHTOOL_GLINK failed: %s£Ün", strerror(errno));
                return 2;
        }

        return (edata.data ? 0 : 1);
}

int net_line_connect(char * net_dev)
{
        int skfd = 0;
        char *ifname;
        int retval = -1;
      
        ifname = net_dev;

        /* Open a socket. */
        if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
        {
                printf("socket error£Ün"); 				
				return -1;
        }

        retval = detect_ethtool(skfd, ifname);

		printf("detect_ethtool ret=%d\n",retval);

        if (retval == 2)
                retval = detect_mii(skfd, ifname);

        close(skfd);

        if (retval == 2)
                printf("Could not determine status£Ün");

        if (retval == 1)
                printf("Link down\n");

        if (retval == 0)
                printf("Link up\n");

        return retval;
}

