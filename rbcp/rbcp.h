/************************************************
*						*
* SiTCP Bus Control Protocol			*
* Header file					*
*						*
* 2009/08/14 Tomohisa Uchida			*
* 2010/01/12 modified Igarashi			*
*						*
*************************************************/

#ifndef _RBCP_H
#define _RBCP_H

struct bcp_header{
	unsigned char type;
	unsigned char command;
	unsigned char id;
	unsigned char length;
	unsigned int address;
};

struct rbcp_header {
	unsigned char type;
	unsigned char command;
	unsigned char id;
	unsigned char length;
	unsigned int address;
};

#define RBCP_VER 0xFF
#define RBCP_CMD_WR 0x80
#define RBCP_CMD_RD 0xC0

#endif
