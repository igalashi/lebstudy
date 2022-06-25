/*
 *
 */

#include <iostream>
#include <string>
#include <cstring>
#include "zmq.hpp"

#define COMMAND_PORT "tcp://localhost:5560"
#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "5560"

int command_loop(zmq::context_t &context, std::string &com_url)
{

	zmq::socket_t socket(context, ZMQ_PUSH);
	socket.connect(com_url);

        while (true) {
                std::string oneline;
                std::cout << "> ";
                std::cin >> oneline;

		#if 0
                if (oneline == "run") eb->set_state(SM_RUNNING);
                if (oneline == "idle") eb->set_state(SM_IDLE);
                if (oneline == "stop") eb->set_state(SM_IDLE);
                if (oneline == "init") init_sequence(eb, tasks);
                if (oneline == "end") eb->set_state(SM_END);

                if ((oneline == "state")
                        || (oneline == "status")
                        || (oneline == "stat")) {
                        print_state(eb->get_state()) ;
                        std::cout << std::endl;
                }
                if (oneline == "mon") eb->monitor();

                if (oneline == "exit") break;
                if (oneline == "quit") break;
		#endif

                if (oneline == "q") break;

		std::cout << "size: " << oneline.size() << std::endl;

		zmq::message_t message(oneline.size());
		memcpy(reinterpret_cast<void *>(message.data()),
			oneline.c_str(), oneline.size());
		try {
			socket.send(message);
		} catch (zmq::error_t &e) {
			std::cerr << "#E zmq send err. " << e.what() << std::endl;
			return -1;
		}
        }

	std::cout << "# break loop" << std::endl;
	socket.close();

        return 0;
}

int send_one_command(zmq::context_t &context, std::string &com_url, std::string &command)
{

	zmq::socket_t socket(context, ZMQ_PUSH);
	socket.connect(com_url);

	std::cout << "size: " << command.size() << std::endl;

	zmq::message_t message(command.size());
	memcpy(reinterpret_cast<void *>(message.data()),
		command.c_str(), command.size());
	try {
		socket.send(message);
	} catch (zmq::error_t &e) {
		std::cerr << "#E zmq send err. " << e.what() << std::endl;
		return -1;
	}

	socket.close();

        return 0;
}


int main(int argc, char* argv[])
{
	char *chost = const_cast<char *>(DEFAULT_HOST);
	char *cport = const_cast<char *>(DEFAULT_PORT);
	std::string command;
	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if (sargv[0] != '-') command = sargv;

		if ((sargv == "-h") && (argc > i + 1)) {
			chost = argv[++i];
		}
		if ((sargv == "-p") && (argc > i + 1)) {
			int port = strtol(argv[++i], NULL, 0);
			if (port > 0) {
				cport = argv[i];
			}
		}
	}
	std::string str_host(chost);
	std::string str_port(cport);
	std::string com_url = "tcp://" + str_host + ":" + str_port;
	std::cout << "#D " << com_url << std::endl;

	zmq::context_t context(1);

	if (command.size() == 0) {
		command_loop(context, com_url);
	} else {
		send_one_command(context, com_url, command);
	}
	
	return 0;
}
