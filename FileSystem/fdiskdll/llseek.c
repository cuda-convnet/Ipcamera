/*
 * llseek.c -- stub calling the llseek system call
 *
 * Copyright (C) 1994 Remy Card.  This file may be redistributed
 * under the terms of the GNU Public License.
 */

#include <sys/types.h>

#include <errno.h>
#include <unistd.h>

#if defined(__GNUC__) || defined(HAS_LONG_LONG)
typedef long long       ext2_loff_t;
#else
typedef long            ext2_loff_t;
#endif

extern ext2_loff_t ext2_llseek (unsigned int, ext2_loff_t, unsigned int);

#ifdef __linux__

#ifdef HAVE_LLSEEK
#include <syscall.h>

#else	/* HAVE_LLSEEK */

#if defined(__alpha__) || defined(__ia64__) || defined(__s390x__)

#define my_llseek lseek

#else
#include <linux/unistd.h>	/* for __NR__llseek */

static int _llseek (unsigned int, unsigned long,
		   unsigned long, ext2_loff_t *, unsigned int);

#ifdef __NR__llseek

static _syscall5(int,_llseek,unsigned int,fd,unsigned long,offset_high,
		 unsigned long, offset_low,ext2_loff_t *,result,
		 unsigned int, origin)

#else

/* no __NR__llseek on compilation machine - might give it explicitly */
static int _llseek (unsigned int fd, unsigned long oh,
		    unsigned long ol, ext2_loff_t *result,
		    unsigned int origin) {
	errno = ENOSYS;
	return -1;
}

#endif

static ext2_loff_t my_llseek (unsigned int fd, ext2_loff_t offset,
		unsigned int origin)
{
	ext2_loff_t result;
	int retval;

	retval = _llseek (fd, ((unsigned long long) offset) >> 32,
			((unsigned long long) offset) & 0xffffffff,
			&result, origin);
	return (retval == -1 ? (ext2_loff_t) retval : result);
}

#endif /* __alpha__ */

#endif	/* HAVE_LLSEEK */


ext2_loff_t ext2_llseek (unsigned int fd, ext2_loff_t offset,
			 unsigned int origin)
{
	if ((sizeof(off_t) < sizeof(ext2_loff_t)) &&
	    (offset >= ((ext2_loff_t) 1 << ((sizeof(off_t)*8) -1)))) {
		errno = EINVAL;
		return -1;
	}
	return lseek (fd, (off_t) offset, origin);
}


#else /* !linux */

ext2_loff_t ext2_llseek (unsigned int fd, ext2_loff_t offset,
			 unsigned int origin)
{
	if ((sizeof(off_t) < sizeof(ext2_loff_t)) &&
	    (offset >= ((ext2_loff_t) 1 << ((sizeof(off_t)*8) -1)))) {
		errno = EINVAL;
		return -1;
	}
	return lseek (fd, (off_t) offset, origin);
}

#endif 	/* linux */


