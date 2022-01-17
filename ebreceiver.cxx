/*
 *
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

#include "mstopwatch.cxx"
#include "dtfilename.cxx"

const char* g_ebsrv_endpoint = "tcp://localhost:5559";
std::ofstream g_ofs;

int write_data(unsigned char *cdata, int data_size)
{
	static int wcount = 0;
	static mStopWatch sw;

	if (! g_ofs.is_open()) {
		char wfname[128];
		strncpy(wfname, dtfilename(fnhead), 128);
		g_ofs.open(wfname, std::ios::out);
		std::cout << wfname << std::endl;
		sw.start();
	}

	g_ofs.write(
		reinterpret_cast<char *>(cdata), data_size);
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

			std::cout << "# Freq::" << freq << " Throughput: "
			<< wcount << " B " << elapse << " ms "
			<< wspeed << " MiB/s"  << std::endl;

			wcount = 0;
			ebcount = 0;
		}
	}
	ebcount++;


	return 0;
}



int recv_data(char *buf, zmq::socket_t &socket)
{
	
	return 0;
}


int main(int argc, char *argv[])
{
	char host[128];
	int port;
	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-h") && (argc > i)) {
			strncpy(host, argv[i++], 128);
		}
		if ((sargv == "-p")  && (argc > i)) {
			port = strtol(argv[i++], NULL, 0);
		}
	}

	zmq::context_t context(1);

	zmq::socket_t socket(context, ZMQ_PULL);
	socket.connect(g_ebsrv_endpoint);

	int nsize = recv_data(buf, socket);
	write_data(buf, nsize);


	if (g_ofs.is_open())  g_ofs.close();
	
	return  0;
}
