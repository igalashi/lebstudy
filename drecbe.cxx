/*
 *
 *
 */

#include <iostream>
#include <string>
#include <sstream>
#include <csignal>

#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include "koltcp.h"
#include "recbe.h"

const int buf_size = 16 * 1024 * 1024;

static bool g_got_sigpipe = false;
void sigpipe_handler(int signum)
{
        std::cerr << "Got SIGPIPE! " << signum << std::endl;
	g_got_sigpipe = true;
        return;
}

int set_signal()
{
        struct sigaction act;

	g_got_sigpipe = false;

        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = sigpipe_handler;
        act.sa_flags |= SA_RESTART;

        if(sigaction(SIGPIPE, &act, NULL) != 0 ) {
		std::cerr << "sigaction(2) error!" << std::endl;
                return -1;
        }

        return 0;
}


static unsigned short int g_nsent = 0;
int gen_dummy(char *buf, int id, int buf_size)
{
	static unsigned int counter = 0;
	const int nsamples = 1;

	int data_len = 48 * sizeof(unsigned short) * 2 * nsamples;
	if (buf_size < static_cast<int>(sizeof(recbe_header)) + data_len) {
		std::cerr << "Too small buffer!!" << std::endl;
		return 0;
	}

	struct recbe_header *header;
	header = reinterpret_cast<struct recbe_header *>(buf);
	header->type = T_RAW_OLD;
	header->id = static_cast<unsigned char>(id & 0xff);
	header->sent_num = htons(g_nsent++);
	header->time = htons(static_cast<unsigned short>(time(NULL) & 0xffff));
	header->trig_count = htonl(counter);

	unsigned short *body;
	body = reinterpret_cast<unsigned short*>(buf + sizeof(recbe_header));

	for (int j = 0 ; j < nsamples ; j++) {
		for (int i = 0 ; i < 48 ; i++) {
			*(body++) = htons(static_cast<unsigned short>(i & 0xffff) + 0xa0);
		}
		for (int i = 0 ; i < 48 ; i++) {
			 *(body++) = htons(0x1000 | static_cast<unsigned short>(i & 0xffff));
		}
	}

	header->len = ntohs(static_cast<unsigned short int>(data_len & 0xffff));
	counter++;

	return data_len + sizeof(recbe_header);
}

int send_data(int id, int port)
{

	try {
		kol::SocketLibrary socklib;

		char *buf = new char[buf_size];

		//int data_size = gen_dummy(buf, id, buf_size);

		g_nsent = 0;
		kol::TcpServer server(port);
		while(true) {
			kol::TcpSocket sock = server.accept();

			while (true) {
				int data_size = gen_dummy(buf, id, buf_size);
				try {
					if (sock.good()) {
						sock.write(buf, data_size);
						std::cout << "." << std::flush;
						usleep(100 * 1000);
					} else {
						std::cout << "sock.good : false" << std::endl;
						sock.close();
						g_nsent = 0;
						break;
					}
				} catch (kol::SocketException &e) {
					std::cout << "sock.write : " << e.what() << std::endl;
					sock.close();
					g_nsent = 0;
					break;
				}
			}
		}
	} catch(kol::SocketException &e) {
		std::cout << "Error " << e.what() << std::endl;
	}

	std::cout << "end of send_data" << std::endl;

	return 0;
}

int main(int argc, char *argv[])
{
	int port = 8024;
	int id = 0;

	for (int i = 0 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if(((sargv == "-p") || (sargv == "--port"))
			&& (argc > i)) {
			std::string param(argv[i + 1]);
			std::istringstream iss(param);
			iss >> port;
		}
		if(((sargv == "-i") || (sargv == "--id"))
			&& (argc > i)) {
			std::string param(argv[i + 1]);
			std::istringstream iss(param);
			iss >> id;
		}
	}
	std::cout << "ID: " << id << "  Port: " << port << std::endl;


	set_signal();
	send_data(id, port);

	return 0;
}
