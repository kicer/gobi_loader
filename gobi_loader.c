/* Firmware loader for Qualcomm Gobi USB hardware */

/* Copyright 2009 Red Hat <mjg@redhat.com> - heavily based on work done by
 * Alexander Shumakovitch <shurik@gwu.edu>
 *
 * Gobi 2000 support provided by Anssi Hannula <anssi.hannula@iki.fi>
 *
 * crc-ccitt code derived from the Linux kernel, lib/crc-ccitt.c
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ChangeLog */

/* 2013-11-06 <joykicer@gmail.com>
 * Fix make magic words error when build in big-endian system
 *
 * 2014-01-22 <joykicer@gmail.com>
 * Send firmware with 256*1024 bytes per package
 * Read and check device response after send cmd
 * Use 0x7d as an escape character encode package
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(BYTE_ORDER) && !defined(__BYTE_ORDER)
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN
#define __BYTE_ORDER BYTE_ORDER
#endif

#ifndef __BYTE_ORDER
#error Unknown endian type
#endif

static inline uint16_t __swab16(uint16_t x)
{
	return x<<8 | x>>8;
}

static inline uint32_t __swab32(uint32_t x)
{
	return x<<24 | x>>24 |
		(x & (uint32_t)0x0000ff00UL)<<8 |
		(x & (uint32_t)0x00ff0000UL)>>8;
}

static inline uint64_t __swab64(uint64_t x)
{
	return x<<56 | x>>56 |
		(x & (uint64_t)0x000000000000ff00ULL)<<40 |
		(x & (uint64_t)0x0000000000ff0000ULL)<<24 |
		(x & (uint64_t)0x00000000ff000000ULL)<< 8 |
		(x & (uint64_t)0x000000ff00000000ULL)>> 8 |
		(x & (uint64_t)0x0000ff0000000000ULL)>>24 |
		(x & (uint64_t)0x00ff000000000000ULL)>>40;
}


#if __BYTE_ORDER == __BIG_ENDIAN
#define SWAPL16(val) __swab16(val)
#define SWAPL32(val) __swab32(val)
#define SWAPL64(val) __swab64(val)
#else
#define SWAPL16(val) (val)
#define SWAPL32(val) (val)
#define SWAPL64(val) (val)
#endif

char magic1[] = {0x01, 0x51, 0x43, 0x4f, 0x4d, 0x20, 0x68, 0x69,
		 0x67, 0x68, 0x20, 0x73, 0x70, 0x65, 0x65, 0x64, 0x20, 
		 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x63, 0x6f, 0x6c, 0x20,
		 0x68, 0x73, 0x74, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04,
		 0x30, 0xff, 0xff};

//char magic1[] = "QCOM high speed protocol hst\0\0\0\0\x04\x04\x30";

char magic2[] = {0x25, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		 0x00, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff};

char magic3[] = {0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0xff, 0xff};

char magic4[] = {0x25, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		 0x00, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff};

char magic5[] = {0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0xff, 0xff};

char magic6[] = {0x25, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		 0x00, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff};

char magic7[] = {0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0xff, 0xff};

char magic8[] = {0x29, 0xff, 0xff};

/*
 * This mysterious table is just the CRC of each possible byte. It can be
 * computed using the standard bit-at-a-time methods. The polynomial can
 * be seen in entry 128, 0x8408. This corresponds to x^0 + x^5 + x^12.
 * Add the implicit x^16, and you have the standard CRC-CCITT.
 */

uint16_t const crc_ccitt_table[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t crc_ccitt_byte(uint16_t crc, const char c)
{
        return (crc >> 8) ^ crc_ccitt_table[(crc ^ c) & 0xff];
}

/**
 *	crc_ccitt - recompute the CRC for the data buffer
 *	@crc: previous CRC value
 *	@buffer: data pointer
 *	@len: number of bytes in the buffer
 */
uint16_t crc_ccitt(int16_t crc, char const *buffer, size_t len)
{
	while (len--)
		crc = crc_ccitt_byte(crc, *buffer++);
	return crc;
}

void usage (char **argv) {
	printf ("usage: %s [-2000] serial_device firmware_dir\n", argv[0]);
}

#define DATA_ENCODE	1	/* add 0x7e head/tail, do encode */
#define DATA_NOENCODE	0	/* no 0x7e head/tail, no encode */
int qdl_server_send_request(int fd, const char *data, int len, char flag) {
	int i, cnt;
	char buff[64];

	if(data == NULL) return -1;
	if(len < 3) return -1;

	cnt = len;
	memcpy(buff, data, len);

	*(int16_t *)&buff[len-2] = SWAPL16(~crc_ccitt(0xffff, data, len-2)); /* crc */

	if(flag == DATA_ENCODE) { /* do transposition, similar to PPP protocol */
		for(i=0; i<len; i++) {
			switch(buff[i]) {
				case 0x7e:
					buff[i] = 0x7d;
					memmove(buff+i+2, buff+i+1, len-i-1);
					buff[i+1] = 0x5e;
					cnt++;
					break;

				case 0x7d:
					buff[i] = 0x7d;
					memmove(buff+i+2, buff+i+1, len-i-1);
					buff[i+1] = 0x5d;
					cnt++;
					break;
			}
		}
	}

	if(flag == DATA_ENCODE) write(fd, "\x7e", 1);
	write(fd, buff, cnt);
	if(flag == DATA_ENCODE) write(fd, "\x7e", 1);

	return 0;
}

static void die(const char *err) {
	fprintf(stderr, "[QDL ERROR]: %s\n", err);
}

int qdl_server_wait_response(int fd, char code) {
	int len;
	char buff[64];

	len = read(fd, buff, sizeof(buff));

	if(len < 4) { /* 0x7e crc1 crc2 0x7e */
		die("Invalid Length");
		return 1;
	}

	if((buff[0] != 0x7e) || (buff[len-1] != 0x7e)) {
		die("Invalid Package");
		return 2;
	}

	if(buff[1] != code) {
		die("Invalid response Code");
		return 3;
	}

	/* check crc? */

	return 0;
}

#define FW_SIZE_PER_PACKAGE		(256*1024)
int main(int argc, char **argv) {	
	int serialfd;
	int fwfd;
	int len;
	int err;
	int gobi2000 = 0;
	struct termios terminal_data;
	struct stat file_data;
	char *fwdata = malloc(FW_SIZE_PER_PACKAGE);

	if (argc < 3 || argc > 4) {
		usage(argv);
		return -1;
	}

	if (!fwdata) {
		fprintf(stderr, "Failed to allocate memory for firmware\n");
		return -1;
	}

	if (argc == 4) {
		if (!strcmp(argv[1], "-2000")) {
			gobi2000=1;
			magic1[33]++;
			magic1[34]++;
		} else {
			usage(argv);
		}
	}

	serialfd = open(argv[argc-2], O_RDWR);

	if (serialfd == -1) {
		perror("Failed to open serial device: ");
		usage(argv);
		return -1;
	}

	err = chdir(argv[argc-1]);
	if (err) {
		perror("Failed to change directory: ");
		usage(argv);
		return -1;
	}

	fwfd = open("amss.mbn", O_RDONLY);

	if (fwfd == -1) {
		perror("Failed to open firmware: ");
		usage(argv);
		return -1;
	}

	fstat(fwfd, &file_data);
	*(int32_t *)&magic2[2] = SWAPL32(file_data.st_size - 8);
	*(int32_t *)&magic3[7] = SWAPL32(file_data.st_size - 8);

	tcgetattr (serialfd, &terminal_data);
	cfmakeraw (&terminal_data);
	tcsetattr (serialfd, TCSANOW, &terminal_data);

	qdl_server_send_request(serialfd, magic1, sizeof(magic1), DATA_ENCODE);
	qdl_server_wait_response(serialfd, 0x02);

	qdl_server_send_request(serialfd, magic2, sizeof(magic2), DATA_ENCODE);
	qdl_server_wait_response(serialfd, 0x26);
	qdl_server_send_request(serialfd, magic3, sizeof(magic3), DATA_NOENCODE);

	while (1) {
		len = read (fwfd, fwdata, FW_SIZE_PER_PACKAGE);
		if (len == FW_SIZE_PER_PACKAGE)
			write (serialfd, fwdata, FW_SIZE_PER_PACKAGE);
		else {
			write (serialfd, fwdata, len-8);
			break;
		}
		write (serialfd, fwdata, 0);
	}
	qdl_server_wait_response(serialfd, 0x28);
	printf("QDL amss.mbn finish\n");

	fwfd = open("apps.mbn", O_RDONLY);

	if (fwfd == -1) {
		perror("Failed to open secondary firmware: ");
		usage(argv);
		return -1;
	}

	fstat(fwfd, &file_data);
	*(int32_t *)&magic4[2] = SWAPL32(file_data.st_size);
	*(int32_t *)&magic5[7] = SWAPL32(file_data.st_size);

	qdl_server_send_request(serialfd, magic4, sizeof(magic4), DATA_ENCODE);
	qdl_server_wait_response(serialfd, 0x26);
	qdl_server_send_request(serialfd, magic5, sizeof(magic5), DATA_NOENCODE);

	while (1) {
		len = read (fwfd, fwdata, FW_SIZE_PER_PACKAGE);
		if (len == FW_SIZE_PER_PACKAGE)
			write (serialfd, fwdata, FW_SIZE_PER_PACKAGE);
		else {
			write (serialfd, fwdata, len);
			break;
		}
		write (serialfd, fwdata, 0);
	}
	qdl_server_wait_response(serialfd, 0x28);
	printf("QDL apps.mbn finish\n");

	if (gobi2000) {
		fwfd = open("UQCN.mbn", O_RDONLY);

		if (fwfd == -1)
			fwfd = open("uqcn.mbn", O_RDONLY);

		if (fwfd == -1) {
			perror("Failed to open tertiary firmware: ");
			usage(argv);
			return -1;
		}

		fstat(fwfd, &file_data);
		*(int32_t *)&magic6[2] = SWAPL32(file_data.st_size);
		*(int32_t *)&magic7[7] = SWAPL32(file_data.st_size);

		qdl_server_send_request(serialfd, magic6, sizeof(magic6), DATA_ENCODE);
		qdl_server_wait_response(serialfd, 0x26);
		qdl_server_send_request(serialfd, magic7, sizeof(magic7), DATA_NOENCODE);

		while (1) {
			len = read (fwfd, fwdata, FW_SIZE_PER_PACKAGE);
			if (len == FW_SIZE_PER_PACKAGE)
				write (serialfd, fwdata, FW_SIZE_PER_PACKAGE);
			else {
				write (serialfd, fwdata, len);
				break;
			}
			write (serialfd, fwdata, 0);
		}
		qdl_server_wait_response(serialfd, 0x28);
		printf("QDL uqcn.mbn finish\n");
	}

	qdl_server_send_request(serialfd, magic8, sizeof(magic8), DATA_ENCODE);
	printf("QDL success\n");

	return 0;
}
