#ifndef RUDP_TYPE_H
#define	RUDP_TYPE_H

#ifdef _WIN32
typedef unsigned int u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char u_int8_t;
#endif

 //#ifdef USING_MINGW
  #define    timeradd(a, b, result)                       \
     do {                                                  \
         (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;     \
         (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;  \
         if ((result)->tv_usec >= 1000000)                 \
         {                                                 \
             ++(result)->tv_sec;                           \
             (result)->tv_usec -= 1000000;                 \
         }                                                 \
     } while (0)
   #define    timersub(a, b, result)                       \
     do {                                                  \
         (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;     \
         (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;  \
         if ((result)->tv_usec < 0) {                      \
             --(result)->tv_sec;                           \
            (result)->tv_usec += 1000000;                  \
         }                                                 \
     } while (0)
 //  #endif // USING_MINGW

#endif

