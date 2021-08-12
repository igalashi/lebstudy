/*
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#include "rbcp.h"


static char *default_host = "192.168.10.16";
static int default_port = 4660;
static struct sockaddr_in destsockaddr;
static int sequence = 0;

static int seq1 = 0;

int udpreg_open(char *hostname, int port)
{
	//struct sockaddr_in destsockaddr;
	struct addrinfo hints, *ai;
	int sock;
	int status;

	struct timeval timeout={3, 0};

	memset((char *)&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	status = getaddrinfo(hostname, NULL, &hints, &ai);
	if (status != 0) {
		fprintf(stderr, "Unknown host %s (%s)\n",
			hostname, gai_strerror(status));
		perror("udpreg_open");
		return -1;
	}

	memset((char *)&destsockaddr, 0, sizeof(destsockaddr));
	memcpy(&destsockaddr, ai->ai_addr, ai->ai_addrlen);
	//destsockaddr.sin_addr.s_addr = ai.ai_addr.sin_addr.s_addr;
	//destsockaddr.sin_addr.s_addr = inet_addr(dest);
	destsockaddr.sin_port = htons(port);
	destsockaddr.sin_family = AF_INET;
	freeaddrinfo(ai);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("udpreg_open socket");
		return -1;
	}
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&timeout, sizeof(struct timeval));

	status = connect(sock,
		(struct sockaddr *)&destsockaddr, sizeof(destsockaddr));
	if (status != 0) {
		perror("udpreg_open connect");
		close(sock);
		return -1;
	}


	return sock;
}

int udpreg_send(int sock, char *buf, int len)
{
	int status;

	status = sendto(sock, buf, len, 0,
		(struct sockaddr *)&destsockaddr, sizeof(destsockaddr));
	if (status != len) {
		perror("udpreg_send sendto");
		return -1;
	}

	return 0;
}

int udpreg_receive(int sock, char *buf)
{
	int status;
	//socklen_t fromlen;

	//status = *len = recvfrom(sock, buf, 1024, 0, NULL, &fromlen);
	status = recvfrom(sock, buf, 2048, 0, NULL, NULL);
	
	if (status < 0) {
		perror("udpreg_receive recvfrom (Timeout ?)");
		return -1;
	}
	return status;
}

int udpreg_close(int sock)
{
	return close(sock);
}

int updreg_wait_ans(int sock)
{
	return 0;
}

int udpreg_read(int sock, char *buf, unsigned int addr, int len)
{
	struct bcp_header bcp;
	struct rbcp_header *rbcp;
	char *data;
	int status;
	int rlen = 0;
	int i;

	bcp.type = RBCP_VER;
	bcp.command = RBCP_CMD_RD;
	bcp.address = htonl(addr);
	bcp.length = len;
	bcp.id = sequence++;

	status = udpreg_send(sock, (char *)&bcp, sizeof(bcp));
	if (status < 0) printf("udpreg_read udp_send: error\n");
	//fprintf(stderr, "#D send\n");
	rlen = udpreg_receive(sock, buf);
	//fprintf(stderr, "#D receive %d\n", rlen);
	rbcp = (struct rbcp_header *)buf;
	data = buf + sizeof(struct rbcp_header);	

	if (rlen > 0) {
		printf("type: 0x%x, command: 0x%x, id: 0x%x, length: %d, address: %d",
			rbcp->type & 0xff, rbcp->command& 0xff,
			rbcp->id & 0xff, rbcp->length & 0xff,
			ntohl(rbcp->address & 0xffffffff));
	
		for (i = 0 ; i < (int)(rlen - sizeof(struct rbcp_header)) ; i++) {
			if ((i % 8) == 0) printf("\n%04x: ", addr + i);
			printf("%02x ", data[i] & 0xff);
		}
		printf("\n");
	}

	return 0;
}

int udpreg_write(int sock, char *data, unsigned int addr, int len)
{
	struct bcp_header *bcp;
	struct rbcp_header *rbcp;
	char *bcp_body;
	static char sbuf[2048];
	static char rbuf[2048];
	int status;
	int rlen = 0;
	int i;

	bcp = (struct bcp_header *)sbuf;
	bcp_body = sbuf + sizeof(struct bcp_header);

	bcp->type = RBCP_VER;
	bcp->command = RBCP_CMD_WR;
	bcp->address = htonl(addr);
	//bcp->length = len + sizeof(struct bcp_header);
	bcp->length = len;
	bcp->id = sequence++;

	if (len < 2048 - sizeof(struct bcp_header)) {
		memcpy(bcp_body, data, len);
	} else {
		fprintf(stderr, "udpreg_write: Buffer overflow %d\n", len);
		return -1;
	}

	status = udpreg_send(sock, (char *)&sbuf, sizeof(struct bcp_header) + len);
	if (status < 0) printf("udpreg_write udp_send: error\n");
	//fprintf(stderr, "#D send\n");
	rlen = udpreg_receive(sock, rbuf);
	//fprintf(stderr, "#D receive %d\n", rlen);
	rbcp = (struct rbcp_header *)rbuf;
	data = rbuf + sizeof(struct rbcp_header);	

	if (rlen > 0) {
		printf("type: 0x%x, command: 0x%x, id: 0x%x, length: %d, address: %d",
			rbcp->type & 0xff, rbcp->command& 0xff,
			rbcp->id & 0xff, rbcp->length & 0xff,
			ntohl(rbcp->address & 0xffffffff));

		for (i = 0 ; i < (int)(rlen - sizeof(struct rbcp_header)) ; i++) {
			if ((i % 8) == 0) printf("\n%04x: ", addr + i);
			printf("%02x ", data[i] & 0xff);
		}
		printf("\n");
	}

	return rlen;
}

void helpline(char *cname)
{
	printf("%s <Commads>\n", cname);
	printf("Commands \n");
	printf("--host=<hotname>\n");
	printf("--port=<port nmber>\n");
	printf("--read=<address>:<number of reading>\n");
	printf("--write=<addres>:<data>\n");

	return;
}

int main(int argc, char* argv[])
{
	int sock;
	//struct bcp_header bcp;
	static char buf[1024];
	//int rlen;
	char hostname[256];
	int port;
	unsigned int raddress, rlen;
	unsigned int waddress;
	int wdata;
	int status;
	int i;

	strcpy(hostname, default_host);
	port = default_port;

	wdata = -1;
	rlen = 0;
	for (i = 1 ; i < argc ; i++) {
		if (sscanf(argv[i], "--host=%s", hostname) == 1) ;
		if (sscanf(argv[i], "--port=%d", &port) == 1) ;
		if (sscanf(argv[i], "--read=%x:%x", &raddress, &rlen) == 1);
		if (sscanf(argv[i], "--write=%x:%x", &waddress, &wdata) == 1);
		if (strncmp(argv[i], "--seq1", 6) == 0) seq1 = 1;
	}

	printf("host: %s\n", hostname);
	sock = udpreg_open(hostname, port);

	
	if (rlen > 0) {
		printf("read: 0x%x : %d\n", raddress, rlen);
		udpreg_read(sock, buf, raddress, rlen);
	}
	if (wdata >= 0) {
		printf("write: 0x%x : %x\n", waddress, wdata);
		buf[0] = wdata & 0xff;
		buf[1] = 0;
		status = udpreg_write(sock, buf, waddress, 1);
		if (status <= 0) printf("Write Error %d\n", status);
	}


	if (seq1 == 1) {
		int j;
		for (j = 0 ; j < 128 ; j++) {
			for (i = 0 ; i < 128 ; i++) buf[i] = i;
			buf[0] = j;
			udpreg_write(sock, buf, 0 + 128*j, 128);
		}
	}


	udpreg_close(sock);

	return 0;
}
