/*
 *
 */

#include <iostream>
#include <string>

#include <cstring>

#include "zmq.hpp"


#define COMMAND_PORT "tcp://localhost:5560"

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
		#endif

                if (oneline == "exit") break;
                if (oneline == "quit") break;
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


int main(int argc, char* argv[])
{
	std::string com_url(COMMAND_PORT);

	zmq::context_t context(1);

	command_loop(context, com_url);
	
	return 0;
}
