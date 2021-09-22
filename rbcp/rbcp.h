/************************************************
*						*
* SiTCP Bus Control Protocol			*
* Header file					*
*						*
* 2009/08/14 Tomohisa Uchida			*
* 2010/01/12 modified Igarashi			*
*						*
*************************************************/

#ifndef RBCP_HEADER
#define RBCP_HEADER

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

#ifdef __cplusplus
extern "C" {
#endif

int rbcp_open(char *, int);
int rbcp_send(int, char *, int);
int rbcp_receive(int, char *);
int rbcp_close(int);
int rbcp_read(int, char *, unsigned int, int);
int rbcp_write(int, char *, unsigned int, int);

#ifdef __cplusplus
}
#endif


#endif
