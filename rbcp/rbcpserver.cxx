/*
 *
 *
 */

#include <iostream>
#include <iomanip>

#include <cstdio>
#include <cstring>
#include <unistd.h>
//#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#include "rbcp.cxx"

class RBCPServer : public RBCP
{
public:
	RBCPServer();
	RBCPServer(int);
	virtual ~RBCPServer();
	int server(int);
	//int server_read(char *, unsigned int, int);
	//int server_write(char *, unsigned int, int);

	int server_recv_to_reg_mem(char *);
	void init_reg_mem(unsigned int);
	void fina_reg_mem();
	void set_reg_mem(char *mem) {m_reg_mem = mem;};
	char* get_reg_mem() {return m_reg_mem;};
protected:
private:
	char *m_reg_mem = nullptr;
	unsigned int m_reg_mem_size = 0;
};

RBCPServer::RBCPServer() : RBCP::RBCP()
{
	return;
}

RBCPServer::RBCPServer(int port) : RBCP::RBCP()
{
	RBCPServer::server(port);
	return;
}

RBCPServer::~RBCPServer()
{
	if (m_is_open) {
		::close(m_sock);
		m_is_open = false;
	}
	return;
}

int RBCPServer::server(int port)
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

	#if 1 //TIMEOUT
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&m_timeout, sizeof(struct timeval));
	#endif

	if (bind(m_sock, (const struct sockaddr *)&serveraddr,
		sizeof(serveraddr)) < 0) {
		perror("RBCP::server bind faild");
		return -1;
	}

	m_is_open = true;
	return m_sock;
}


void RBCPServer::init_reg_mem(unsigned int size)
{
	m_reg_mem = new char[size];
	m_reg_mem_size = size;

	return;
}

void RBCPServer::fina_reg_mem()
{
	delete m_reg_mem;
	m_reg_mem_size = 0;

	return;
}

int RBCPServer::server_recv_to_reg_mem(char *buf)
{
	struct bcp_header *bcph;
	struct rbcp_header *rbcph;
	static char rbuf[256 + sizeof(struct bcp_header)];
	
	bcph = reinterpret_cast<struct bcp_header *>(buf);
	rbcph = reinterpret_cast<struct rbcp_header *>(rbuf);
	char *data = buf + sizeof(struct bcp_header); 
	char *rdata = rbuf + sizeof(struct rbcp_header); 

	unsigned int addr;
	int com, len, id;

	int ndata = 0;
	while (true) {
		if ((ndata = RBCPServer::receive(buf)) > 0) {
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

			#if 1
			std::cout << "# com:" << com << " id:" << id
				<< " addr:" << addr << " len:" << len << std::endl;
			std::cout << "Data: ";
			for (unsigned int i = 0 ; i < ndata - sizeof(struct bcp_header); i++)
				std::cout << std::hex << " "
				<< (static_cast<unsigned int>(data[i]) & 0xff);
			std::cout << std::endl;
			#endif
			
			if (com == RBCP_CMD_RD) {
				for (int i = 0 ; i < len ; i++) rdata[i] = m_reg_mem[i + addr];
			}
			if (com == RBCP_CMD_WR) {
				for (int i = 0 ; i < len ; i++) rdata[i] = m_reg_mem[i + addr] = data[i];
			}

			RBCPServer::send(rbuf, rlen);
			break;

		} else {
			//std::cout << "TO " << std::flush;
		}
	}

	return ndata;
}


#if 0
int main(int argc, char* argv[])
{
	static char buf[1024];
	int port = 8888;
	RBCPServer  rbcp;
	rbcp.server(port);

	std::cout << "Port :" << port << std::endl;

	//char *mem = new char[1024];
	//rbcp.set_reg_mem(mem);
	rbcp.init_reg_mem(128*128);
	while (true) {
		rbcp.server_recv_to_reg_mem(buf);
	}
	rbcp.fina_reg_mem();

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


#ifdef TEST_SERVER

void structdump(char *data, int len)
{
	struct erbcp_header *erbcph = reinterpret_cast<struct erbcp_header *>(data);
	struct rbcp_header *rbcph = reinterpret_cast<struct rbcp_header *>(data);
	int lconte = len - sizeof(rbcp_header);
	char *conte = data + sizeof(rbcp_header);

	std::cout << "Type: 0x" << std::hex << static_cast<unsigned int>(rbcph->type)
		<< " Com: 0x" << static_cast<unsigned int>(rbcph->command)
		<< " Id: 0x" <<  static_cast<unsigned int>(rbcph->id)
		<< " len: 0x" <<  static_cast<unsigned int>(rbcph->length)
		<< " addr: 0x" << rbcph->address
		<< std::endl;
	if (erbcph->type != RBCP_VER) {
		lconte = len - sizeof(erbcp_header);
		conte = data + sizeof(erbcp_header);
		std::cout << "Dest_ip: 0x" << std::hex << ntohl(erbcph->dest_ip)
			<< " Dest_port: " << std::dec << ntohl(erbcph->dest_port)
			<< std::endl;
	}

	for (int i = 0 ; i < lconte ; i++) {
		if (((i % 16) == 0) && (i != 0)) std::cout << std::endl;
		if ((i % 16) == 0) std::cout << "Data: " << std::hex << std::setw(4) << i << " : ";
		std::cout << " " << std::hex
			<< (static_cast<unsigned int>(conte[i]) & 0xff);
	}
	std::cout << std::endl;

	return;
}

int main(int argc, char* argv[])
{
	static char buf[1024];
	int port = 8888;

	for (int i = 0 ; i < argc ; i++) {
                if (sscanf(argv[i], "--port=%d", &port) == 1);
	}

	RBCPServer  rbcp;
	rbcp.server(port);

	std::cout << "Port :" << port << std::endl;

	unsigned int reg_mem_size = 128 * 128;
	char *reg_mem = new char[reg_mem_size];

	while (true) {
		struct bcp_header *bcph;
		struct rbcp_header *rbcph;
		static char rbuf[256 + sizeof(struct bcp_header)];
	
		bcph = reinterpret_cast<struct bcp_header *>(buf);
		rbcph = reinterpret_cast<struct rbcp_header *>(rbuf);
		char *data = buf + sizeof(struct bcp_header); 
		char *rdata = rbuf + sizeof(struct rbcp_header); 

		unsigned int addr;
		int com, len;
		//int id;

		int ndata = 0;
		while (true) {
			if ((ndata = rbcp.receive(buf)) > 0) {
				#if 0
				std::cout << "# packet length: " << ndata << ", ";
				for (int i = 0 ; i < ndata ; i++) {
					std::cout << std::hex << std::setw(2) <<
						(static_cast<unsigned int>(buf[i]) & 0xff) << " ";
				}
				std::cout << std::endl;
				#endif

				addr = ntohl(bcph->address);
				com = static_cast<int>(bcph->command);
				len = static_cast<int>(bcph->length);
				//id = static_cast<int>(bcph->id);

				rbcph->type = bcph->type;
				rbcph->address = bcph->address;
				rbcph->command = bcph->command;
				rbcph->length = bcph->length;
				rbcph->id = bcph->id;
				int rlen = sizeof(struct rbcp_header) + len;

				#if 0
				std::cout << "#R ndata: " << ndata << " rlen: " << rlen << std::endl;
				std::cout << "#R com:" << com << " id:" << id
					<< " addr:" << addr << " len:" << len << std::endl;
				std::cout << "#R Data: ";
				for (unsigned int i = 0 ; i < ndata - sizeof(struct bcp_header) ; i++)
					std::cout << std::hex << " "
					<< (static_cast<unsigned int>(data[i]) & 0xff);
				std::cout << std::endl;
				#endif
				std::cout << "#Receive ndata: " << ndata << " rlen: " << rlen << std::endl;
				structdump(buf, ndata);

				if (com == RBCP_CMD_RD) {
					for (int i = 0 ; i < len ; i++)
						rdata[i] = reg_mem[i + addr];
				}
				if (com == RBCP_CMD_WR) {
					for (int i = 0 ; i < len ; i++)
						rdata[i] = reg_mem[i + addr] = data[i];
				}

				#if 0
				std::cout << "#S rlen: " << rlen
					<< " com:"  << static_cast<unsigned int>(rbcph->command)
					<< " id:"   << static_cast<unsigned int>(rbcph->id)
					<< " addr:" << static_cast<unsigned int>(rbcph->address)
					<< " len:"  << static_cast<unsigned int>(rbcph->length)
					<< std::endl;
				std::cout << "#S Data: ";
				for (int i = 0 ; i < len ; i++)
					std::cout << std::hex << " "
					<< (static_cast<unsigned int>(rdata[i]) & 0xff);
				std::cout << std::endl;
				#endif
				std::cout << "#Send" << std::endl;
				structdump(rbuf, rlen);

				rbcp.send(rbuf, rlen);
				break;

			} else {
				//std::cout << "TO " << std::flush;
			}
		}

	}
	delete reg_mem;
	reg_mem_size = 0;

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
