/*
 *
 */

#include <iostream>
#include <cstring>

//const char *g_snd_endpoint = "tcp://localhost:5558";
//const char *g_snd_endpoint = "ipc://./hello";
const char *g_snd_endpoint = "inproc://hello";
//const char *g_rec_endpoint = "tcp://*:5558";
//const char *g_rec_endpoint = "ipc://./hello";
const char *g_rec_endpoint = "inproc://hello";

const char *ebsrv_endpoint_default = "tcp://*:5559";
const char *com_endpoint_default = "tcp://*:5560";
char *g_ebsrv_endpoint;
char *g_com_endpoint;

#include "nodelist.cxx"
#include "daqtask.cxx"
#include "dtarecbe.cxx"
#include "dteb.cxx"



void print_state(int state)
{
	if (state == SM_INIT) std::cout << "INIT" << std::flush;
	if (state == SM_IDLE) std::cout << "IDLE" << std::flush;
	if (state == SM_RUNNING) std::cout << "RUNNING" << std::flush;
	if (state == SM_END) std::cout << "END" << std::flush;

	return;
}

bool init_sequence(DTeb *eb, std::vector<DAQTask *> &tasks)
{
	for (auto &i : tasks) i->clear_is_done();
	for (auto &i : tasks) i->clear_is_good();
	eb->set_state(SM_INIT);
	bool is_good_init = true;
	for (auto &i : tasks) {
		for (int j = 0 ; j < 10 ; j++) {
			if (i->is_done() == true) {
				is_good_init = false;
				std::cout << "#E " << i->get_id()
					<< "initialize fail !" << std::endl;
				break;
			} else {
				usleep(100);
			}
		}
		if (is_good_init != true) break;
	}
	for (auto &i : tasks) if (i->is_good() != true) is_good_init = false;

	return is_good_init;
}

int command_loop(DTeb *eb, std::vector<DAQTask *> &tasks)
{
	while (true) {
		std::string oneline;
		std::cout << "> ";
		std::cin >> oneline;
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


int remote_command_loop(DTeb *eb, std::vector<DAQTask *> &tasks, zmq::context_t &context)
{
	zmq::socket_t comport(context, ZMQ_PULL);
	comport.bind(g_com_endpoint);

	while (true) {
		zmq::message_t message;
		bool rc;
		try {
			rc = comport.recv(&message, ZMQ_NOBLOCK);
		} catch (zmq::error_t &e) {
			std::cerr << "#E leb command loop recv err." << e.what() << std::endl;
			continue;
		}
		if (! rc) {
			usleep(100);
			continue;
		}

		//std::string oneline(reinterpret_cast<char *>(message.data()));
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

	std::cout << "#D out of the command loop" << std::endl;


	return 0;
}


int main(int argc, char* argv[])
{

	//int port = 24;
	//static char host[128];
	//strcpy(host, "192,168,10.56");
	//bool is_dummy = false;
	int buf_size = 0;
	int nspill = 0;
	int quelen = 0;

	char default_file[] = "nodes.txt";
	char *nodefile = default_file;

	std::string ebsrv_endpoint(ebsrv_endpoint_default);
	std::string com_endpoint(com_endpoint_default);
	g_ebsrv_endpoint = const_cast<char *>(ebsrv_endpoint.c_str());
	g_com_endpoint = const_cast<char *>(com_endpoint.c_str());

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		//if ((sargv == "-h") && (argc > i)) {
		//	strncpy(host, argv[i++], 128);
		//}
	
		if ((sargv == "-p")  && (argc > i + 1)) {
			int port = strtol(argv[++i], NULL, 0);
			if (port > 0) {
				std::string strport(argv[i]);
				ebsrv_endpoint = "tcp://*:" + strport;
				g_ebsrv_endpoint = const_cast<char *>(ebsrv_endpoint.c_str());
			}
		}
		if ((sargv == "-c")  && (argc > i + 1)) {
			int port = strtol(argv[++i], NULL, 0);
			if (port > 0) {
				std::string strport(argv[i]);
				com_endpoint = "tcp://*:" + strport;
				g_com_endpoint = const_cast<char *>(com_endpoint.c_str());
			}
		}
		
		if ((sargv == "-b")  && (argc > i + 1)) {
			buf_size = strtol(argv[i++], NULL, 0);
		}
		if ((sargv == "-q")  && (argc > i + 1)) {
			quelen = strtol(argv[i++], NULL, 0);
		}
		if ((sargv == "-n")  && (argc > i + 1)) {
			nspill = strtol(argv[i++], NULL, 0);
		}
		//if (sargv == "--dummy") {
		//	is_dummy = true;
		//}

		#if 0
		if (sargv == "--help") {
			dt_printhelp(argv);
			return 0;
		}
		#endif

		if (sargv.front() != '-') {
			nodefile = argv[i];
		}

	}

	std::vector<struct nodeprop> nodes;
	std::vector<struct rbcp_com> coms;
	if (nodelist(nodefile, nodes, coms)) {
		std::cerr << "Node file error : " << nodefile << std::endl;
		return 1;
	}

	for (auto &i : nodes) {
		std::cout << "id: " << i.id
			<< "  host: "  <<i.host.c_str()
			<< "  port: "  << i.port
			<< "  is_dummy: "  << i.is_dummy <<std::endl;
	}

	for (auto &i : coms) {
		std::cout << "host: " << i.host
			<< "  port: "  << i.port
			<< "  addr: "  << i.address
			<< "  data:" << std::hex;
		for (auto &j : i.data) {
			std::cout << " " << std::setw(2) << std::setfill('0')
				<< (static_cast<int>(j) & 0xff);
		}
		std::cout << std::dec << std::endl;
	}


	zmq::context_t context(120);

	std::vector<DAQTask*> tasks;
	for (auto &i : nodes) {
		DTArecbe *avant = new DTArecbe(
			i.id, const_cast<char *>(i.host.c_str()),
			i.port, i.is_dummy);
		tasks.push_back(avant);
	}
	DTeb  *eb  = new DTeb(999);
	tasks.push_back(eb);

	eb->init(nodes);

	if (buf_size > 0) for (auto &i : tasks) {
		reinterpret_cast<DTArecbe *>(i)->set_bufsize(buf_size);
	}
	if (quelen > 0)   for (auto &i : tasks) {
		reinterpret_cast<DTArecbe *>(i)->set_quelen(quelen);
	}
	if (nspill > 0) eb->set_nspill(nspill);


	for (auto &i : tasks) i->run(&context);

	for (auto &i : tasks) i->clear_is_done();
	for (auto &i : tasks) i->clear_is_good();
	eb->set_state(SM_INIT);
	for (auto &i : tasks) while (i->is_done() != true) usleep(10);
	bool is_good_init = true;
	for (auto &i : tasks) if (i->is_good() != true) is_good_init = false;

	if (is_good_init) {

		eb->set_state(SM_IDLE);
		usleep(100*1000);
		eb->set_state(SM_RUNNING);

		{
		usleep(500*1000);
		std::cout << "#D data : " << g_ebsrv_endpoint  << std::endl;
		std::cout << "#D com  : " << g_com_endpoint  << std::endl;
		}
		//command_loop(eb, tasks);
		remote_command_loop(eb, tasks, context);

		eb->set_state(SM_EXIT);
	} else {
		std::cout << "Initialization fail!" << std::endl;
		eb->set_state(SM_EXIT);
	}

	for (auto &i : tasks) i->get_thread()->join();
	for (auto &i : tasks) delete i;

	std::cout << "main end" << std::endl;

	return 0;
}
