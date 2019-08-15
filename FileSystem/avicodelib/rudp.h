#ifndef RUDP_PROTO_H
#define	RUDP_PROTO_H

#include "rudp_type.h"

#define RUDP_VERSION	1	/* Protocol version */
#define RUDP_MAXPKTSIZE 1000	/* Number of data bytes that can sent in a packet, RUDP header not included */
#define RUDP_MAXRETRANS 10 /* Max. number of retransmissions */
#define RUDP_TIMEOUT	200	/* Timeout for the first retransmission in milliseconds */
#define RUDP_WINDOW	10	/* Max. number of unacknowledged packets that can be sent to the network*/

/* Packet types */

#define RUDP_DATA	1
#define RUDP_ACK	2
#define RUDP_SYN	4
#define RUDP_FIN	5

/*
 * Sequence numbers are 32-bit integers operated on with modular arithmetic.
 * These macros can be used to compare sequence numbers.
 */

#define	SEQ_LT(a,b)	((short)((a)-(b)) < 0)
#define	SEQ_LEQ(a,b)	((short)((a)-(b)) <= 0)
#define	SEQ_GT(a,b)	((short)((a)-(b)) > 0)
#define	SEQ_GEQ(a,b)	((short)((a)-(b)) >= 0)

/* RUDP packet header */

struct rudp_hdr {
  u_int32_t startno;
  u_int16_t type;
  u_int16_t pack_num_once;
  u_int16_t seqno;
}__attribute__ ((packed));


struct rudp_packet {
  struct rudp_hdr header;
  int payload_length;
  char payload[RUDP_MAXPKTSIZE];
};

#endif /* RUDP_PROTO_H */
