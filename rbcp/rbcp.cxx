/*
 *
 *
 */

#include <iostream>
#include <iomanip>

#include <cstdio>
#include <cstring>
//#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>
//#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

//#include "rbcp.h"

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


class RBCP
{
public:
	RBCP();
	RBCP(char *, int);
	virtual ~RBCP();
	int open(char *, int);
	int close();
	int send(char *, int);
	int receive(char *);
	int read(char *, unsigned int, int);
	int write(char *, unsigned int, int);
	void set_timeout(struct timeval *);
	void get_timeout(struct timeval *);
	int server(int);
	int recvres(char *);
protected:
private:
	struct sockaddr_in m_destsockaddr;
	int m_sock = 0;
	struct timeval m_timeout = {3, 0};
	int m_sequence = 0;
	bool m_is_open = false;
};

//static struct sockaddr_in destsockaddr;
//static int sequence = 0;

RBCP::RBCP() : m_sock(0), m_sequence(0)
{
	return;
};

RBCP::RBCP(char *hostname, int port) : m_sock(0), m_sequence(0)
{
	RBCP::open(hostname, port);
	return;
};

RBCP::~RBCP()
{
	if (m_is_open) {
		::close(m_sock);
		m_is_open = false;
	}
	return;
};

void RBCP::set_timeout(struct timeval *time)
{
	m_timeout.tv_sec = time->tv_sec;
	m_timeout.tv_usec = time->tv_usec;
	return;
} 

void RBCP::get_timeout(struct timeval *time)
{
	time->tv_sec = m_timeout.tv_sec;
	time->tv_usec = m_timeout.tv_usec;
	return;
} 

int RBCP::open(char *hostname, int port)
{
	struct addrinfo hints, *ai;
	int status;


	memset((char *)&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	status = getaddrinfo(hostname, NULL, &hints, &ai);
	if (status != 0) {
		fprintf(stderr, "Unknown host %s (%s)\n",
			hostname, gai_strerror(status));
		perror("RBCP::open");
		return -1;
	}

	memset((char *)&m_destsockaddr, 0, sizeof(m_destsockaddr));
	memcpy(&m_destsockaddr, ai->ai_addr, ai->ai_addrlen);
	//m_destsockaddr.sin_addr.s_addr = ai.ai_addr.sin_addr.s_addr;
	//m_destsockaddr.sin_addr.s_addr = inet_addr(dest);
	m_destsockaddr.sin_port = htons(port);
	m_destsockaddr.sin_family = AF_INET;
	freeaddrinfo(ai);

	m_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_sock < 0) {
		perror("RBCP::open socket");
		return -1;
	}
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&m_timeout, sizeof(struct timeval));

	status = connect(m_sock,
		(struct sockaddr *)&m_destsockaddr, sizeof(m_destsockaddr));
	if (status != 0) {
		perror("RBCP::open connect");
		::close(m_sock);
		return -1;
	}

	m_is_open = true;

	return m_sock;
}


int RBCP::close()
{
	m_is_open = false;
	return ::close(m_sock);
}


int RBCP::send(char *buf, int len)
{
	int status;

	status = sendto(m_sock, buf, len, 0,
		(struct sockaddr *)&m_destsockaddr, sizeof(m_destsockaddr));
	if (status != len) {
		perror("RBCP::send sendto");
		return -1;
	}

	return 0;
}

int RBCP::receive(char *buf)
{
	int status;

	//status = recvfrom(m_sock, buf, 2048, 0, NULL, NULL);
	socklen_t len = sizeof(m_destsockaddr);
	status = recvfrom(m_sock, buf, 2048, 0,
		reinterpret_cast<struct sockaddr *>(&m_destsockaddr), &len);

	if (status < 0) {
		perror("RBCP::receive recvfrom (Timeout ?)");
		return -1;
	}

	return status;
}

int RBCP::read(char *buf, unsigned int addr, int len)
{
	struct bcp_header bcp;
#ifdef DEBUG
	struct rbcp_header *rbcp;
#endif
	char *data;
	int status;
	int rlen = 0;

	bcp.type = RBCP_VER;
	bcp.command = RBCP_CMD_RD;
	bcp.address = htonl(addr);
	bcp.length = len;
	bcp.id = m_sequence++;

	status = RBCP::send((char *)&bcp, sizeof(bcp));
	if (status < 0) printf("RBCP::read udp_send: error\n");
	//fprintf(stderr, "#D send\n");
	rlen = RBCP::receive(buf);
	//fprintf(stderr, "#D receive %d\n", rlen);
	data = buf + sizeof(struct rbcp_header);	

#ifdef DEBUG
	rbcp = (struct rbcp_header *)buf;
	if (rlen > 0) {
		int i;
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
#endif

	if (rlen > (int)(sizeof(struct rbcp_header))) {
		//printf("#D rlen %d %d    \n", rlen, sizeof(struct rbcp_header));
		memmove(buf, data, rlen - sizeof(struct rbcp_header));
	} else {
		fprintf(stderr, "RBCP::read: RBCP Read err.\n");
	}

	return rlen;
}

int RBCP::write(char *data, unsigned int addr, int len)
{
	struct bcp_header *bcp;
#ifdef DEBUG
	struct rbcp_header *rbcp;
#endif
	char *bcp_body;
	static char sbuf[2048];
	static char rbuf[2048];
	int status;
	int rlen = 0;

	bcp = (struct bcp_header *)sbuf;
	bcp_body = sbuf + sizeof(struct bcp_header);

	bcp->type = RBCP_VER;
	bcp->command = RBCP_CMD_WR;
	bcp->address = htonl(addr);
	//bcp->length = len + sizeof(struct bcp_header);
	bcp->length = len;
	bcp->id = m_sequence++;

	if (len < 2048 - static_cast<int>(sizeof(struct bcp_header))) {
		memcpy(bcp_body, data, len);
	} else {
		fprintf(stderr, "RBCP::write: Buffer overflow %d\n", len);
		return -1;
	}

	status = RBCP::send((char *)&sbuf, sizeof(struct bcp_header) + len);
	if (status < 0) printf("rbcp_write udp_send: error\n");
	//fprintf(stderr, "#D send\n");
	rlen = RBCP::receive(rbuf);
	//fprintf(stderr, "#D receive %d\n", rlen);
	data = rbuf + sizeof(struct rbcp_header);	

#ifdef DEBUG
	rbcp = (struct rbcp_header *)rbuf;
	if (rlen > 0) {
		int i;
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
#endif

	return rlen;
}


int RBCP::server(int port)
{

	if ((m_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("RBCP::server socket creation failed");
		return -1;
	}

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(port);

	//setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO,
	//	(char *)&m_timeout, sizeof(struct timeval));

	if (bind(m_sock, (const struct sockaddr *)&serveraddr,
		sizeof(serveraddr)) < 0) {
		perror("RBCP::server bind faild");
		return -1;
	}

	m_is_open = true;
	return m_sock;
}


int RBCP::recvres(char *buf)
{
	struct bcp_header *bcph;
	struct rbcp_header *rbcph;
	static char rbuf[512];
	
	bcph = reinterpret_cast<struct bcp_header *>(buf);
	rbcph = reinterpret_cast<struct rbcp_header *>(rbuf);
	char *rdata = rbuf + sizeof(struct rbcp_header); 
	char *data = buf + sizeof(struct bcp_header); 

	unsigned int addr;
	int com, len, id;

	char *mem = new char[1024];

	while (true) {
		int ndata = 0;
		while ((ndata = RBCP::receive(buf)) > 0) {
			std::cout << "# packet length: " << ndata << ", ";
			for (int i = 0 ; i < ndata ; i++) {
				std::cout << std::hex << std::setw(2) <<
					(static_cast<unsigned int>(buf[i]) & 0xff) << " ";
			}
			std::cout << std::endl;
			addr = ntohl(bcph->address);
			com = static_cast<int>(bcph->command);
			len = static_cast<int>(bcph->length);
			id = static_cast<int>(bcph->id);
			rbcph->address = bcph->address;
			rbcph->command = bcph->command;
			rbcph->length = bcph->length;
			rbcph->id = bcph->id;
			int rlen = sizeof(struct rbcp_header) + len;

			std::cout << "# com:" << com << " id:" << id
				<< " addr:" << addr << " len:" << len << std::endl;
			
			if (com == RBCP_CMD_RD) {
				for (int i = 0 ; i < len ; i++) rdata[i] = mem[i + addr];
			}
			if (com == RBCP_CMD_WR) {
				for (int i = 0 ; i < len ; i++) rdata[i] = mem[i + addr] = data[i];
			}

			RBCP::send(rbuf, rlen);

		}
		std::cout << "TO ";
	}

	return 0;
}


#ifdef TEST_MAIN

static const char *default_host = "192.168.10.16";
static int default_port = 4660;
static int seq1 = 0;

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
	//int sock;
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

	RBCP rbcp;
	rbcp.open(hostname, port);

	
	if (rlen > 0) {
		printf("read: 0x%x : %d\n", raddress, rlen);
		rbcp.read(buf, raddress, rlen);
	}
	if (wdata >= 0) {
		printf("write: 0x%x : %x\n", waddress, wdata);
		buf[0] = wdata & 0xff;
		buf[1] = 0;
		status = rbcp.write(buf, waddress, 1);
		if (status <= 0) printf("Write Error %d\n", status);
	}


	if (seq1 == 1) {
		int j;
		for (j = 0 ; j < 128 ; j++) {
			for (i = 0 ; i < 128 ; i++) buf[i] = i;
			buf[0] = j;
			rbcp.write(buf, 0 + 128*j, 128);
		}
	}


	rbcp.close();

	return 0;
}

#endif


#ifdef TEST_SERVER
int main(int argc, char* argv[])
{
	static char buf[1024];
	int port = 8888;
	RBCP  rbcp;
	rbcp.server(port);

	rbcp.recvres(buf);

	#if 0
	while (true) {
		int len = 0;
		while ((len = rbcp.receive(buf)) > 0) {
			std::cout << "# len: " << len << " ";
			for (int i = 0 ; i < len ; i++) {
				std::cout << std::hex << std::setw(2) <<
					(static_cast<unsigned int>(buf[i]) & 0xff) << " ";
			}
			std::cout << std::endl;
		}
		std::cout << "TO ";
	}
	#endif

	return 0;
}
#endif
