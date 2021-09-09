/*
 *
 */
  
#include <iostream>
#include <string>
#include <sstream>

#include <unistd.h>
  
#include "zmq.hpp"

const char *zport = "tcp://*:8888";

int trig_gen(double freq)
{
	static unsigned int trig_num = 0;
	zmq::context_t context(1);
	zmq::socket_t pubsocket(context, ZMQ_PUB);
	pubsocket.bind(zport);

	while(true) {
		usleep(static_cast<int>(1000000 / freq));

		zmq::message_t msg(sizeof(unsigned int));
		*(reinterpret_cast<unsigned int *>(msg.data())) = trig_num++;
		try {
			pubsocket.send(msg);
		} catch (zmq::error_t &e) {
			std::cerr << "#E send zmq err. " << e.what() << std::endl;
			return -1;
		}
		std::cout << "\r" << trig_num << "  " << std::flush;
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

	trig_gen(freq);

	return 0;
}