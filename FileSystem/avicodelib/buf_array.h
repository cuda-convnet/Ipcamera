//////////////////////////////////////////////////////////////////////

#ifndef _BUF_ARRAY_H_
#define _BUF_ARRAY_H_


#include <pthread.h>

#define BUFFSIZE 1024 * 1024

#define min(x, y) ((x) < (y) ? (x) : (y))


typedef struct cycle_buffer {
        unsigned char *buf; /* store data */
        unsigned int size; /* cycle buffer size */
        unsigned int in; /* next write position in buffer */
        unsigned int out; /* next read position in buffer */
		int item_num;		
		int save_num;
        pthread_mutex_t lock;
		pthread_mutex_t read_lock;
		int total_write;
		int total_read;
		int max_write_buf;
}CYCLE_BUFFER;

unsigned int fifo_len(CYCLE_BUFFER * fifo);

#endif // !defined(_BUF_ARRAY_H_)
