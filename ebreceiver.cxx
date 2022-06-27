/*
 *
 */
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>

#include <arpa/inet.h>

#include "zmq.hpp"

#include "recbe.h"
#include "packed_event.h"
#include "mstopwatch.cxx"
#include "disp_throughput.cxx"

#if 1
#include "dtfilename.cxx"
#else
const char *dtfilename(const char *name)
{
	static const char fname[] = "/dev/null";
	return fname;
}
#endif

const char *fnhead = "leb";

//const char *g_ebsrv_endpoint = "tcp://localhost:5559";
std::ofstream g_ofs;

bool g_flag_ofile = false;
bool g_flag_ifile = false;
bool g_flag_checkdata = false;


void hex_dump(char *data, int size)
{
	std::cout << std::hex;
	for (int i = 0 ; i < size ; i++) {
		if ((i % 16) == 0) {
			if (i != 0) std::cout << std::endl;
			std::cout
				<< std::setw(4) << std::setfill('0') << i
				<< " : ";
		}
		std::cout << " "
		<< std::setw(2) << std::setfill('0')
		<< (static_cast<unsigned int>(data[i]) & 0xff);
	}
	std::cout << std::dec << std::endl;

	return;
}

int check_data(char *data, int size)
{
	struct packed_event *ev
		= reinterpret_cast<struct packed_event *>(data);

	#if 0
	hex_dump(data, size);
	#endif

	int dlen = ntohl(ev->length);
	std::cout << "#EV Size: " << std::setw(5) << size
		<< " Mag: "  << std::hex << ev->magic
		<< " len: " << std::setw(7) << std::dec << dlen
		<< " id: "  << ntohs(ev->id)
		<< " nodes: " << ntohs(ev->nodes)
		<< " flag: " << ntohl(ev->flag)
		<< std::endl;

	int nnodes = static_cast<int>(ntohs(ev->nodes));
	int nrecbe = 0;
	char *body = data + sizeof(struct packed_event);
	//while (body < (data + size)) {
	for (int i = 0 ; i < nnodes ; i++) {
		//std::cout << "#D " << std::hex << reinterpret_cast<void *>(body)
		//	<< " " << reinterpret_cast<void *>(data)
		//	<< " " << std::dec << body - data << std::endl;
		if ((body - data) >= dlen) {
			std::cout << "#E Data was trancated. "
				<< "Len: " << dlen << " Size: " << size << std::endl;
			break;
		}

		struct recbe_header *hrecbe
			= reinterpret_cast<struct recbe_header *>(body);
		std::cout << "#RECBE " << std::setw(3) << nrecbe;
		std::cout << " Type: " << (static_cast<int>(hrecbe->type) & 0xff);
		std::cout << " id: " << std::setw(3) << (static_cast<int>(hrecbe->id) & 0xff);
		std::cout << " Sent: " << (static_cast<int>(ntohs(hrecbe->sent_num)) & 0xffff);
		std::cout << " Time: " << std::setw(5)
			<< (static_cast<int>(ntohs(hrecbe->time)) & 0xffff);
		std::cout << " Len: " << (static_cast<int>(ntohs(hrecbe->len)) & 0xffff);
		std::cout << " Trig: " << (static_cast<int>(ntohl(hrecbe->trig_count)) & 0xffff);
		std::cout << std::endl;
		int len = static_cast<int>(ntohs(hrecbe->len)) & 0xffff;
		body += len + sizeof(struct recbe_header);
		nrecbe++;
	}

	return 0;
}

#if 0
double mean_array(int *data, int ndata)
{
	long sum = 0;
	long entry = 0;
	for (int i = 0 ; i < ndata ; i++) {
		sum += (i * data[i]);
		entry += data[i];
	}
	double mean = static_cast<double>(sum) / static_cast<double>(entry);

	return mean;
}

void disp_throughput(char *cdata, int data_size)
{
	static long int wcount = 0;
	static mStopWatch sw;
	static bool at_start = true;
	static int stat_flag[2] = {0, 0};
	static int stat_nodes[110];

	struct packed_event *pheader;
	pheader = reinterpret_cast<struct packed_event *>(cdata);


	if (at_start) {
		at_start = false;
		sw.start();

		for (int i = 0 ; i < 110 ; i++) stat_nodes[i] = 0;
	}


	unsigned int flag = ntohl(pheader->flag);
	unsigned int nodes = ntohs(pheader->nodes);
	if (flag == 1) {
		stat_flag[1]++;
	} else {
		stat_flag[0]++;
	}
	stat_nodes[nodes]++;

	#if 0
	double flag_ratio = static_cast<double>(stat_flag[1]) /
			static_cast<double>(stat_flag[0] + stat_flag[1]);
	std::cout << "#D magic: 0x" << std::hex << ntohl(pheader->magic)
		<< " len: " << std::dec << std::setw(8) << ntohl(pheader->length)
		<< " id: " << std::setw(4) << ntohs(pheader->id)
		<< " nodes: " << std::setw(3) << nodes
		<< " flag: " << flag
		<< " Su eff. : " << flag_ratio
		<< std::endl;
	#endif

	wcount += data_size;
	static unsigned int ebcount = 0;
	if ((ebcount % 100) == 0) {
		int elapse = sw.elapse();
		if (elapse >= 10000) {
			sw.start();
			double wspeed = static_cast<double>(wcount)
				/ 1024 / 1024
				* 1000 / static_cast<double>(elapse);
			double freq = ebcount * 1000 / static_cast<double>(elapse);

			double flag_ratio = static_cast<double>(stat_flag[1]) /
				static_cast<double>(stat_flag[0] + stat_flag[1]);
			std::cout << "# Freq: " << freq << " Throughput: "
			<< wcount << " B " << elapse << " ms "
			<< wspeed << " MiB/s"
			<< " Su Eff.: " << flag_ratio
			<< " nodes: " << mean_array(stat_nodes, 105)
			<< std::endl;

			wcount = 0;
			ebcount = 0;

			stat_flag[0] = 0;
			stat_flag[1] = 0;
			for (int i = 0 ; i < 110 ; i++) stat_nodes[i] = 0;
		}
	}
	ebcount++;

	return;
}
#endif

int poll_socket(zmq::socket_t &socket)
{
	zmq::pollitem_t items[8];

	items[0].socket = static_cast<void*>(socket);
	items[0].fd = 0;
	items[0].events = ZMQ_POLLIN;
	items[0].revents = 0;
	items[1].socket = nullptr;
	items[1].fd = 0;
	items[1].events = 0;
	items[1].revents = 0;

	//int rc = zmq::poll(items, 1, -1);
	int rc = zmq::poll(items, 1, 0);

	return rc;
}


int recv_data(zmq::socket_t &socket)
{

	#if 1
	int rc;
	while (!(rc = poll_socket(socket))) {
		//std::cout << rc << std::flush;
		usleep(1);
	}
	#endif

	zmq::message_t message;
	//socket.recv(&message, ZMQ_NOBLOCK);
	socket.recv(&message);
	char *buf = reinterpret_cast<char *>(message.data());
	int nsize = message.size();

	#if 0
	if (nsize > 0) std::cout << "O";
	else std::cout << "-";
	std::cout << std::flush;
	#endif

	#if 0
	std::cout << "#Size: " << nsize << std::endl;
	if (nsize < 32) {
		hex_dump(buf, nsize);
	} else {
		hex_dump(buf, 64);
	}
	#endif
	
	if (g_flag_checkdata) check_data(buf, nsize);
	if (g_flag_ofile) g_ofs.write(buf, nsize);

	disp_throughput(buf, nsize);

	return nsize;
}

int read_file(std::string &file)
{
	std::ifstream ifs(file);
	if(ifs)	{
		std::cout << "file : " << file << std::endl;
	} else {
		std::cout << "file : " << file << " open fail!" << std::endl;
	}

	char *cpheader = new char[sizeof(packed_event)];
	struct packed_event *pheader = reinterpret_cast<struct packed_event *>(cpheader);
	while (ifs) {
		ifs.read(cpheader, sizeof(packed_event));
		if (ifs.eof()) break;

		int len = static_cast<int>(ntohs(pheader->length));
		//int nnode = static_cast<int>(ntohs(pheader->nodes));
		//len = nnode * (6144 + sizeof(struct recbe_header));

		#if 0
		std::cout << "nnode: " << nnode << std::endl;
		hex_dump(cpheader, 32);
		std::cout << std::endl;
		#endif

		char *data = new char[len + sizeof(packed_event)];
		memcpy(data, cpheader, sizeof(packed_event));
		char *body = data + sizeof(packed_event);
		ifs.read(body, len);

		#if 0
		std::cout << "len: " << len << " gcount: " << ifs.gcount() << std::endl;
		std::cout << " top: " << std::endl;
		hex_dump(body, 1024);
		std::cout << " tail: " << std::endl;
		hex_dump(body + len - 6156, 6156);
		#endif

		int nsize = ifs.gcount();
		if (ifs.eof()) break;
		check_data(data, nsize + sizeof(packed_event));

		delete data;
	}

	delete cpheader;

	if (ifs.is_open()) ifs.close();

	return 0;
}

int main(int argc, char *argv[])
{
	std::string host("localhost");
	std::string port("5559");
	std::string file("-");

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-h") && (argc > i)) {
			host = std::string(argv[++i]);
		}
		if ((sargv == "-p")  && (argc > i)) {
			port = std::string(argv[++i]);
		}
		if (sargv == "-o") g_flag_ofile = true;
		if ((sargv == "-i")  && (argc > i)) {
			file = std::string(argv[++i]);
			g_flag_ifile = true;
			g_flag_ofile = false;
		}
		if (sargv == "-c") g_flag_checkdata = true;

	}

	if (g_flag_ifile) {
		read_file(file);
	} else {

		std::string zurl = "tcp://" + host + ":" + port;
		std::cout << "url: " << zurl << std::endl;

		zmq::context_t context(1);
		zmq::socket_t socket(context, ZMQ_PULL);
		socket.connect(zurl);

		if (g_flag_ofile) {
			char wfname[128];
			strncpy(wfname, dtfilename(fnhead), 128);
			g_ofs.open(wfname, std::ios::out);
			std::cout << wfname << std::endl;
		}

		while (true) {
			//int nsize = recv_data(socket);
			recv_data(socket);
			//if (nsize <= 0) break;
		}

		if (g_ofs.is_open())  g_ofs.close();
	
	}
	
	return  0;
}
