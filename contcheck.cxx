/*
 *
 */

#include <iostream>
#include <string>

#include <cstring>
#include <unistd.h>

#include "zmq.hpp"


#define COMMAND_PORT "tcp://localhost:5560"
const char *g_com_endpoint = "tcp://*:5560";

enum RunState {
	SM_INIT,
	SM_IDLE,
	SM_RUNNING,
	SM_END,
	SM_EXIT,
};

class TestEb {
	private:
		int c_state = SM_INIT;
	public:
		void set_state(int state) {c_state = state ; return;};
		int get_state() {return c_state;};
		void monitor() {std::cout << "mon" << std::endl; return;};
	protected:
};

int tasks = 0;
void init_sequence(TestEb *eb, int tasks)
{
	std::cout << "init_sequence" << std::endl;
	return;
}

void print_state(int state)
{
	std::cout << "State: " << state << std::endl;
	return;
}

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


int remote_command_loop(TestEb *eb, zmq::context_t &context)
{
	zmq::socket_t comport(context, ZMQ_PULL);
	comport.bind(g_com_endpoint);

	while (true) {
		zmq::message_t message;
		bool rc;
		try {
			//rc = comport.recv(&message, ZMQ_NOBLOCK);
			rc = comport.recv(&message);
		} catch (zmq::error_t &e) {
			std::cerr << "#E leb command loop recv err." << e.what() << std::endl;
			continue;
		}
		if (! rc) {
			std::cout << "." << std::flush;
			usleep(100);
			continue;
		}

		char word[message.size() + 1];
		memcpy(word, reinterpret_cast<char *>(message.data()), message.size());
		word[message.size()] = '\0';
		std::string oneline(word);
		std::cout << "#D " << message.size() << " " << oneline << std::endl;

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
		if (oneline == "q") break;
		
	}


	return 0;
}



int main(int argc, char* argv[])
{
	std::string com_url(COMMAND_PORT);

	TestEb *eb = new TestEb();

	zmq::context_t context(1);
	remote_command_loop(eb, context);
	
	return 0;
}
