/* fdisk.c -- Partition table manipulator for Linux.
 *
 * Copyright (C) 1992  A. V. Le Blanc (LeBlanc@mcc.ac.uk)
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 1 or
 * (at your option) any later version.
 *
 * For detailed old history, see older versions.
 * Contributions before 2001 by faith@cs.unc.edu, Michael Bischoff,
 * LeBlanc@mcc.ac.uk, martin@cs.unc.edu, leisner@sdsp.mc.xerox.com,
 * esr@snark.thyrsus.com, aeb@cwi.nl, quinlan@yggdrasil.com,
 * fasten@cs.bonn.edu, orschaer@cip.informatik.uni-erlangen.de,
 * jj@sunsite.mff.cuni.cz, fasten@shw.com, ANeuper@GUUG.de,
 * kgw@suse.de, kalium@gmx.de, dhuggins@linuxcare.com,
 * michal@ellpspace.math.ualberta.ca and probably others.
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <setjmp.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

#include "nls.h"
#include "common.h"
#include "fdisk.h"

#include "libfdisk.h"

#include "fdisksunlabel.h"
#include "fdisksgilabel.h"
#include "fdiskaixlabel.h"


#include "defines.h"
#ifdef HAVE_blkpg_h
#include <linux/blkpg.h>
#endif

#define USE_SATA_DISK

int iExceptionError = 0;

static void delete_partition(int i);

#define hex_val(c)	({ \
				char _c = (c); \
				isdigit(_c) ? _c - '0' : \
				tolower(_c) + 10 - 'a'; \
			})


#define LINE_LENGTH	800
#define pt_offset(b, n)	((struct partition *)((b) + 0x1be + \
				(n) * sizeof(struct partition)))
#define sector(s)	((s) & 0x3f)
#define cylinder(s, c)	((c) | (((s) & 0xc0) << 2))

#define hsc2sector(h,s,c) (sector(s) - 1 + sectors * \
				((h) + heads * cylinder(s,c)))
#define set_hsc(h,s,c,sector) { \
				s = sector % sectors + 1;	\
				sector /= sectors;	\
				h = sector % heads;	\
				sector /= heads;	\
				c = sector & 0xff;	\
				s |= (sector >> 2) & 0xc0;	\
			}



/* A valid partition table sector ends in 0x55 0xaa */
static unsigned int
part_table_flag(char *b) {
	return ((uint) b[510]) + (((uint) b[511]) << 8);
}

int
valid_part_table_flag(unsigned char *b) {
	return (b[510] == 0x55 && b[511] == 0xaa);
}

static void
write_part_table_flag(char *b) {
	b[510] = 0x55;
	b[511] = 0xaa;
}

/* start_sect and nr_sects are stored little endian on all machines */
/* moreover, they are not aligned correctly */
static void
store4_little_endian(unsigned char *cp, unsigned int val) {
	cp[0] = (val & 0xff);
	cp[1] = ((val >> 8) & 0xff);
	cp[2] = ((val >> 16) & 0xff);
	cp[3] = ((val >> 24) & 0xff);
}

static unsigned int
read4_little_endian(unsigned char *cp) {
	return (uint)(cp[0]) + ((uint)(cp[1]) << 8)
		+ ((uint)(cp[2]) << 16) + ((uint)(cp[3]) << 24);
}

static void
set_start_sect(struct partition *p, unsigned int start_sect) {
	store4_little_endian(p->start4, start_sect);
}

unsigned int
get_start_sect(struct partition *p) {
	return read4_little_endian(p->start4);
}

static void
set_nr_sects(struct partition *p, unsigned int nr_sects) {
	store4_little_endian(p->size4, nr_sects);
}

unsigned int
get_nr_sects(struct partition *p) {
	return read4_little_endian(p->size4);
}

/* normally O_RDWR, -l option gives O_RDONLY */
static int type_open = O_RDWR;

/*
 * Raw disk label. For DOS-type partition tables the MBR,
 * with descriptions of the primary partitions.
 */
char MBRbuffer[MAX_SECTOR_SIZE];

/*
 * per partition table entry data
 *
 * The four primary partitions have the same sectorbuffer (MBRbuffer)
 * and have NULL ext_pointer.
 * Each logical partition table entry has two pointers, one for the
 * partition and one link to the next one.
 */
struct pte {
	struct partition *part_table;	/* points into sectorbuffer */
	struct partition *ext_pointer;	/* points into sectorbuffer */
	char changed;		/* boolean */
	uint offset;		/* disk sector number */
	char *sectorbuffer;	/* disk sector contents */
} ptes[MAXIMUM_PARTS];

int  DiskNum = 0;                      /* the number of HardDisk */

int  iCurrentDevNumber = 0;

GST_DISKINFO disk_info[MAXDEVNUM];

int iUsbNum = 0;

static pthread_mutex_t disk_op_mutex;

GST_DISKINFO usb_info;

int g_haveUsb = 0;

int deletenumber = 0;
char patitionselect;
uint  endcylinder;
int no_disk_flag = 0;

char	*disk_device,			/* must be specified */
	*line_ptr,			/* interactive input */
	line_buffer[LINE_LENGTH];

int	fd,				/* the disk */
	ext_index,			/* the prime extended partition */
	listing = 0,			/* no aborts for fdisk -l */
	nowarn = 0,			/* no warnings for fdisk -l/-s */
	dos_compatible_flag = ~0,
	dos_changed = 0,
	partitions = 4;			/* maximum partition + 1 */

uint	user_cylinders, user_heads, user_sectors;
uint	pt_heads, pt_sectors;
uint	kern_heads, kern_sectors;

uint	heads,
	sectors,
	cylinders,
	sector_size = DEFAULT_SECTOR_SIZE,
	user_set_sector_size = 0,
	sector_offset = 1,
	units_per_sector = 1,
	display_in_cyl_units = 1,
	extended_offset = 0;		/* offset of link pointers */

unsigned long total_number_of_sectors;

#define dos_label (!sun_label && !sgi_label && !aix_label && !osf_label)
int     sun_label = 0;                  /* looking at sun disklabel */
int	sgi_label = 0;			/* looking at sgi disklabel */
int	aix_label = 0;			/* looking at aix disklabel */
int	osf_label = 0;			/* looking at OSF/1 disklabel */
int	possibly_osf_label = 0;

jmp_buf listingbuf;

void fatal(enum failure why) {
	char	error[LINE_LENGTH],
		*message = error;

	if (listing) {
		close(fd);
		longjmp(listingbuf, 1);
	}

	switch (why) {
		case usage: message = (
"Usage: fdisk [-b SSZ] [-u] DISK     Change partition table\n"
"       fdisk -l [-b SSZ] [-u] DISK  List partition table(s)\n"
"       fdisk -s PARTITION           Give partition size(s) in blocks\n"
"       fdisk -v                     Give fdisk version\n"
"Here DISK is something like /dev/hdb or /dev/sda\n"
"and PARTITION is something like /dev/hda7\n"
"-u: give Start and End in sector (instead of cylinder) units\n"
"-b 2048: (for certain MO disks) use 2048-byte sectors\n");
			break;
		case usage2:
		  /* msg in cases where fdisk used to probe */
			message = (
"Usage: fdisk [-l] [-b SSZ] [-u] device\n"
"E.g.: fdisk /dev/hda  (for the first IDE disk)\n"
"  or: fdisk /dev/sdc  (for the third SCSI disk)\n"
"  or: fdisk /dev/eda  (for the first PS/2 ESDI drive)\n"
"  or: fdisk /dev/rd/c0d0  or: fdisk /dev/ida/c0d0  (for RAID devices)\n"
"  ...\n");
			break;
		case unable_to_open:
			snprintf(error, sizeof(error),
				 ("Unable to open %s\n"), disk_device);
			break;
		case unable_to_read:
			snprintf(error, sizeof(error),
				 ("Unable to read %s\n"), disk_device);
			break;
		case unable_to_seek:
			snprintf(error, sizeof(error),
				("Unable to seek on %s\n"),disk_device);
			break;
		case unable_to_write:
			snprintf(error, sizeof(error),
				("Unable to write %s\n"), disk_device);
			break;
		case ioctl_error:
			snprintf(error, sizeof(error),
				 ("BLKGETSIZE ioctl failed on %s\n"),
				disk_device);
			break;
		case out_of_memory:
			message = ("Unable to allocate any more memory\n");
			break;
		default:
			message = ("Fatal error\n");
	}

	fputc('\n', stderr);
	fputs(message, stderr);
	iExceptionError = 1;
	//exit(1);
}

static void
seek_sector(int fd, uint secno) {
	ext2_loff_t offset = (ext2_loff_t) secno * sector_size;
	if (ext2_llseek(fd, offset, SEEK_SET) == (ext2_loff_t) -1)
	{
		fatal(unable_to_seek);
		iExceptionError = 1;
	}
}

static void
read_sector(int fd, uint secno, char *buf) {
	seek_sector(fd, secno);

	if( iExceptionError == 0 )
	{
		if (read(fd, buf, sector_size) != sector_size)
			fatal(unable_to_read);
	}
}

static void
write_sector(int fd, uint secno, char *buf) {
	seek_sector(fd, secno);
	if( iExceptionError == 0 )
	{
		if (write(fd, buf, sector_size) != sector_size)
			fatal(unable_to_write);
	}
}

/* Allocate a buffer and read a partition table sector */
static void
read_pte(int fd, int pno, uint offset) {
	struct pte *pe = &ptes[pno];

	pe->offset = offset;
	pe->sectorbuffer = (char *) malloc(sector_size);
	if (!pe->sectorbuffer)
		fatal(out_of_memory);
	read_sector(fd, offset, pe->sectorbuffer);
	 if( iExceptionError == 0 )
	{
		pe->changed = 0;
		pe->part_table = pe->ext_pointer = NULL;
	 }
}

static unsigned int
get_partition_start(struct pte *pe) {
	return pe->offset + get_start_sect(pe->part_table);
}

struct partition *
get_part_table(int i) {
	return ptes[i].part_table;
}

void
set_all_unchanged(void) {
	int i;

	for (i = 0; i < MAXIMUM_PARTS; i++)
		ptes[i].changed = 0;
}

void
set_changed(int i) {
	ptes[i].changed = 1;
}

/*
 * Avoid warning about DOS partitions when no DOS partition was changed.
 * Here a heuristic "is probably dos partition".
 * We might also do the opposite and warn in all cases except
 * for "is probably nondos partition".
 */
static int
is_dos_partition(int t) {
	return (t == 1 || t == 4 || t == 6 ||
		t == 0x0b || t == 0x0c || t == 0x0e ||
		t == 0x11 || t == 0x12 || t == 0x14 || t == 0x16 ||
		t == 0x1b || t == 0x1c || t == 0x1e || t == 0x24 ||
		t == 0xc1 || t == 0xc4 || t == 0xc6);
}

static void
menu(void) {
	if (sun_label) {
	   puts(("Command action"));
	   puts(("   a   toggle a read only flag")); 		/* sun */
	   puts(("   b   edit bsd disklabel"));
	   puts(("   c   toggle the mountable flag"));		/* sun */
	   puts(("   d   delete a partition"));
	   puts(("   l   list known partition types"));
	   puts(("   m   print this menu"));
	   puts(("   n   add a new partition"));
	   puts(("   o   create a new empty DOS partition table"));
	   puts(("   p   print the partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   s   create a new empty Sun disklabel"));	/* sun */
	   puts(("   t   change a partition's system id"));
	   puts(("   u   change display/entry units"));
	   puts(("   v   verify the partition table"));
	   puts(("   w   write table to disk and exit"));
	   puts(("   x   extra functionality (experts only)"));
	}
	else if (sgi_label) {
	   puts(("Command action"));
	   puts(("   a   select bootable partition"));    /* sgi flavour */
	   puts(("   b   edit bootfile entry"));          /* sgi */
	   puts(("   c   select sgi swap partition"));    /* sgi flavour */
	   puts(("   d   delete a partition"));
	   puts(("   l   list known partition types"));
	   puts(("   m   print this menu"));
	   puts(("   n   add a new partition"));
	   puts(("   o   create a new empty DOS partition table"));
	   puts(("   p   print the partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   s   create a new empty Sun disklabel"));	/* sun */
	   puts(("   t   change a partition's system id"));
	   puts(("   u   change display/entry units"));
	   puts(("   v   verify the partition table"));
	   puts(("   w   write table to disk and exit"));
	}
	else if (aix_label) {
	   puts(("Command action"));
	   puts(("   m   print this menu"));
	   puts(("   o   create a new empty DOS partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   s   create a new empty Sun disklabel"));	/* sun */
	}
	else {
	   puts(("Command action"));
	   puts(("   a   toggle a bootable flag"));
	   puts(("   b   edit bsd disklabel"));
	   puts(("   c   toggle the dos compatibility flag"));
	   puts(("   d   delete a partition"));
	   puts(("   l   list known partition types"));
	   puts(("   m   print this menu"));
	   puts(("   n   add a new partition"));
	   puts(("   o   create a new empty DOS partition table"));
	   puts(("   p   print the partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   s   create a new empty Sun disklabel"));	/* sun */
	   puts(("   t   change a partition's system id"));
	   puts(("   u   change display/entry units"));
	   puts(("   v   verify the partition table"));
	   puts(("   w   write table to disk and exit"));
	   puts(("   x   extra functionality (experts only)"));
	}
}

static void
xmenu(void) {
	if (sun_label) {
	   puts(("Command action"));
	   puts(("   a   change number of alternate cylinders"));      /*sun*/
	   puts(("   c   change number of cylinders"));
	   puts(("   d   print the raw data in the partition table"));
	   puts(("   e   change number of extra sectors per cylinder"));/*sun*/
	   puts(("   h   change number of heads"));
	   puts(("   i   change interleave factor"));			/*sun*/
	   puts(("   o   change rotation speed (rpm)"));		/*sun*/
	   puts(("   m   print this menu"));
	   puts(("   p   print the partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   r   return to main menu"));
	   puts(("   s   change number of sectors/track"));
	   puts(("   v   verify the partition table"));
	   puts(("   w   write table to disk and exit"));
	   puts(("   y   change number of physical cylinders"));	/*sun*/
	}
	else if (sgi_label) {
	   puts(("Command action"));
	   puts(("   b   move beginning of data in a partition")); /* !sun */
	   puts(("   c   change number of cylinders"));
	   puts(("   d   print the raw data in the partition table"));
	   puts(("   e   list extended partitions"));		/* !sun */
	   puts(("   g   create an IRIX (SGI) partition table"));/* sgi */
	   puts(("   h   change number of heads"));
	   puts(("   m   print this menu"));
	   puts(("   p   print the partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   r   return to main menu"));
	   puts(("   s   change number of sectors/track"));
	   puts(("   v   verify the partition table"));
	   puts(("   w   write table to disk and exit"));
	}
	else if (aix_label) {
	   puts(("Command action"));
	   puts(("   b   move beginning of data in a partition")); /* !sun */
	   puts(("   c   change number of cylinders"));
	   puts(("   d   print the raw data in the partition table"));
	   puts(("   e   list extended partitions"));		/* !sun */
	   puts(("   g   create an IRIX (SGI) partition table"));/* sgi */
	   puts(("   h   change number of heads"));
	   puts(("   m   print this menu"));
	   puts(("   p   print the partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   r   return to main menu"));
	   puts(("   s   change number of sectors/track"));
	   puts(("   v   verify the partition table"));
	   puts(("   w   write table to disk and exit"));
	}
	else {
	   puts(("Command action"));
	   puts(("   b   move beginning of data in a partition")); /* !sun */
	   puts(("   c   change number of cylinders"));
	   puts(("   d   print the raw data in the partition table"));
	   puts(("   e   list extended partitions"));		/* !sun */
	   puts(("   f   fix partition order"));		/* !sun, !aix, !sgi */
	   puts(("   g   create an IRIX (SGI) partition table"));/* sgi */
	   puts(("   h   change number of heads"));
	   puts(("   m   print this menu"));
	   puts(("   p   print the partition table"));
	   puts(("   q   quit without saving changes"));
	   puts(("   r   return to main menu"));
	   puts(("   s   change number of sectors/track"));
	   puts(("   v   verify the partition table"));
	   puts(("   w   write table to disk and exit"));
	}
}

static int
get_sysid(int i) {
	return (
		sun_label ? sunlabel->infos[i].id :
		sgi_label ? sgi_get_sysid(i) :
		ptes[i].part_table->sys_ind);
}

static struct systypes *
get_sys_types(void) {
	return (
		sun_label ? sun_sys_types :
		sgi_label ? sgi_sys_types :
		i386_sys_types);
}

char *partition_type(unsigned char type)
{
	int i;
	struct systypes *types = get_sys_types();

	for (i=0; types[i].name; i++)
		if (types[i].type == type)
			return (types[i].name);

	return NULL;
}

void list_types(struct systypes *sys)
{
	uint last[4], done = 0, next = 0, size;
	int i;

	for (i = 0; sys[i].name; i++);
	size = i;

	for (i = 3; i >= 0; i--)
		last[3 - i] = done += (size + i - done) / (i + 1);
	i = done = 0;

	do {
		printf("%c%2x  %-15.15s", i ? ' ' : '\n',
		        sys[next].type, (sys[next].name));
 		next = last[i++] + done;
		if (i > 3 || next >= last[i]) {
			i = 0;
			next = ++done;
		}
	} while (done < last[0]);
	putchar('\n');
}

static int
is_cleared_partition(struct partition *p) {
	return !(!p || p->boot_ind || p->head || p->sector || p->cyl ||
		 p->sys_ind || p->end_head || p->end_sector || p->end_cyl ||
		 get_start_sect(p) || get_nr_sects(p));
}

static void
clear_partition(struct partition *p) {
	if (!p)
		return;
	p->boot_ind = 0;
	p->head = 0;
	p->sector = 0;
	p->cyl = 0;
	p->sys_ind = 0;
	p->end_head = 0;
	p->end_sector = 0;
	p->end_cyl = 0;
	set_start_sect(p,0);
	set_nr_sects(p,0);
}

static void
set_partition(int i, int doext, uint start, uint stop, int sysid) {
	struct partition *p;
	uint offset;

	if (doext) {
		p = ptes[i].ext_pointer;
		offset = extended_offset;
	} else {
		p = ptes[i].part_table;
		offset = ptes[i].offset;
	}
	p->boot_ind = 0;
	p->sys_ind = sysid;
	set_start_sect(p, start - offset);
	set_nr_sects(p, stop - start + 1);
	if (dos_compatible_flag && (start/(sectors*heads) > 1023))
		start = heads*sectors*1024 - 1;
	set_hsc(p->head, p->sector, p->cyl, start);
	if (dos_compatible_flag && (stop/(sectors*heads) > 1023))
		stop = heads*sectors*1024 - 1;
	set_hsc(p->end_head, p->end_sector, p->end_cyl, stop);
	ptes[i].changed = 1;
}

static int
test_c(char **m, char *mesg) {
	int val = 0;
	if (!*m)
		fprintf(stderr, ("You must set"));
	else {
		fprintf(stderr, " %s", *m);
		val = 1;
	}
	*m = mesg;
	return val;
}

static int
warn_geometry(void) {
	char *m = NULL;
	int prev = 0;
	if (!heads)
		prev = test_c(&m, ("heads"));
	if (!sectors)
		prev = test_c(&m, ("sectors"));
	if (!cylinders)
		prev = test_c(&m, ("cylinders"));
	if (!m)
		return 0;
	fprintf(stderr,
		("%s%s.\nYou can do this from the extra functions menu.\n"),
		prev ? (" and ") : " ", m);
	return 1;
}

void update_units(void)
{
	int cyl_units = heads * sectors;

	if (display_in_cyl_units && cyl_units)
		units_per_sector = cyl_units;
	else
		units_per_sector = 1; 	/* in sectors */
}

static void
warn_cylinders(void) {
	if (dos_label && cylinders > 1024 && !nowarn);
		/*fprintf(stdout, ("\n"
"The number of cylinders for this disk is set to %d.\n"
"There is nothing wrong with that, but this is larger than 1024,\n"
"and could in certain setups cause problems with:\n"
"1) software that runs at boot time (e.g., old versions of LILO)\n"
"2) booting and partitioning software from other OSs\n"
"   (e.g., DOS FDISK, OS/2 FDISK)\n"),
			cylinders); */
}

static void
read_extended(int ext) {
	int i;
	struct pte *pex;
	struct partition *p, *q;

	ext_index = ext;
	pex = &ptes[ext];
	pex->ext_pointer = pex->part_table;

	p = pex->part_table;
	if (!get_start_sect(p)) {
		fprintf(stderr,
			("Bad offset in primary extended partition\n"));
		return;
	}

	while (IS_EXTENDED (p->sys_ind)) {
		struct pte *pe = &ptes[partitions];

		if (partitions >= MAXIMUM_PARTS) {
			/* This is not a Linux restriction, but
			   this program uses arrays of size MAXIMUM_PARTS.
			   Do not try to `improve' this test. */
			struct pte *pre = &ptes[partitions-1];

			fprintf(stderr,
				("Warning: deleting partitions after %d\n"),
				partitions);
			clear_partition(pre->ext_pointer);
			pre->changed = 1;
			return;
		}

		read_pte(fd, partitions, extended_offset + get_start_sect(p));

		 if( iExceptionError != 0 )
			return;

		if (!extended_offset)
			extended_offset = get_start_sect(p);

		q = p = pt_offset(pe->sectorbuffer, 0);
		for (i = 0; i < 4; i++, p++) if (get_nr_sects(p)) {
			if (IS_EXTENDED (p->sys_ind)) {
				if (pe->ext_pointer)
					fprintf(stderr,
						("Warning: extra link "
						  "pointer in partition table"
						  " %d\n"), partitions + 1);
				else
					pe->ext_pointer = p;
			} else if (p->sys_ind) {
				if (pe->part_table)
					fprintf(stderr,
						("Warning: ignoring extra "
						  "data in partition table"
						  " %d\n"), partitions + 1);
				else
					pe->part_table = p;
			}
		}

		/* very strange code here... */
		if (!pe->part_table) {
			if (q != pe->ext_pointer)
				pe->part_table = q;
			else
				pe->part_table = q + 1;
		}
		if (!pe->ext_pointer) {
			if (q != pe->part_table)
				pe->ext_pointer = q;
			else
				pe->ext_pointer = q + 1;
		}

		p = pe->ext_pointer;
		partitions++;
	}

	/* remove empty links */
 remove:
	for (i = 4; i < partitions; i++) {
		struct pte *pe = &ptes[i];

		if (!get_nr_sects(pe->part_table) &&
		    (partitions > 5 || ptes[4].part_table->sys_ind)) {
			printf("omitting empty partition (%d)\n", i+1);
			delete_partition(i);
			goto remove; 	/* numbering changed */
		}
	}
}

static void
create_doslabel(void) {
	int i;

	fprintf(stderr,
	("Building a new DOS disklabel. Changes will remain in memory only,\n"
	  "until you decide to write them. After that, of course, the previous\n"
	  "content won't be recoverable.\n\n"));
	sun_nolabel();  /* otherwise always recognised as sun */
	sgi_nolabel();  /* otherwise always recognised as sgi */
	aix_label = osf_label = possibly_osf_label = 0;
	partitions = 4;

	for (i = 510-64; i < 510; i++)
		MBRbuffer[i] = 0;
	write_part_table_flag(MBRbuffer);
	extended_offset = 0;
	set_all_unchanged();
	set_changed(0);
	get_boot(create_empty_dos);
}

#include <sys/utsname.h>
#define MAKE_VERSION(p,q,r)     (65536*(p) + 256*(q) + (r))

static int
linux_version_code(void) {
	static int kernel_version = 0;
        struct utsname my_utsname;
        int p, q, r;

        if (!kernel_version && uname(&my_utsname) == 0) {
                p = atoi(strtok(my_utsname.release, "."));
                q = atoi(strtok(NULL, "."));
                r = atoi(strtok(NULL, "."));
                kernel_version = MAKE_VERSION(p,q,r);
        }
        return kernel_version;
}

static void
get_sectorsize(int fd) {
#if defined(BLKSSZGET)
	if (!user_set_sector_size &&
	    linux_version_code() >= MAKE_VERSION(2,3,3)) {
		int arg;
		if (ioctl(fd, BLKSSZGET, &arg) == 0)
			sector_size = arg;
		if (sector_size != DEFAULT_SECTOR_SIZE)
			printf(("Note: sector size is %d (not %d)\n"),
			       sector_size, DEFAULT_SECTOR_SIZE);
	}
#else
	/* maybe the user specified it; and otherwise we still
	   have the DEFAULT_SECTOR_SIZE default */
#endif
}

static void
get_kernel_geometry(int fd) {
#ifdef HDIO_GETGEO
	struct hd_geometry geometry;

	if (!ioctl(fd, HDIO_GETGEO, &geometry)) {
		kern_heads = geometry.heads;
		kern_sectors = geometry.sectors;
		/* never use geometry.cylinders - it is truncated */
	}
#endif
}

static void
get_partition_table_geometry(void) {
	unsigned char *bufp = MBRbuffer;
	struct partition *p;
	int i, h, s, hh, ss;
	int first = 1;
	int bad = 0;

	if (!(valid_part_table_flag(bufp)))
		return;

	hh = ss = 0;
	for (i=0; i<4; i++) {
		p = pt_offset(bufp, i);
		if (p->sys_ind != 0) {
			h = p->end_head + 1;
			s = (p->end_sector & 077);
			if (first) {
				hh = h;
				ss = s;
				first = 0;
			} else if (hh != h || ss != s)
				bad = 1;
		}
	}

	if (!first && !bad) {
		pt_heads = hh;
		pt_sectors = ss;
	}
}

void
get_geometry(int fd, struct geom *g) {
	int sec_fac;
	unsigned long longsectors;

	get_sectorsize(fd);
	sec_fac = sector_size / 512;
	guess_device_type(fd);
	heads = cylinders = sectors = 0;
	kern_heads = kern_sectors = 0;
	pt_heads = pt_sectors = 0;

	get_kernel_geometry(fd);
	get_partition_table_geometry();

	heads = user_heads ? user_heads :
		pt_heads ? pt_heads :
		kern_heads ? kern_heads : 255;
	sectors = user_sectors ? user_sectors :
		pt_sectors ? pt_sectors :
		kern_sectors ? kern_sectors : 63;

	if (ioctl(fd, BLKGETSIZE, &longsectors))
		longsectors = 0;

	sector_offset = 1;
	if (dos_compatible_flag)
		sector_offset = sectors;


	cylinders = longsectors / (heads * sectors);
	cylinders /= sec_fac;
	if (!cylinders)
		cylinders = user_cylinders;

	if (g) {
		g->heads = heads;
		g->sectors = sectors;
		g->cylinders = cylinders;
	}

	total_number_of_sectors = longsectors;


}

/*
 * Read MBR.  Returns:
 *   -1: no 0xaa55 flag present (possibly entire disk BSD)
 *    0: found or created label
 *    1: I/O error
 */
int
get_boot(enum action what) {
	int i;

	partitions = 4;

	for (i = 0; i < 4; i++) {
		struct pte *pe = &ptes[i];

		pe->part_table = pt_offset(MBRbuffer, i);
		pe->ext_pointer = NULL;
		pe->offset = 0;
		pe->sectorbuffer = MBRbuffer;
		pe->changed = (what == create_empty_dos);
	}

	if (what == create_empty_sun && check_sun_label())
		return 0;

	memset(MBRbuffer, 0, 512);

	if (what == create_empty_dos)
		goto got_dos_table;		/* skip reading disk */

	if ((fd = open(disk_device, type_open)) < 0) {
	    if ((fd = open(disk_device, O_RDONLY)) < 0) {
		if (what == try_only)
		    return 1;
		fatal(unable_to_open);
	    } else
		printf(("You will not be able to write "
			 "the partition table.\n"));
	}

	if (512 != read(fd, MBRbuffer, 512)) {
		if (what == try_only)
			return 1;
		fatal(unable_to_read);
	}

	get_geometry(fd, NULL);

	update_units();

	if (check_sun_label())
		return 0;

	if (check_sgi_label())
		return 0;

	if (check_aix_label())
		return 0;

	if (check_osf_label()) {
		possibly_osf_label = 1;
		if (!valid_part_table_flag(MBRbuffer)) {
			osf_label = 1;
			return 0;
		}
		printf(("This disk has both DOS and BSD magic.\n"
			 "Give the 'b' command to go to BSD mode.\n"));
	}

got_dos_table:

	if (!valid_part_table_flag(MBRbuffer)) {
		switch(what) {
		case fdisk:
			fprintf(stderr,
				("Device contains neither a valid DOS "
				  "partition table, nor Sun, SGI or OSF "
				  "disklabel\n"));
#ifdef __sparc__
			create_sunlabel();
#else
			create_doslabel();
#endif
			return 0;
		case require:
			return -1;
		case try_only:
		        return -1;
		case create_empty_dos:
		case create_empty_sun:
			break;
		default:
			fprintf(stderr, ("Internal error\n"));
			exit(1);
		}
	}

	warn_cylinders();
	warn_geometry();

	for (i = 0; i < 4; i++) {
		struct pte *pe = &ptes[i];

		if (IS_EXTENDED (pe->part_table->sys_ind)) {
			if (partitions != 4)
				fprintf(stderr, ("Ignoring extra extended "
					"partition %d\n"), i + 1);
			else
			{
				read_extended(i);
				if( iExceptionError == 0 )
				{
					return 0;
				}
			}
		}
	}

	for (i = 3; i < partitions; i++) {
		struct pte *pe = &ptes[i];

		if (!valid_part_table_flag(pe->sectorbuffer)) {
			fprintf(stderr,
				("Warning: invalid flag 0x%04x of partition "
				"table %d will be corrected by w(rite)\n"),
				part_table_flag(pe->sectorbuffer), i + 1);
			pe->changed = 1;
		}
	}

	return 0;
}

/* read line; return 0 or first char */
int
read_line(void)
{
	static int got_eof = 0;

	line_ptr = line_buffer;
	if (!fgets(line_buffer, LINE_LENGTH, stdin)) {
		if (feof(stdin))
			got_eof++; 	/* user typed ^D ? */
		if (got_eof >= 3) {
			fflush(stdout);
			fprintf(stderr, ("\ngot EOF thrice - exiting..\n"));
			exit(1);
		}
		return 0;
	}
	while (*line_ptr && !isgraph(*line_ptr))
		line_ptr++;
	return *line_ptr;
}

char
read_char(char *mesg)
{
	do {
		fputs(mesg, stdout);
		fflush (stdout);	 /* requested by niles@scyld.com */
	} while (!read_line());
	return *line_ptr;
}

char
read_chars(char *mesg)
{
	int i = 0, j =0;
	
	if( deletenumber == 0 )
	{
	       fputs(mesg, stdout);
		fflush (stdout);	/* niles@scyld.com */
	
	if (!read_line()) {
		*line_ptr = '\n';
		line_ptr[1] = 0;
		}
	}
	else
	{ 
		line_ptr = line_buffer;

		if( deletenumber >= 10 )
		{	
			i = deletenumber /10;

			j = deletenumber % 10;
		
			*line_ptr = (char)(i + 48); // number change to ascii char.
			line_ptr[1] =  (char)(j + 48);
			line_ptr[2] = 0;
		}
		else
		{
			*line_ptr = (char)(deletenumber + 48); // number change to ascii char.
			line_ptr[1] = 0;
		}
	}
	return *line_ptr;
}

int
read_hex(struct systypes *sys)
{
        int hex;

        while (1)
        {
           read_char(("Hex code (type L to list codes): "));
           if (tolower(*line_ptr) == 'l')
               list_types(sys);
	   else if (isxdigit (*line_ptr))
	   {
	      hex = 0;
	      do
		 hex = hex << 4 | hex_val(*line_ptr++);
	      while (isxdigit(*line_ptr));
	      return hex;
	   }
        }
}

/*
 * Print the message MESG, then read an integer between LOW and HIGH (inclusive).
 * If the user hits Enter, DFLT is returned.
 * Answers like +10 are interpreted as offsets from BASE.
 *
 * There is no default if DFLT is not between LOW and HIGH.
 */
uint
read_int(uint low, uint dflt, uint high, uint base, char *mesg)
{
	uint i;
	int default_ok = 1;
	static char *ms = NULL;
	static int mslen = 0;

	

	if (!ms || strlen(mesg)+100 > mslen) {
		mslen = strlen(mesg)+200;
		if (!(ms = realloc(ms,mslen)))
			fatal(out_of_memory);
	}

	

	if (dflt < low || dflt > high)
		default_ok = 0;

	if (default_ok)
		snprintf(ms, mslen, ("%s (%d-%d, default %d): "),
			 mesg, low, high, dflt);
	else
		snprintf(ms, mslen, "%s (%d-%d): ",
			 mesg, low, high);

	
	while (1) {
		int use_default = default_ok;

	
		/* ask question and read answer */
		while (read_chars(ms) != '\n' && !isdigit(*line_ptr)
		       && *line_ptr != '-' && *line_ptr != '+')
			continue;

		if (*line_ptr == '+' || *line_ptr == '-') {
			
			int minus = (*line_ptr == '-');
			int  absolute = 0;

			i = atoi(line_ptr+1);

 			while (isdigit(*++line_ptr))
				use_default = 0;


				


			switch (*line_ptr) {
				case 'c':
				case 'C':
					if (!display_in_cyl_units)
						i *= heads * sectors;
					break;
				case 'k':
				case 'K':
					absolute = 1000;
					break;
				case 'm':
				case 'M':
					absolute = 1000000;
					break;
				case 'g':
				case 'G':
					absolute = 1000000000;
					break;
				default:
					break;
			}
			if (absolute) {
				unsigned long long bytes;
				unsigned long unit;

				bytes = (unsigned long long) i * absolute;
				unit = sector_size * units_per_sector;
				bytes += unit/2;	/* round */
				bytes /= unit;
				i = bytes;
			}
			if (minus)
				i = -i;
			i += base;
		} else {
			i = atoi(line_ptr);
			while (isdigit(*line_ptr)) {
				line_ptr++;
				use_default = 0;
			}
		}
		if (use_default)
			printf(("Using default value %d\n"), i = dflt);
		if (i >= low && i <= high)
			break;
		else
			printf(("Value out of range.\n"));
	}

	
	
	return i;
}

int
get_partition(int warn, int max) {
	struct pte *pe;
	int i;

	

	i = read_int(1, 0, max, 0, ("Partition number")) - 1;


	pe = &ptes[i];
	
	if (warn) {
		if ((!sun_label && !sgi_label && !pe->part_table->sys_ind)
		    || (sun_label &&
			(!sunlabel->partitions[i].num_sectors ||
			 !sunlabel->infos[i].id))
		    || (sgi_label && (!sgi_get_num_sectors(i)))
		   )
			fprintf(stderr,
				("Warning: partition %d has empty type\n"),
				i+1);
	}
	return i;
}

static int
get_existing_partition(int warn, int max) {
	int pno = -1;
	int i;

	for (i = 0; i < max; i++) {
		struct pte *pe = &ptes[i];
		struct partition *p = pe->part_table;

		if (p && !is_cleared_partition(p)) {
			if (pno >= 0)
				goto not_unique;
			pno = i;
		}
		
	}

	
	if (pno >= 0) {
	//	printf(("Selected partition %d\n"), pno+1);
		return pno;
	}
	printf(("No partition is defined yet!\n"));
	return -1;

 not_unique:

	
	return get_partition(warn, max);
}

static int
get_nonexisting_partition(int warn, int max) {
	int pno = -1;
	int i;

	for (i = 0; i < max; i++) {
		struct pte *pe = &ptes[i];
		struct partition *p = pe->part_table;

		if (p && is_cleared_partition(p)) {
			if (pno >= 0)
				goto not_unique;
			pno = i;
		}
	}
	if (pno >= 0) {
	//	printf(("Selected partition %d\n"), pno+1);
		return pno;
	}
	printf(("All primary partitions have been defined already!\n"));
	return -1;

 not_unique:
	return get_partition(warn, max);
}

char * const
str_units(int n) {	/* n==1: use singular */
	if (n == 1)
		return display_in_cyl_units ? ("cylinder") : ("sector");
	else
		return display_in_cyl_units ? ("cylinders") : ("sectors");
}

void change_units(void)
{
	display_in_cyl_units = !display_in_cyl_units;
	update_units();
	printf(("Changing display/entry units to %s\n"),
		str_units(PLURAL));
}

static void
toggle_active(int i) {
	struct pte *pe = &ptes[i];
	struct partition *p = pe->part_table;

	if (IS_EXTENDED (p->sys_ind) && !p->boot_ind)
		fprintf(stderr,
			("WARNING: Partition %d is an extended partition\n"),
			i + 1);
	p->boot_ind = (p->boot_ind ? 0 : ACTIVE_FLAG);
	pe->changed = 1;
}

static void
toggle_dos_compatibility_flag(void) {
	dos_compatible_flag = ~dos_compatible_flag;
	if (dos_compatible_flag) {
		sector_offset = sectors;
		printf(("DOS Compatibility flag is set\n"));
	}
	else {
		sector_offset = 1;
		printf(("DOS Compatibility flag is not set\n"));
	}
}

static void
delete_partition(int i) {
	struct pte *pe = &ptes[i];
	struct partition *p = pe->part_table;
	struct partition *q = pe->ext_pointer;

/* Note that for the fifth partition (i == 4) we don't actually
 * decrement partitions.
 */

	if (warn_geometry())
		return;		/* C/H/S not set */
	pe->changed = 1;

	if (sun_label) {
		sun_delete_partition(i);
		return;
	}

	if (sgi_label) {
		sgi_delete_partition(i);
		return;
	}

	if (i < 4) {
		if (IS_EXTENDED (p->sys_ind) && i == ext_index) {
			partitions = 4;
			ptes[ext_index].ext_pointer = NULL;
			extended_offset = 0;
		}
		clear_partition(p);
		return;
	}

	if (!q->sys_ind && i > 4) {
		/* the last one in the chain - just delete */
		--partitions;
		--i;
		clear_partition(ptes[i].ext_pointer);
		ptes[i].changed = 1;
	} else {
		/* not the last one - further ones will be moved down */
		if (i > 4) {
			/* delete this link in the chain */
			p = ptes[i-1].ext_pointer;
			*p = *q;
			set_start_sect(p, get_start_sect(q));
			set_nr_sects(p, get_nr_sects(q));
			ptes[i-1].changed = 1;
		} else if (partitions > 5) {    /* 5 will be moved to 4 */
			/* the first logical in a longer chain */
			struct pte *pe = &ptes[5];

			if (pe->part_table) /* prevent SEGFAULT */
				set_start_sect(pe->part_table,
					       get_partition_start(pe) -
					       extended_offset);
			pe->offset = extended_offset;
			pe->changed = 1;
		}

		if (partitions > 5) {
			partitions--;
			while (i < partitions) {
				ptes[i] = ptes[i+1];
				i++;
			}
		} else
			/* the only logical: clear only */
			clear_partition(ptes[i].part_table);
	}
}

static void
change_sysid(void) {
	char *temp;
	int i, sys, origsys;
	struct partition *p;

	i = get_existing_partition(0, partitions);
	if (i == -1)
		return;
	p = ptes[i].part_table;
	origsys = sys = get_sysid(i);

	/* if changing types T to 0 is allowed, then
	   the reverse change must be allowed, too */
	if (!sys && !sgi_label && !sun_label && !get_nr_sects(p))
                printf(("Partition %d does not exist yet!\n"), i + 1);
        else while (1) {
		sys = read_hex (get_sys_types());

		if (!sys && !sgi_label && !sun_label) {
			printf(("Type 0 means free space to many systems\n"
			       "(but not to Linux). Having partitions of\n"
			       "type 0 is probably unwise. You can delete\n"
			       "a partition using the `d' command.\n"));
			/* break; */
		}

		if (!sun_label && !sgi_label) {
			if (IS_EXTENDED (sys) != IS_EXTENDED (p->sys_ind)) {
				printf(("You cannot change a partition into"
				       " an extended one or vice versa\n"
				       "Delete it first.\n"));
				break;
			}
		}

                if (sys < 256) {
			if (sun_label && i == 2 && sys != WHOLE_DISK)
				printf(("Consider leaving partition 3 "
				       "as Whole disk (5),\n"
				       "as SunOS/Solaris expects it and "
				       "even Linux likes it.\n\n"));
			if (sgi_label && ((i == 10 && sys != ENTIRE_DISK)
					  || (i == 8 && sys != 0)))
				printf(("Consider leaving partition 9 "
				       "as volume header (0),\nand "
				       "partition 11 as entire volume (6)"
				       "as IRIX expects it.\n\n"));
                        if (sys == origsys)
				break;
			if (sun_label) {
				sun_change_sysid(i, sys);
			} else
			if (sgi_label) {
				sgi_change_sysid(i, sys);
			} else
				p->sys_ind = sys;
                        printf (("Changed system type of partition %d "
                                "to %x (%s)\n"), i + 1, sys,
                                (temp = partition_type(sys)) ? temp :
                                ("Unknown"));
                        ptes[i].changed = 1;
			if (is_dos_partition(origsys) ||
			    is_dos_partition(sys))
				dos_changed = 1;
                        break;
                }
        }
}

/* check_consistency() and long2chs() added Sat Mar 6 12:28:16 1993,
 * faith@cs.unc.edu, based on code fragments from pfdisk by Gordon W. Ross,
 * Jan.  1990 (version 1.2.1 by Gordon W. Ross Aug. 1990; Modified by S.
 * Lubkin Oct.  1991). */

static void long2chs(ulong ls, uint *c, uint *h, uint *s) {
	int	spc = heads * sectors;

	*c = ls / spc;
	ls = ls % spc;
	*h = ls / sectors;
	*s = ls % sectors + 1;	/* sectors count from 1 */
}

static void check_consistency(struct partition *p, int partition) {
	uint	pbc, pbh, pbs;		/* physical beginning c, h, s */
	uint	pec, peh, pes;		/* physical ending c, h, s */
	uint	lbc, lbh, lbs;		/* logical beginning c, h, s */
	uint	lec, leh, les;		/* logical ending c, h, s */

	if (!heads || !sectors || (partition >= 4))
		return;		/* do not check extended partitions */

/* physical beginning c, h, s */
	pbc = (p->cyl & 0xff) | ((p->sector << 2) & 0x300);
	pbh = p->head;
	pbs = p->sector & 0x3f;

/* physical ending c, h, s */
	pec = (p->end_cyl & 0xff) | ((p->end_sector << 2) & 0x300);
	peh = p->end_head;
	pes = p->end_sector & 0x3f;

/* compute logical beginning (c, h, s) */
	long2chs(get_start_sect(p), &lbc, &lbh, &lbs);

/* compute logical ending (c, h, s) */
	long2chs(get_start_sect(p) + get_nr_sects(p) - 1, &lec, &leh, &les);

/* Same physical / logical beginning? */
	if (cylinders <= 1024 && (pbc != lbc || pbh != lbh || pbs != lbs)) {
		printf(("Partition %d has different physical/logical "
			"beginnings (non-Linux?):\n"), partition + 1);
		printf(("     phys=(%d, %d, %d) "), pbc, pbh, pbs);
		printf(("logical=(%d, %d, %d)\n"),lbc, lbh, lbs);
	}

/* Same physical / logical ending? */
	if (cylinders <= 1024 && (pec != lec || peh != leh || pes != les)) {
		printf(("Partition %d has different physical/logical "
			"endings:\n"), partition + 1);
		printf(("     phys=(%d, %d, %d) "), pec, peh, pes);
		printf(("logical=(%d, %d, %d)\n"),lec, leh, les);
	}

#if 0
/* Beginning on cylinder boundary? */
	if (pbh != !pbc || pbs != 1) {
		printf(("Partition %i does not start on cylinder "
			"boundary:\n"), partition + 1);
		printf(("     phys=(%d, %d, %d) "), pbc, pbh, pbs);
		printf(("should be (%d, %d, 1)\n"), pbc, !pbc);
	}
#endif

/* Ending on cylinder boundary? */
	if (peh != (heads - 1) || pes != sectors) {
		printf(("Partition %i does not end on cylinder boundary.\n"),
			partition + 1);
#if 0
		printf(("     phys=(%d, %d, %d) "), pec, peh, pes);
		printf(("should be (%d, %d, %d)\n"),
		pec, heads - 1, sectors);
#endif
	}
}

static void
list_disk_geometry(void) {
	long long bytes = (long long) total_number_of_sectors * 512;
	long megabytes = bytes/1000000;
	long long max_usb_size = 0;

/*
	if (megabytes < 10000)
		printf(("\nDisk %s: %ld MB, %lld bytes\n"),
		       disk_device, megabytes, bytes);
	else
		printf(("\nDisk %s: %ld.%ld GB, %lld bytes\n"),
		       disk_device, megabytes/1000, (megabytes/100)%10, bytes);
	printf(("%d heads, %d sectors/track, %d cylinders"),
	       heads, sectors, cylinders);
	if (units_per_sector == 1)
		printf((", total %lu sectors"),
		       total_number_of_sectors / (sector_size/512));
	printf("\n");
	printf(("Units = %s of %d * %d = %d bytes\n\n"),
	       str_units(PLURAL),
	       units_per_sector, sector_size, units_per_sector * sector_size);
*/
/*
	// add
	 if( g_haveUsb == 1 )
	 {
	 	usb_info.size = bytes;
		usb_info.cylinder = cylinders;
	 }
	 else
	 {
		disk_info[iCurrentDevNumber].size = bytes;

		disk_info[iCurrentDevNumber].cylinder = cylinders;
	 }
*/

	 //add 09-08-26
	 

	max_usb_size = (long long)MAX_USB_SIZE*1024*1024*1024;

	//printf("bytes = %lld  max_usb_size=%lld\n",bytes,max_usb_size);
	if( bytes < max_usb_size )
	{
		//小于MAX_USB_SIZE 的我们认为是usb 设备

		usb_info.size = bytes;
		usb_info.cylinder = cylinders;
		g_haveUsb = 1;
	}else
	{
		disk_info[iCurrentDevNumber].size = bytes;
		disk_info[iCurrentDevNumber].cylinder = cylinders;
		g_haveUsb = 0;
	}
	
	 
/*
	printf("\n id = %s, size = %lld, cylinder = %d",
		disk_info[iCurrentDevNumber].devname,
		disk_info[iCurrentDevNumber].size,
		disk_info[iCurrentDevNumber].cylinder);
		*/
	// end
}

/*
 * Check whether partition entries are ordered by their starting positions.
 * Return 0 if OK. Return i if partition i should have been earlier.
 * Two separate checks: primary and logical partitions.
 */
static int
wrong_p_order(int *prev) {
	struct pte *pe;
	struct partition *p;
	uint last_p_start_pos = 0, p_start_pos;
	int i, last_i = 0;

	for (i = 0 ; i < partitions; i++) {
		if (i == 4) {
			last_i = 4;
			last_p_start_pos = 0;
		}
		pe = &ptes[i];
		if ((p = pe->part_table)->sys_ind) {
			p_start_pos = get_partition_start(pe);

			if (last_p_start_pos > p_start_pos) {
				if (prev)
					*prev = last_i;
				return i;
			}

			last_p_start_pos = p_start_pos;
			last_i = i;
		}
	}
	return 0;
}

/*
 * Fix the chain of logicals.
 * extended_offset is unchanged, the set of sectors used is unchanged
 * The chain is sorted so that sectors increase, and so that
 * starting sectors increase.
 *
 * After this it may still be that cfdisk doesnt like the table.
 * (This is because cfdisk considers expanded parts, from link to
 * end of partition, and these may still overlap.)
 * Now
 *   sfdisk /dev/hda > ohda; sfdisk /dev/hda < ohda
 * may help.
 */
static void
fix_chain_of_logicals(void) {
	int j, oj, ojj, sj, sjj;
	struct partition *pj,*pjj,tmp;

	/* Stage 1: sort sectors but leave sector of part 4 */
	/* (Its sector is the global extended_offset.) */
 stage1:
	for (j = 5; j < partitions-1; j++) {
		oj = ptes[j].offset;
		ojj = ptes[j+1].offset;
		if (oj > ojj) {
			ptes[j].offset = ojj;
			ptes[j+1].offset = oj;
			pj = ptes[j].part_table;
			set_start_sect(pj, get_start_sect(pj)+oj-ojj);
			pjj = ptes[j+1].part_table;
			set_start_sect(pjj, get_start_sect(pjj)+ojj-oj);
			set_start_sect(ptes[j-1].ext_pointer,
				       ojj-extended_offset);
			set_start_sect(ptes[j].ext_pointer,
				       oj-extended_offset);
			goto stage1;
		}
	}

	/* Stage 2: sort starting sectors */
 stage2:
	for (j = 4; j < partitions-1; j++) {
		pj = ptes[j].part_table;
		pjj = ptes[j+1].part_table;
		sj = get_start_sect(pj);
		sjj = get_start_sect(pjj);
		oj = ptes[j].offset;
		ojj = ptes[j+1].offset;
		if (oj+sj > ojj+sjj) {
			tmp = *pj;
			*pj = *pjj;
			*pjj = tmp;
			set_start_sect(pj, ojj+sjj-oj);
			set_start_sect(pjj, oj+sj-ojj);
			goto stage2;
		}
	}

	/* Probably something was changed */
	for (j = 4; j < partitions; j++)
		ptes[j].changed = 1;
}

static void
fix_partition_table_order(void) {
	struct pte *pei, *pek;
	int i,k;

	if (!wrong_p_order(NULL)) {
		printf(("Nothing to do. Ordering is correct already.\n\n"));
		return;
	}

	while ((i = wrong_p_order(&k)) != 0 && i < 4) {
		/* partition i should have come earlier, move it */
		/* We have to move data in the MBR */
		struct partition *pi, *pk, *pe, pbuf;
		pei = &ptes[i];
		pek = &ptes[k];

		pe = pei->ext_pointer;
		pei->ext_pointer = pek->ext_pointer;
		pek->ext_pointer = pe;

		pi = pei->part_table;
		pk = pek->part_table;

		memmove(&pbuf, pi, sizeof(struct partition));
		memmove(pi, pk, sizeof(struct partition));
		memmove(pk, &pbuf, sizeof(struct partition));

		pei->changed = pek->changed = 1;
	}

	if (i)
		fix_chain_of_logicals();

	printf("Done.\n");

}

static void
list_table(int xtra) {
	struct partition *p;
	char *type;
	int i, w;

	if (sun_label) {
		sun_list_table(xtra);
		return;
	}

	if (sgi_label) {
		sgi_list_table(xtra);
		return;
	}

	list_disk_geometry();

	if (osf_label) {
		xbsd_print_disklabel(xtra);
		return;
	}

	/* Heuristic: we list partition 3 of /dev/foo as /dev/foo3,
	   but if the device name ends in a digit, say /dev/foo1,
	   then the partition is called /dev/foo1p3. */
	w = strlen(disk_device);
	if (w && isdigit(disk_device[w-1]))
		w++;
	if (w < 5)
		w = 5;

	if( g_haveUsb == 1 )
		usb_info.partitionNum = 0;
	else
		disk_info[iCurrentDevNumber].partitionNum = 0;


//	printf(("%*s Boot    Start       End    Blocks   Id  System\n"),
//	       w+1, ("Device"));

	

	for (i = 0; i < partitions; i++) {
		struct pte *pe = &ptes[i];

		p = pe->part_table;
		if (p && !is_cleared_partition(p)) {
			unsigned int psects = get_nr_sects(p);
			unsigned int pblocks = psects;
			unsigned int podd = 0;

			if (sector_size < 1024) {
				pblocks /= (1024 / sector_size);
				podd = psects % (1024 / sector_size);
			}
			if (sector_size > 1024)
				pblocks *= (sector_size / 1024);

/*
                       printf(
			    "%s  %c %9ld %9ld %9ld%c  %2x  %s\n",
			partname(disk_device, i+1, w+2),
			!p->boot_ind ? ' ' : p->boot_ind == ACTIVE_FLAG
			? '*' : '?',
			(long) cround(get_partition_start(pe)),
			(long) cround(get_partition_start(pe) + psects
					- (psects ? 1 : 0)),
			(long) pblocks, podd ? '+' : ' ',
		p->sys_ind,
			(type = partition_type(p->sys_ind)) ?
			type : ("Unknown"));
*/
			
				

//add
			if( g_haveUsb == 1 )
			{
				usb_info.partitionID[usb_info.partitionNum] = ( i+1 );
				usb_info.disktype[usb_info.partitionNum] = p->sys_ind;
				usb_info.partitionSize[usb_info.partitionNum] = pblocks ;
				usb_info.partitionNum++;
			}
			else
			{
				disk_info[iCurrentDevNumber].partitionID[disk_info[iCurrentDevNumber].partitionNum] = ( i+1 );

				disk_info[iCurrentDevNumber].disktype[disk_info[iCurrentDevNumber].partitionNum] = p->sys_ind;

				disk_info[iCurrentDevNumber].partitionSize[disk_info[iCurrentDevNumber].partitionNum] = pblocks ;

				disk_info[iCurrentDevNumber].partitionNum++;
			}
//end

						
			check_consistency(p, i);
		}
	}

	/* Is partition table in disk order? It need not be, but... */
	/* partition table entries are not checked for correct order if this
	   is a sgi, sun or aix labeled disk... */
	if (dos_label && wrong_p_order(NULL)) {
		printf(("\nPartition table entries are not in disk order\n"));
	}
}

static void
x_list_table(int extend) {
	struct pte *pe;
	struct partition *p;
	int i;

	printf(("\nDisk %s: %d heads, %d sectors, %d cylinders\n\n"),
		disk_device, heads, sectors, cylinders);
        printf(("Nr AF  Hd Sec  Cyl  Hd Sec  Cyl    Start     Size ID\n"));
	for (i = 0 ; i < partitions; i++) {
		pe = &ptes[i];
		p = (extend ? pe->ext_pointer : pe->part_table);
		if (p != NULL) {
                        printf("%2d %02x%4d%4d%5d%4d%4d%5d%9d%9d %02x\n",
				i + 1, p->boot_ind, p->head,
				sector(p->sector),
				cylinder(p->sector, p->cyl), p->end_head,
				sector(p->end_sector),
				cylinder(p->end_sector, p->end_cyl),
				get_start_sect(p), get_nr_sects(p), p->sys_ind);
			if (p->sys_ind)
				check_consistency(p, i);
		}
	}
}

static void
fill_bounds(uint *first, uint *last) {
	int i;
	struct pte *pe = &ptes[0];
	struct partition *p;

	for (i = 0; i < partitions; pe++,i++) {
		p = pe->part_table;
		if (!p->sys_ind || IS_EXTENDED (p->sys_ind)) {
			first[i] = 0xffffffff;
			last[i] = 0;
		} else {
			first[i] = get_partition_start(pe);
			last[i] = first[i] + get_nr_sects(p) - 1;
		}
	}
}

static void
check(int n, uint h, uint s, uint c, uint start) {
	uint total, real_s, real_c;

	real_s = sector(s) - 1;
	real_c = cylinder(s, c);
	total = (real_c * sectors + real_s) * heads + h;
	if (!total)
		fprintf(stderr, ("Warning: partition %d contains sector 0\n"), n);
	if (h >= heads)
		fprintf(stderr,
			("Partition %d: head %d greater than maximum %d\n"),
			n, h + 1, heads);
	if (real_s >= sectors)
		fprintf(stderr, ("Partition %d: sector %d greater than "
			"maximum %d\n"), n, s, sectors);
	if (real_c >= cylinders)
		fprintf(stderr, ("Partitions %d: cylinder %d greater than "
			"maximum %d\n"), n, real_c + 1, cylinders);
	if (cylinders <= 1024 && start != total)
		fprintf(stderr,
			("Partition %d: previous sectors %d disagrees with "
			"total %d\n"), n, start, total);
}

static void
verify(void) {
	int i, j;
	uint total = 1;
	uint first[partitions], last[partitions];
	struct partition *p;

	if (warn_geometry())
		return;

	if (sun_label) {
		verify_sun();
		return;
	}

	if (sgi_label) {
		verify_sgi(1);
		return;
	}

	fill_bounds(first, last);
	for (i = 0; i < partitions; i++) {
		struct pte *pe = &ptes[i];

		p = pe->part_table;
		if (p->sys_ind && !IS_EXTENDED (p->sys_ind)) {
			check_consistency(p, i);
			if (get_partition_start(pe) < first[i])
				printf(("Warning: bad start-of-data in "
					"partition %d\n"), i + 1);
			check(i + 1, p->end_head, p->end_sector, p->end_cyl,
				last[i]);
			total += last[i] + 1 - first[i];
			for (j = 0; j < i; j++)
			if ((first[i] >= first[j] && first[i] <= last[j])
			 || ((last[i] <= last[j] && last[i] >= first[j]))) {
				printf(("Warning: partition %d overlaps "
					"partition %d.\n"), j + 1, i + 1);
				total += first[i] >= first[j] ?
					first[i] : first[j];
				total -= last[i] <= last[j] ?
					last[i] : last[j];
			}
		}
	}

	if (extended_offset) {
		struct pte *pex = &ptes[ext_index];
		uint e_last = get_start_sect(pex->part_table) +
			get_nr_sects(pex->part_table) - 1;

		for (i = 4; i < partitions; i++) {
			total++;
			p = ptes[i].part_table;
			if (!p->sys_ind) {
				if (i != 4 || i + 1 < partitions)
					printf(("Warning: partition %d "
						"is empty\n"), i + 1);
			}
			else if (first[i] < extended_offset ||
					last[i] > e_last)
				printf(("Logical partition %d not entirely in "
					"partition %d\n"), i + 1, ext_index + 1);
		}
	}

	if (total > heads * sectors * cylinders)
		printf(("Total allocated sectors %d greater than the maximum "
			"%d\n"), total, heads * sectors * cylinders);
	else if ((total = heads * sectors * cylinders - total) != 0)
		printf(("%d unallocated sectors\n"), total);
}

static void
add_partition(int n, int sys) {
	char mesg[256];		/* 48 does not suffice in Japanese */
	int i, read = 0;
	struct partition *p = ptes[n].part_table;
	struct partition *q = ptes[ext_index].part_table;
	uint start, stop = 0, limit, temp,
		first[partitions], last[partitions];

	if (p && p->sys_ind) {
		printf(("Partition %d is already defined.  Delete "
			 "it before re-adding it.\n"), n + 1);
		return;
	}
	fill_bounds(first, last);
	if (n < 4) {
		start = sector_offset;
		if (display_in_cyl_units)
			limit = heads * sectors * cylinders - 1;
		else
			limit = total_number_of_sectors - 1;
		if (extended_offset) {
			first[ext_index] = extended_offset;
			last[ext_index] = get_start_sect(q) +
				get_nr_sects(q) - 1;
		}
	} else {
		start = extended_offset + sector_offset;
		limit = get_start_sect(q) + get_nr_sects(q) - 1;
		//printf(" \n my limit: %d-%d",cround(start),cround(limit));
	}
	if (display_in_cyl_units)
		for (i = 0; i < partitions; i++)
			first[i] = (cround(first[i]) - 1) * units_per_sector;

	snprintf(mesg, sizeof(mesg), ("First %s"), str_units(SINGULAR));
	do {
		temp = start;
		for (i = 0; i < partitions; i++) {
			int lastplusoff;

			if (start == ptes[i].offset)
				start += sector_offset;
			lastplusoff = last[i] + ((n<4) ? 0 : sector_offset);
			if (start >= first[i] && start <= lastplusoff)
				start = lastplusoff + 1;
		}
		if (start > limit)
			break;
		if (start >= temp+units_per_sector && read) {
			printf(("Sector %d is already allocated\n"), temp);
			temp = start;
			read = 0;
		}
		if (!read && start == temp) {
			uint i;
			i = start;
			if( patitionselect == 'e' )
			{
				start = cround(i);          // use default start value.
			}
			else    //  patitionselect == 'l'  logic
			{
				start = cround(i);        // use default start value.
			}

	//		start = read_int(cround(i), cround(i), cround(limit),
	//				 0, mesg);
			
			if (display_in_cyl_units) {
				start = (start - 1) * units_per_sector;
				if (start < i) start = i;
			}
			read = 1;
		}
	} while (start != temp || !read);
	if (n > 4) {			/* NOT for fifth partition */
		struct pte *pe = &ptes[n];

		pe->offset = start - sector_offset;
		if (pe->offset == extended_offset) { /* must be corrected */
			pe->offset++;
			if (sector_offset == 1)
				start++;
		}
	}

	for (i = 0; i < partitions; i++) {
		struct pte *pe = &ptes[i];

		if (start < pe->offset && limit >= pe->offset)
			limit = pe->offset - 1;
		if (start < first[i] && limit >= first[i])
			limit = first[i] - 1;
	}
	if (start > limit) {
		printf(("No free sectors available\n"));
		if (n > 4)
			partitions--;
		return;
	}
	if (cround(start) == cround(limit)) {
		stop = limit;
	} else {
		snprintf(mesg, sizeof(mesg),
			 ("Last %s or +size or +sizeM or +sizeK"),
			 str_units(SINGULAR));

		if( patitionselect == 'e' )
		{
				stop = cround(limit); 				// use default end value.	
		}
		else
		{
				stop = endcylinder;
		}

	//	stop = read_int(cround(start), cround(limit), cround(limit),
	//			cround(start), mesg);
		
		if (display_in_cyl_units) {
			stop = stop * units_per_sector - 1;
			if (stop >limit)
				stop = limit;
		}
	}

	set_partition(n, 0, start, stop, sys);
	if (n > 4)
		set_partition(n - 1, 1, ptes[n].offset, stop, EXTENDED);

	if (IS_EXTENDED (sys)) {
		struct pte *pe4 = &ptes[4];
		struct pte *pen = &ptes[n];

		ext_index = n;
		pen->ext_pointer = p;
		pe4->offset = extended_offset = start;
		if (!(pe4->sectorbuffer = calloc(1, sector_size)))
			fatal(out_of_memory);
		pe4->part_table = pt_offset(pe4->sectorbuffer, 0);
		pe4->ext_pointer = pe4->part_table + 1;
		pe4->changed = 1;
		partitions = 5;
	}
}

static void
add_logical(void) {
	if (partitions > 5 || ptes[4].part_table->sys_ind) {
		struct pte *pe = &ptes[partitions];

		if (!(pe->sectorbuffer = calloc(1, sector_size)))
			fatal(out_of_memory);
		pe->part_table = pt_offset(pe->sectorbuffer, 0);
		pe->ext_pointer = pe->part_table + 1;
		pe->offset = 0;
		pe->changed = 1;
		partitions++;
	}
	add_partition(partitions - 1, WIN95_FAT32);
}

static void
new_partition(void) {
	int i, free_primary = 0;

	if (warn_geometry())
		return;

	if (sun_label) {
		add_sun_partition(get_partition(0, partitions), LINUX_NATIVE);
		return;
	}

	if (sgi_label) {
		sgi_add_partition(get_partition(0, partitions), LINUX_NATIVE);
		return;
	}

	if (aix_label) {
		printf(("\tSorry - this fdisk cannot handle AIX disk labels."
			 "\n\tIf you want to add DOS-type partitions, create"
			 "\n\ta new empty DOS partition table first. (Use o.)"
			 "\n\tWARNING: "
			 "This will destroy the present disk contents.\n"));
		return;
	}

	for (i = 0; i < 4; i++)
		free_primary += !ptes[i].part_table->sys_ind;

	if (!free_primary && partitions >= MAXIMUM_PARTS) {
		printf(("The maximum number of partitions has been created\n"));
		return;
	}

	if (!free_primary) {
		if (extended_offset)
			add_logical();
		else
			printf(("You must delete some partition and add "
				 "an extended partition first\n"));
	} else {
		char c, line[LINE_LENGTH];
		
		c = patitionselect;
		
		snprintf(line, sizeof(line),
			 ("Command action\n   %s\n   p   primary "
			   "partition (1-4)\n"), extended_offset ?
			 ("l   logical (5 or over)") : ("e   extended"));
		while (1) {
		//	if ((c = tolower(read_char(line))) == 'p') {
		if (c  == 'p') {
				//int i = get_nonexisting_partition(0, 4);
				//if (i >= 0)
					add_partition(0, WIN95_FAT32);
				return;
			}
			else if (c == 'l' && extended_offset) {
				add_logical();
				return;
			}
			else if (c == 'e' && !extended_offset) {
			//	int i = get_nonexisting_partition(0, 4);
			//	if (i >= 0)
					add_partition(0, WIN98_EXTENDED);
				return;
			}
			else
				printf(("Invalid partition number "
					 "for type `%c'\n"), c);
		}
	}
}

static void
write_table(void) {
	int i;

	if (dos_label) {
		for (i=0; i<3; i++)
			if (ptes[i].changed)
				ptes[3].changed = 1;
		for (i = 3; i < partitions; i++) {
			struct pte *pe = &ptes[i];

			if (pe->changed) {
				write_part_table_flag(pe->sectorbuffer);
				write_sector(fd, pe->offset, pe->sectorbuffer);
			}
		}
	}
	else if (sgi_label) {
		/* no test on change? the printf below might be mistaken */
		sgi_write_table();
	} else if (sun_label) {
		int needw = 0;

		for (i=0; i<8; i++)
			if (ptes[i].changed)
				needw = 1;
		if (needw)
			sun_write_table();
	}

	printf(("The partition table has been altered!\n\n"));
	reread_partition_table(1);
}

void
reread_partition_table(int leave) {
	int error = 0;
	int i;


	printf(("Calling ioctl() to re-read partition table.\n"));
	sync();
	sleep(2);
	if ((i = ioctl(fd, BLKRRPART)) != 0) {
                error = errno;
        } else {
                /* some kernel versions (1.2.x) seem to have trouble
                   rereading the partition table, but if asked to do it
		   twice, the second time works. - biro@yggdrasil.com */
                sync();
                sleep(2);
                if ((i = ioctl(fd, BLKRRPART)) != 0)
                        error = errno;
        }

	if (i) {
		printf(("\nWARNING: Re-reading the partition table "
			 "failed with error %d: %s.\n"
			 "The kernel still uses the old table.\n"
			 "The new table will be used "
			 "at the next reboot.\n"),
			error, strerror(error));
	}

	if (dos_changed)
	    printf(
		("\nWARNING: If you have created or modified any DOS 6.x\n"
		"partitions, please see the fdisk manual page for additional\n"
		"information.\n"));

	if (leave) {
		close(fd);

		printf(("Syncing disks.\n"));
		sync();
		sleep(4);		/* for sync() */
	//	exit(!!i);
	}
}

#define MAX_PER_LINE	16
static void
print_buffer(char pbuffer[]) {
	int	i,
		l;

	for (i = 0, l = 0; i < sector_size; i++, l++) {
		if (l == 0)
			printf("0x%03X:", i);
		printf(" %02X", (unsigned char) pbuffer[i]);
		if (l == MAX_PER_LINE - 1) {
			printf("\n");
			l = -1;
		}
	}
	if (l > 0)
		printf("\n");
	printf("\n");
}

static void
print_raw(void) {
	int i;

	printf(("Device: %s\n"), disk_device);
	if (sun_label || sgi_label)
		print_buffer(MBRbuffer);
	else for (i = 3; i < partitions; i++)
		print_buffer(ptes[i].sectorbuffer);
}

static void
move_begin(int i) {
	struct pte *pe = &ptes[i];
	struct partition *p = pe->part_table;
	uint new, first;

	if (warn_geometry())
		return;
	if (!p->sys_ind || !get_nr_sects(p) || IS_EXTENDED (p->sys_ind)) {
		printf(("Partition %d has no data area\n"), i + 1);
		return;
	}
	first = get_partition_start(pe);
	new = read_int(first, first, first + get_nr_sects(p) - 1, first,
		       ("New beginning of data")) - pe->offset;

	if (new != get_nr_sects(p)) {
		first = get_nr_sects(p) + get_start_sect(p) - new;
		set_nr_sects(p, first);
		set_start_sect(p, new);
		pe->changed = 1;
	}
}

static void
xselect(void) {
	char c;

	while(1) {
		putchar('\n');
		c = tolower(read_char(("Expert command (m for help): ")));
		switch (c) {
		case 'a':
			if (sun_label)
				sun_set_alt_cyl();
			break;
		case 'b':
			if (dos_label)
				move_begin(get_partition(0, partitions));
			break;
		case 'c':
			user_cylinders = cylinders =
				read_int(1, cylinders, 1048576, 0,
					 ("Number of cylinders"));
			if (sun_label)
				sun_set_ncyl(cylinders);
			if (dos_label)
				warn_cylinders();
			break;
		case 'd':
			print_raw();
			break;
		case 'e':
			if (sgi_label)
				sgi_set_xcyl();
			else if (sun_label)
				sun_set_xcyl();
			else
			if (dos_label)
				x_list_table(1);
			break;
		case 'f':
			if (dos_label)
				fix_partition_table_order();
			break;
		case 'g':
			create_sgilabel();
			break;
		case 'h':
			user_heads = heads = read_int(1, heads, 256, 0,
					 ("Number of heads"));
			update_units();
			break;
		case 'i':
			if (sun_label)
				sun_set_ilfact();
			break;
		case 'o':
			if (sun_label)
				sun_set_rspeed();
			break;
		case 'p':
			if (sun_label)
				list_table(1);
			else
				x_list_table(0);
			break;
		case 'q':
			close(fd);
			printf("\n");
			exit(0);
		case 'r':
			return;
		case 's':
			user_sectors = sectors = read_int(1, sectors, 63, 0,
					   ("Number of sectors"));
			if (dos_compatible_flag) {
				sector_offset = sectors;
				fprintf(stderr, ("Warning: setting "
					"sector offset for DOS "
					"compatiblity\n"));
			}
			update_units();
			break;
		case 'v':
			verify();
			break;
		case 'w':
			write_table(); 	/* does not return */
			break;
		case 'y':
			if (sun_label)
				sun_set_pcylcount();
			break;
		default:
			xmenu();
		}
	}
}

static int
is_ide_cdrom_or_tape(char *device) {
	FILE *procf;
	char buf[100];
	struct stat statbuf;
	int is_ide = 0;

	/* No device was given explicitly, and we are trying some
	   likely things.  But opening /dev/hdc may produce errors like
           "hdc: tray open or drive not ready"
	   if it happens to be a CD-ROM drive. It even happens that
	   the process hangs on the attempt to read a music CD.
	   So try to be careful. This only works since 2.1.73. */

	if (strncmp("/dev/hd", device, 7))
		return 0;

	snprintf(buf, sizeof(buf), "/proc/ide/%s/media", device+5);
	procf = fopen(buf, "r");
	if (procf != NULL && fgets(buf, sizeof(buf), procf))
		is_ide = (!strncmp(buf, "cdrom", 5) ||
			  !strncmp(buf, "tape", 4));
	else
		/* Now when this proc file does not exist, skip the
		   device when it is read-only. */
		if (stat(device, &statbuf) == 0)
			is_ide = ((statbuf.st_mode & 0222) == 0);

	if (procf)
		fclose(procf);
	return is_ide;
}

static void
try(char *device, int user_specified) {
	int gb;

int fd1;

disk_device = device;

	if (setjmp(listingbuf))
		return;
	if (!user_specified)
		if (is_ide_cdrom_or_tape(device))
			return;
	if ((fd1 = open(disk_device, type_open)) >= 0) {

/*    // 原1.0版
		gb = get_boot(try_only);     // get /dev/hda info.
		close(fd);
		if (gb > 0) {	
			close(fd1);
		} else if (gb < 0) { 
			list_disk_geometry();
			if (aix_label)
				return;
			if (btrydev(device) < 0)
				fprintf(stderr,
					("Disk %s doesn't contain a valid "
					  "partition table\n"), device);
			close(fd1);
		} else {
			close(fd1);
			list_table(0);
		
			if (!sun_label && partitions > 4)
				delete_partition(ext_index);
		}
*/
//2006年 3月10 日改，支持了未有分区表的硬盘。	

		
		gb = get_boot(try_only);     // get /dev/hda info.	
			if (gb > 0) {	/* I/O error */
			close(fd);
			close(fd1);
		} else if (gb < 0) { /* no DOS signature */
			list_disk_geometry();
			if (aix_label)
				return;
			if (btrydev(device) < 0)
				fprintf(stderr,
					("Disk %s doesn't contain a valid "
					  "partition table\n"), device);
			close(fd);
			close(fd1);
		} else {
			close(fd);
			close(fd1);
			list_table(0);
			if (!sun_label && partitions > 4)
				delete_partition(ext_index);
		}
	} else {
		/* Ignore other errors, since we try IDE
		   and SCSI hard disks which may not be
		   installed on the system. */
		if (errno == EACCES) {
			fprintf(stderr, ("Cannot open %s\n"), device);
			return;
		}
	}
}

/* for fdisk -l: try all things in /proc/partitions
   that look like a partition name (do not end in a digit) */
static void
tryprocpt(void) {
	FILE *procpt;
	char line[100], ptname[100], devname[120], *s;
	int ma, mi, sz;
	char * p;
	int i;
	int between = 0;

	int only_once = 1;

	DiskNum = 0;

	iUsbNum = 0;

	g_haveUsb = 0;	

	procpt = fopen(PROC_PARTITIONS, "r");
	if (procpt == NULL) {
		fprintf(stderr, ("cannot open %s\n"), PROC_PARTITIONS);
		return;
	}	

	while (fgets(line, sizeof(line), procpt)) {
		if (sscanf (line, " %d %d %d %[^\n ]",
			    &ma, &mi, &sz, ptname) != 4)
			continue;

		/*
		#ifdef USE_SATA_DISK
   			if(  strstr(ptname,"mtdblock") != NULL )
				between = 1;
		#endif
		*/

		//printf(" dev = %s\n",ptname);


		if(  (strstr(ptname,"mmcblk0") != NULL)  &&  (only_once == 1) )
		{
			only_once = 0;
		}else
		{		
			for (s = ptname; *s; s++);
			if (isdigit(s[-1]))
				continue;
		}

		//printf("222 dev = %s\n",ptname);

		// make sure this device is disk!
		/*
		// 2006.11.14 change
		// because the harddriver devices in /proc/partitions had change to 
		// ide/host0/bus0/target0/lun0/disc  not  /dev/hda
		if( ( strstr(ptname,"hd") == NULL ) && ( strstr(ptname,"sd") == NULL ) )
			{
				continue;
			}
			*/

		// old
	/*	if( ( strstr(ptname,"ide") == NULL ) && ( strstr(ptname,"lun") == NULL ) )
		{
				continue;
		}

		if(  strstr(ptname,"ide/host0/bus0/target0") != NULL )
			sprintf(devname, "/dev/hda");
		
		if(  strstr(ptname,"ide/host0/bus0/target1") != NULL )
			sprintf(devname, "/dev/hdb");

		if(  strstr(ptname,"scsi/host0/bus0/target0") != NULL )
			sprintf(devname, "/dev/sda");
			

		//if( strstr(ptname,"sd") != NULL )
		if( strstr(ptname,"scsi/host0/") != NULL )
		{
			g_haveUsb = 1;

			strcpy(usb_info.devname,devname);

			iUsbNum++;
			
		}
		else
		{
			g_haveUsb = 0;
		
			iCurrentDevNumber = DiskNum;

			strcpy(disk_info[iCurrentDevNumber].devname, devname);

			DiskNum++;
		
		}
		*/

		// change
/*
		if( ( strstr(ptname,"ide") == NULL ) )
		{
				continue;
		}
*/
/*
		
#ifdef USE_SATA_DISK	

	     if( between == 0 ) 
	     {
		     	if(  strstr(ptname,"sda") != NULL )
				sprintf(devname, "/dev/sda");
			
			if(  strstr(ptname,"sdb") != NULL )
				sprintf(devname, "/dev/sdb");

			if(  strstr(ptname,"sdc") != NULL )
				sprintf(devname, "/dev/sdc");
			
			if(  strstr(ptname,"sdd") != NULL )
				sprintf(devname, "/dev/sdd");

			if(  strstr(ptname,"hda") != NULL )
				sprintf(devname, "/dev/hda");
		
			if(  strstr(ptname,"hdb") != NULL )
				sprintf(devname, "/dev/hdb");

			
			printf("find disk %s\n",devname);


			iCurrentDevNumber = DiskNum;

			strcpy(disk_info[iCurrentDevNumber].devname, devname);

			DiskNum++;
	     }	

	   if( between == 1 )
	   {
	   	
		if( strstr(ptname,"sd") != NULL )
		{
			g_haveUsb = 1;

			if(  strstr(ptname,"sda") != NULL )
				sprintf(devname, "/dev/sda");


			if(  strstr(ptname,"sdb") != NULL )
				sprintf(devname, "/dev/sdb");

			if(  strstr(ptname,"sdc") != NULL )
				sprintf(devname, "/dev/sdc");
			
			if(  strstr(ptname,"sdd") != NULL )
				sprintf(devname, "/dev/sdd");

			if(  strstr(ptname,"sde") != NULL )
			sprintf(devname, "/dev/sde");
		
			if(  strstr(ptname,"sdf") != NULL )
			sprintf(devname, "/dev/sdf");

			strcpy(usb_info.devname,devname);

			printf("find usb %s\n",devname);

			iUsbNum++;
			
		}
	   }

#else

		if(  strstr(ptname,"hda") != NULL )
			sprintf(devname, "/dev/hda");
		
		if(  strstr(ptname,"hdb") != NULL )
			sprintf(devname, "/dev/hdb");
	

		if( strstr(ptname,"sd") != NULL )
		{
			g_haveUsb = 1;

			if(  strstr(ptname,"sda") != NULL )
			sprintf(devname, "/dev/sda");
		
			if(  strstr(ptname,"sdb") != NULL )
			sprintf(devname, "/dev/sdb");

			strcpy(usb_info.devname,devname);

			printf("find %s\n",devname);

			iUsbNum++;
			
		}
		else
		{			
		
			iCurrentDevNumber = DiskNum;

			strcpy(disk_info[iCurrentDevNumber].devname, devname);

			DiskNum++;
		
		}

#endif
*/

		if(  strstr(ptname,"sda") != NULL )
			sprintf(devname, "/dev/sda");
		
		if(  strstr(ptname,"sdb") != NULL )
			sprintf(devname, "/dev/sdb");

		if(  strstr(ptname,"sdc") != NULL )
			sprintf(devname, "/dev/sdc");
		
		if(  strstr(ptname,"sdd") != NULL )
			sprintf(devname, "/dev/sdd");

		if(  strstr(ptname,"sde") != NULL )
			sprintf(devname, "/dev/sde");

		if(  strstr(ptname,"sdf") != NULL )
			sprintf(devname, "/dev/sdf");

		if(  strstr(ptname,"sdg") != NULL )
			sprintf(devname, "/dev/sdg");
		
		if(  strstr(ptname,"sdh") != NULL )
			sprintf(devname, "/dev/sdh");		

		if(  strstr(ptname,"sdi") != NULL )
			sprintf(devname, "/dev/sdi");		


		if(  strstr(ptname,"sdj") != NULL )
			sprintf(devname, "/dev/sdj");			
		

		if(  strstr(ptname,"hda") != NULL )
			sprintf(devname, "/dev/hda");
		
		if(  strstr(ptname,"hdb") != NULL )
			sprintf(devname, "/dev/hdb");		

		if(  strstr(ptname,"mmcblk0") != NULL )
			sprintf(devname, "/dev/sda");		
		


		iCurrentDevNumber = DiskNum;

		strcpy(disk_info[iCurrentDevNumber].devname, devname);	
		
		try(devname, 0);

		if( g_haveUsb == 1 )
		{
			strcpy(usb_info.devname,devname);

			printf("find usb %s\n",devname);

			g_haveUsb = 0;

			iUsbNum++;
		}else
		{
			//printf("find disk %s\n",devname);
			DiskNum++;
		}
	
	}
	fclose(procpt);

/*	iUsbNum = DISK_GetUsbStatus(usb_info.devname,0);
	if( iUsbNum > 0 )
	{
		usb_info.partitionNum = 1;
		usb_info.partitionID[0] = 1;
	}
	*/
}

static void
dummy(int *kk) {}

static void
unknown_command(int c) {
	printf(("%c: unknown command\n"), c);
}

int DISK_EnableDMA(char * device, int flag)
{
	static unsigned long  dma      = 0;

	int fd1;
		
	fd1 = open ( device,   O_RDONLY|O_NONBLOCK);
	if (fd1< 0) {
		printf("can't open %s\n", device);
		return -1;
	}

	dma = flag;

	if (ioctl(fd1, HDIO_SET_DMA, dma))
	{
		return -1;
	}

	close(fd1);
	return 1;

}

#ifndef WIN_CHECKPOWERMODE1
#define WIN_CHECKPOWERMODE1     0xE5
#endif
#ifndef WIN_CHECKPOWERMODE2
#define WIN_CHECKPOWERMODE2     0x98
#endif

int check_powermode(int fd)
{
 unsigned char args[4] = {WIN_CHECKPOWERMODE1,0,0,0};
 int state;

 if (ioctl(fd, HDIO_DRIVE_CMD, &args)
	&& (args[0] = WIN_CHECKPOWERMODE2) /* try again with 0x98 */
	&& ioctl(fd, HDIO_DRIVE_CMD, &args)) {
	if (errno != EIO || args[0] != 0 || args[1] != 0) {
	 state = -1; /* "unknown"; */
	} else
	 state = 0; /* "sleeping"; */
 } else {
	state = (args[2] == 255) ? 1 : 0;
 }
 printf(" drive state is: %d\n", state);

 return state;
}

int DISK_DiskSleep(char * device)
{

	#ifndef WIN_STANDBYNOW1
	#define WIN_STANDBYNOW1 0xE0
	#endif
	#ifndef WIN_STANDBYNOW2
	#define WIN_STANDBYNOW2 0x94
	#endif

	#ifndef WIN_SLEEPNOW1
	#define WIN_SLEEPNOW1 0xE6
	#endif
	#ifndef WIN_SLEEPNOW2
	#define WIN_SLEEPNOW2 0x99
	#endif


	int fd1;

	unsigned char args1[4] = {WIN_STANDBYNOW1,0,0,0};
	unsigned char args2[4] = {WIN_STANDBYNOW2,0,0,0};
		
	fd1 = open ( device,   O_RDONLY|O_NONBLOCK);
	if (fd1< 0) {
		printf("can't open %s\n", device);
		return -1;
	}

	if( check_powermode(fd1) == 1 )	
	{	
		if (ioctl(fd1, HDIO_DRIVE_CMD, &args1)
			 && ioctl(fd1, HDIO_DRIVE_CMD, &args2))
		{
			printf(" sleep  failed\n");

			return -1;
		}
	}

	close(fd1);
	return 1;
}

int DiskAwake(char * device)
{
	int fd1;

	char cmd[4] = { 0xe1, 0, 0, 0 };

	fd1 = open( device, O_RDONLY);

	if (fd1 < 0) {
		printf("can't open %s\n", device);
		return -1;
	}
	
	if (ioctl(fd1, HDIO_DRIVE_CMD, cmd))
	{
		printf(" \nAwake disk error!");
		return -1;
	}

	close(fd1);
	
	return 1;

}

int DiskSetUMDMA66(char * device)
{ 

	#ifndef WIN_SETFEATURES
	#define WIN_SETFEATURES 0xEF
	#endif

	int fd1;	
	
	unsigned char args[4] = {WIN_SETFEATURES,0,3,0};
	
	char cmd[4] = { 0xe1, 0, 0, 0 };

	fd1 = open( device, O_RDONLY);

	if (fd1 < 0) {
		printf("can't open %s\n", device);
		return -1;
	}
	
	args[1] = 66;

	if (ioctl(fd1, HDIO_DRIVE_CMD, &args))
	{
		printf(" HDIO_DRIVE_CMD(setxfermode) failed");
		return -1;
	}

	close(fd1);
	
	return 1;
}

void DeletePartition(int disk_id)
{
	int num = 0,j = 1;


	num = disk_info[disk_id].partitionNum;


	
	for(; num >  0; num--)
	{
		deletenumber = disk_info[disk_id].partitionID[ num - 1];

		j = get_existing_partition(1, partitions);  //partitions is Partition number.
						
		delete_partition(j);
	}

	deletenumber = 0;
	
	
}

void AddPartitions(int disk_id, int size)
{
	uint tempcylinder = 0;
	uint addcylinder = 0;
	float fsize = 0;

	#ifdef USE_FILE_IO_SYS
		int add_two_partitions = 0;

		// force set size 10GBytes		

		fsize = 0.1;
	
		if( fsize > ( (double)disk_info[disk_id].size / 1000000000 ) )
		{
			tempcylinder = disk_info[disk_id].cylinder;
		}
		else
		{
			tempcylinder = disk_info[disk_id].cylinder *( fsize/ ( (double)disk_info[disk_id].size / 1000000000 ) );
		}

		patitionselect = 'e';  // add extend patition.
		new_partition();

		patitionselect = 'l'; // add logical patition.

	
		endcylinder = tempcylinder;

		new_partition();
		printf("add 1 partition!\n");
		
		endcylinder = disk_info[disk_id].cylinder;
		new_partition();
		printf("add 2 partition!\n");
		

	#else	

		if( (float)size > ( (double)disk_info[disk_id].size / 1000000000 ) )
		{
			tempcylinder = disk_info[disk_id].cylinder;
		}
		else
		{
			tempcylinder = disk_info[disk_id].cylinder *( (float)size / ( (double)disk_info[disk_id].size / 1000000000 ) );
		}

		patitionselect = 'e';  // add extend patition.
		new_partition();

		patitionselect = 'l'; // add logical patition.

		for( addcylinder = tempcylinder; addcylinder < disk_info[disk_id].cylinder; )
		{
			
			endcylinder = addcylinder;

			new_partition();

			addcylinder += tempcylinder;
			
		}

		addcylinder -= tempcylinder;

		if( ( ( disk_info[disk_id].cylinder - addcylinder)  >  (tempcylinder / 10) ) 
			 &&  ( ( disk_info[disk_id].cylinder - addcylinder) >10 ) )
		{
			endcylinder = disk_info[disk_id].cylinder;
			new_partition();
		}

	#endif
		

}

int DISK_SetDisk(int disk_id, int size)
{

	int j, c;
	int optl = 0, opts = 0;
	char opdev[40];
	char devn[40];
	
	if( DiskNum == 0 )
	{
		printf(" There are no hard disk!");
		return -1;
	}

	if( disk_id < 0 || disk_id > iCurrentDevNumber )
	{
		printf(" wrong disk id!");
		return -1;
	}

	if( size < 10 )
	{	
		printf(" size of setting must bigger than 10 (GB)");
	//	return -1;
	}

	if( size> ( (double)disk_info[disk_id].size / 1000000000 ) )
	{	
		printf(" size of setting  is  too big!");
	//	return -1;
	}

	if( ( (double)disk_info[disk_id].size / 1000000000 ) / size > GDE_MAXPARTITION )
	{
		printf(" size of setting  is too small, it will make so many patitions!");
		return -1;
	}


#if 0
	printf(("This kernel finds the sector size itself - "
		 "-b option ignored\n"));
#else
//	if (user_set_sector_size && argc-optind != 1)
//		printf(("Warning: the -b (set sector size) option should"
//			 " be used with one specified device\n"));
#endif
	
	strcpy(opdev, disk_info[disk_id].devname);

	disk_device = opdev;

	get_boot(fdisk);

	if (osf_label) {
		/* OSF label, and no DOS label */
		printf(("Detected an OSF/1 disklabel on %s, entering "
			 "disklabel mode.\n"),
		       disk_device);
		bselect();
		osf_label = 0;
		/* If we return we may want to make an empty DOS label? */
	}

	DeletePartition(disk_id);

	AddPartitions( disk_id,  size);

	write_table(); 		/* does not return */
	
	if( fd != 0 )
	{
		close(fd);
		fd = 0;
	}

	tryprocpt(); 
	
	return 1;

	
}

void DISK_DiskInfoClear()
{
	int i ;
	
	DiskNum = 0;

	iUsbNum = 0;

	for(i = 0; i < MAXDEVNUM; i++ )
	{
		strcpy(disk_info[i].devname,"");
		disk_info[i].partitionNum = 0;
	}
}

void DISK_GetAllDiskInfo_lock()
{	
	
	iExceptionError = 0;
	tryprocpt();
	if( iExceptionError != 0  )
	{
		DISK_DiskInfoClear();
	}

	#ifdef USE_FILE_IO_SYS
	{
		int i;
		for(i = 0; i < MAXDEVNUM; i++ )
		{
			if( disk_info[i].partitionNum > 2 )
				disk_info[i].partitionNum = 2;
		}
	}
	#endif
}

void DISK_GetAllDiskInfo()
{
	static int ifirst = 0;

	if( ifirst == 0 )
	{
		ifirst = 1;
		pthread_mutex_init(&disk_op_mutex,NULL);
	}

	pthread_mutex_lock( &disk_op_mutex);

	DISK_GetAllDiskInfo_lock();

	pthread_mutex_unlock( &disk_op_mutex);	

}


int  DISK_GetDiskNum()
{
	int disk_num;

	pthread_mutex_lock( &disk_op_mutex);

	disk_num = DiskNum;

	pthread_mutex_unlock( &disk_op_mutex);	


	if( no_disk_flag == 1 )
		return 0;

	return disk_num;
}

/*
int DISK_GetUsbStatus(char * usb_name,int id)
{
	struct dirent **namelist;
	int i,total;
	int word_num;
	char filebuf[300];
	char filename[50];
	FILE * fp_usb = NULL;
	int dev_num = 0;
	int true_usb_num = 0;
	char * s = NULL;

	if( usb_name == NULL )
		return -1;
	
	total = scandir("/proc/scsi",&namelist ,0,alphasort);
	if(total <0)
	printf("scandir");
	else
	{
		for(i=0;i<total;i++)
		{
			if(  strstr(namelist[i]->d_name,"usb-storage") != NULL )
			{
				dev_num++;
				printf("%s\n",namelist[i]->d_name);	

				strcpy(usb_name,"/dev/sda");

				true_usb_num++;

			}
		}
		printf("total = %d true_usb_num = %d dev_name=%s word_num=%d\n",total,true_usb_num,usb_name,word_num);
	}

	return true_usb_num;
}

*/

int DISK_GetUsbStatus(char * usb_name,int id)
{
	struct dirent **namelist;
	int i,total;
	int word_num;
	char filebuf[300];
	char filename[50];
	FILE * fp_usb = NULL;
	int dev_num = 0;
	int true_usb_num = 0;
	char * s = NULL;	

	if( usb_name == NULL )
		return -1;
	
	total = scandir("/proc/scsi",&namelist ,0,alphasort);
	if(total <0)
	printf("scandir");
	else
	{
		for(i=0;i<total;i++)
		{
			if(  strstr(namelist[i]->d_name,"usb-storage") != NULL )
			{
				dev_num++;
				printf("%s\n",namelist[i]->d_name);	

				sprintf(filename,"/proc/scsi/usb-storage-%d/%d",dev_num -1 ,dev_num);

				printf("filename %s\n",filename);
								
				if( (fp_usb = fopen(filename, "r") ) != NULL )
				{
					memset(filebuf,0,300);
				
					word_num = fread(filebuf,1,300,fp_usb);
					
					fclose(fp_usb);

					if( strstr(filebuf,"Yes") != NULL )
					{
						true_usb_num++;
						
						if( dev_num == 1)
							strcpy(usb_name,"/dev/sda");
						else if( dev_num == 2)
							strcpy(usb_name,"/dev/sdb");
						else if( dev_num == 3)
							strcpy(usb_name,"/dev/sdc");
						else if( dev_num == 4)
							strcpy(usb_name,"/dev/sdd");
						else if( dev_num == 5)
							strcpy(usb_name,"/dev/sde");
						else if( dev_num == 6)
							strcpy(usb_name,"/dev/sdf");
					}						
					
				}
				
				
			}
		}
		printf("total = %d true_usb_num = %d dev_name=%s word_num=%d\n",total,true_usb_num,usb_name,word_num);
	}

	return true_usb_num;
}

int DISK_HaveUsb()
{
	int disk_num;

	pthread_mutex_lock( &disk_op_mutex);

	disk_num = iUsbNum;

	pthread_mutex_unlock( &disk_op_mutex);	

	if(disk_num > 0 )
		return 1;
	else
		return 0;
}

GST_DISKINFO DISK_GetUsbInfo()
{
	GST_DISKINFO tmp_info;

	pthread_mutex_lock( &disk_op_mutex);

	tmp_info = usb_info;

	pthread_mutex_unlock( &disk_op_mutex);	


	return tmp_info;
}

GST_DISKINFO DISK_GetDiskInfo(int disk_id)
{
	GST_DISKINFO tmp_info;

	pthread_mutex_lock( &disk_op_mutex);

	tmp_info = disk_info[disk_id];

	//printf("DISK_GetDiskInfo:  %d ==================\n",disk_id);

	pthread_mutex_unlock( &disk_op_mutex);
	
	return tmp_info;
}

GST_DISKINFO * DISK_GetDiskInfoPoint(int disk_id)
{
	return &disk_info[disk_id];
}

int DISK_Mount(char * dev, char * targetDir)
{	
	int	res = 1;
	char cmd[50];
	
	memset(&cmd, 0x00, sizeof(cmd));

	sprintf(cmd, "mount -t vfat %s %s",dev,targetDir);

	res = system(cmd);

	if(res == 0x00)
	{
				
		return res;
	}
	else
	{
				
		return -1;
	}	
	
	
	return -1;
}

int DISK_UnMount(char * dev)
{
	int	res = 1;
	char cmd[50];
	
	memset(&cmd, 0x00, sizeof(cmd));

	sprintf(cmd, "umount  %s ",dev);

	res = system(cmd);

	if(res == 0x00)
	{
		
		return res;
	}
	else
	{
		
		return -1;
	}	
	
	
	return -1;
}


int DISK_FormatUsb( int size)
{

	int j, c;
	int optl = 0, opts = 0;
	char opdev[40];
	char devn[40];


	if( DISK_HaveUsb() == 0 )
	{
		printf("no usb!\n");
		return -1;
	}
	

	if( size < 10 )
	{	
		printf(" size of setting must bigger than 10 (GB)");
	//	return -1;
	}

	if( size> ( (double)usb_info.size / 1000000000 ) )
	{	
		printf(" size of setting  is  too big!");
	//	return -1;
	}

	if( ( (double)usb_info.size / 1000000000 ) / size > GDE_MAXPARTITION )
	{
		printf(" size of setting  is too small, it will make so many patitions!");
		return -1;
	}



	strcpy(opdev, usb_info.devname);

	disk_device = opdev;

	get_boot(fdisk);

	if (osf_label) {
		/* OSF label, and no DOS label */
		printf(("Detected an OSF/1 disklabel on %s, entering "
			 "disklabel mode.\n"),
		       disk_device);
		bselect();
		osf_label = 0;
		/* If we return we may want to make an empty DOS label? */
	}

	
	{
		int num = 0,j = 1;


		num = usb_info.partitionNum;


		
		for(; num >  0; num--)
		{
			deletenumber = usb_info.partitionID[ num - 1];

			j = get_existing_partition(1, partitions);  //partitions is Partition number.
							
			delete_partition(j);
		}

		deletenumber = 0;		
		
	}

	// AddPartitions(int disk_id, int size)
	{
		uint tempcylinder = 0;

		uint addcylinder = 0;

		if( (float)size > ( (double)usb_info.size / 1000000000 ) )
		{
			tempcylinder = usb_info.cylinder;
		}
		else
		{
			tempcylinder = usb_info.cylinder *( (float)size / ( (double)usb_info.size / 1000000000 ) );
		}

		endcylinder = usb_info.cylinder;

		patitionselect = 'p';  // add extend patition.
		new_partition();
	}

	write_table(); 		/* does not return */
	
	if( fd != 0 )
	{
		close(fd);
		fd = 0;
	}

	tryprocpt(); 
	
	return 1;

	
}

int DISK_Set_No_DISK_FLAG(int flag)
{
	if( flag == 1 )
		no_disk_flag = 1;
	else
		no_disk_flag = 0;
}


