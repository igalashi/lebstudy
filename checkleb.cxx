//
//
//

#include <iostream>
#include <iomanip>
#include <fstream>

//#include <cstdio>
//#include <csignal>

//#include <unistd.h>
//#include <time.h>
#include <arpa/inet.h>

#include "packed_event.h"
#include "recbe.h"

#define uint32_t unsigned int

bool g_flag_disp_recbe_header = false;

void ntoh_header(struct recbe_header *host, struct recbe_header *raw)
{
	host->type = raw->type;
	host->id = raw->id;
	host->sent_num = ntohs(raw->sent_num);
	host->time = ntohs(raw->time);
	host->len = ntohs(raw->len);
	host->trig_count = ntohl(raw->trig_count);

	return;
}

#if 0
int decode_tdc(unsigned int *data)
{
	return 0;
}
#endif

int reading()
{
	int nread = 0;

	struct packed_event event;
	char *cevent = reinterpret_cast<char*>(&event);

	int bufsize = 100000;
	char *ebody = new char [bufsize];

	while (! std::cin.eof()) {
		std::cin.read(cevent, sizeof(struct packed_event));
		nread += std::cin.gcount();
		
		int len = ntohl(event.length);
		int nnode = ntohs(event.nodes);
		std::cout << "Magic: 0x" << std::hex << std::setw(2) << static_cast<unsigned int>(event.magic)
			  << "  Length:" << std::dec << std::setw(7) << std::setfill(' ') << len
			  << "  id:" << std::dec << std::setw(4) << ntohs(event.id)
			  << "  nodes:" << std::dec << std::setw(4) << std::setfill(' ') << nnode
			  << "  flags: 0x" << std::hex << std::setw(6) << std::setfill('0') << ntohl(event.flag)
			  << std::endl;

		if (bufsize < len) {
			delete [] ebody;
			bufsize = len * 2;
			try {
				ebody = new char[bufsize];
			} catch (std::bad_alloc &e) {
				std::cerr << "Buffer allocation fail. " << e.what();
				return -1;
			}
		}

		int lnread = 0;
		while (lnread < len) {
			std::cin.read(ebody, len);
			nread += std::cin.gcount();
			lnread += std::cin.gcount();
			if (std::cin.eof()) break;
		}
		//std::cout << "N read: " << nread << " EOF: " << std::cin.eof() << std::endl;

		if (g_flag_disp_recbe_header) {
			char *buf = ebody;
			for (int i = 0 ; i < nnode ; i++) {
				struct recbe_header header;
				struct recbe_header *pheader_raw;
				pheader_raw = reinterpret_cast<struct recbe_header *>(buf);
				#if 0
				unsigned short int *body
					= reinterpret_cast<unsigned short int *>
						(buf + sizeof(struct recbe_header));
				#endif
	
				ntoh_header(&header, pheader_raw);	

				std::cout << "Type 0x:" << std::hex << std::setw(2)
					<< static_cast<unsigned int>(header.type)
					<< "  id 0x:" << std::hex << std::setw(2)
					<< static_cast<unsigned int>(header.id)
					<< "  Sent:" << std::dec << std::setw(6) << header.sent_num
					<< "  Time:" << std::setw(6) << header.time
					<< "  Len:" << std::setw(6) << header.len
					<< " Trigger:" << std::setw(6) << header.trig_count
					<< std::endl;

					buf += header.len + sizeof(struct recbe_header);
			}
		}
		

	}


	return 0;
}


int main(int argc, char* argv[])
{

	for (int i = 0 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-r")
			|| (sargv == "--show-recbe-header")) {
			g_flag_disp_recbe_header = true;
		}
	}

	reading();	
	
	return 0;
}

