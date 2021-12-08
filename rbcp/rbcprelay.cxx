/*
 *
 */

#include <iostream>
#include <iomanip>

#include "rbcpserver.cxx"

void hexdump(char *data, int len, const char *key)
{
	
	for (int i = 0 ; i < len ; i++) {
		if (((i % 16) == 0) && (i != 0)) std::cout << std::endl;
		if ((i % 16) == 0) std::cout << key << std::hex << std::setw(4) << i << " : ";
		std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
			<< (static_cast<unsigned int>(data[i]) & 0xff);
	}
	std::cout << std::dec << std::endl;

	return;
}

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

int relay(RBCPServer &serv)
{

	struct ebcp_header *ebcph;
	struct erbcp_header *erbcph;
	struct bcp_header *bcph;
	struct rbcp_header *rbcph;
	static char buf[256 + sizeof(struct ebcp_header)];
	static char rbuf[256 + sizeof(struct ebcp_header)];
	
	ebcph = reinterpret_cast<struct ebcp_header *>(buf);
	bcph = reinterpret_cast<struct bcp_header *>(buf);
	erbcph = reinterpret_cast<struct erbcp_header *>(rbuf);
	rbcph = reinterpret_cast<struct rbcp_header *>(rbuf);
	char *edata = buf + sizeof(struct ebcp_header); 
	char *data = buf + sizeof(struct bcp_header); 
	char *erdata = rbuf + sizeof(struct erbcp_header); 
	char *rdata = rbuf + sizeof(struct rbcp_header); 


	RBCP rbcp;
	//rbcp.set_timeout(3);
	
	while (true) {

		int ndata = 0;
		ndata = serv.receive(buf);
		if (ebcph->type == RBCP_VER) {
			serv.send(buf, ndata);
			
		} else if (ebcph->type == RBCP_TYPE_HOP) {
			unsigned int hop_ip = ntohl(ebcph->dest_ip);
			char *chop_ip = RBCP::ip_uint2char(hop_ip);
			unsigned int hop_port = ntohl(ebcph->dest_port);

			std::cout << "Dest IP: " << chop_ip << " Port: " << hop_port << std::endl;
			hexdump(buf, ndata, "R  ");

			int status = rbcp.open(chop_ip, hop_port);
			if (status < 0) {
				perror("relay: RBCP open err.");
				rbcp.close();
				continue;
			}

			bcph->type = RBCP_VER;
			for (int i = 0 ; i < static_cast<int>(ebcph->length) ; i++) {
				data[i] = edata[i];
			}
			int len = ndata + (sizeof(struct bcp_header) - sizeof(struct ebcp_header));

			hexdump(buf, len, "HS ");

			status = rbcp.send(buf, len);
			if (status != len) {
				perror("relay: Hop send err.");
				return -1;
			}
			int rlen = rbcp.receive(rbuf);
			if (rlen <= 0) {
				perror("relay: Hop receive err.");
				return rlen;
			}

			hexdump(rbuf, rlen, "HR ");

			rbcp.close();


			erbcph = reinterpret_cast<struct erbcp_header *>(rbuf);
			rbcph = reinterpret_cast<struct rbcp_header *>(rbuf);
			int lcontainer = static_cast<int>(rbcph->length);
			for (int i = 0 ; i < lcontainer ; i++) {
				erdata[lcontainer - i - 1] = rdata[lcontainer - i - 1];
			}
			erbcph->dest_ip = htonl(hop_ip);
			erbcph->dest_port= htonl(hop_port);
			erbcph->type = RBCP_TYPE_HOP;
			int elen = rlen + (sizeof(struct erbcp_header) - sizeof(struct rbcp_header));

			hexdump(rbuf, elen, "S  ");
			structdump(rbuf, elen);

			status = serv.send(rbuf, elen);
			if (status < 0) {
				perror("relay: erbcp return back err.");
			}

		}

	}

	return 0;
}

int main(int argc, char* argv[])
{

	int port = 8888;

	std::cout << "Port: " << port << std::endl;

	RBCPServer serv(port);
	relay(serv);
	serv.close();

	return 0;
}
