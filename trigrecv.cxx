/*
 *
 */
  
#include <iostream>
#include <string>
#include <sstream>

#include <unistd.h>
  
#include "zmq.hpp"

const char *zport = "tcp://localhost:8888";

int trig_recv()
{
	zmq::context_t context(1);
	zmq::socket_t subsocket(context, ZMQ_SUB);
	subsocket.connect(zport);
	subsocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);

	while(true) {
		zmq::message_t msg;
		//bool rc;
		try {
			subsocket.recv(&msg);
			//rc = subsocket.recv(&msg, ZMQ_NOBLOCK);
		} catch (zmq::error_t &e) {
			std::cerr << "#E recv zmq err. " << e.what() << std::endl;
			return -1;
		}
		#if 0
		if (! rc) {
			usleep(100);
			continue;
		} 
		#endif

		unsigned int trig = *(reinterpret_cast<unsigned int *>(msg.data()));
		std::cout << "got " << trig << " size: " << msg.size() << std::endl;

	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	double freq = 100.;

	for (int i = 0 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if (((sargv == "-f") || (sargv == "--freq"))
			&& (argc > i)) {
			std::string param(argv[i+1]);
			std::istringstream iss(param);
			iss >> freq;
		}
	}

	trig_recv();

	return 0;
}