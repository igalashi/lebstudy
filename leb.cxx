/*
 *
 */

#include <iostream>
#include <cstring>

//const char* g_snd_endpoint = "tcp://localhost:5558";
//const char* g_snd_endpoint = "ipc://./hello";
const char* g_snd_endpoint = "inproc://hello";
//const char* g_rec_endpoint = "tcp://*:5558";
//const char* g_rec_endpoint = "ipc://./hello";
const char* g_rec_endpoint = "inproc://hello";

#include "nodelist.cxx"
#include "daqtask.cxx"
#include "dtarecbe.cxx"
#include "dteb.cxx"


int main(int argc, char* argv[])
{

	int port = 24;
	static char host[128];
	strcpy(host, "192,168,10.56");
	bool is_dummy = false;
	int buf_size = 0;
	int nspill = 0;
	int quelen = 0;

	char default_file[] = "nodes.txt";
	char *nodefile = default_file;

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-h") && (argc > i)) {
			strncpy(host, argv[i++], 128);
		}
		if ((sargv == "-p")  && (argc > i)) {
			port = strtol(argv[i++], NULL, 0);
		}
		if ((sargv == "-b")  && (argc > i)) {
			buf_size = strtol(argv[i++], NULL, 0);
		}
		if ((sargv == "-q")  && (argc > i)) {
			quelen = strtol(argv[i++], NULL, 0);
		}
		if ((sargv == "-n")  && (argc > i)) {
			nspill = strtol(argv[i++], NULL, 0);
		}
		if (sargv == "--dummy") {
			is_dummy = true;
		}

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
	if (nodelist(nodefile, nodes)) {
		std::cerr << "Node file error : " << nodefile << std::endl;
		return 1;
	}

	for (auto &i : nodes) {
		std::cout << "id: " << i.id
			<< "  host: "  <<i.host.c_str()
			<< "  port: "  << i.port
			<< "  is_dummy: "  << i.is_dummy <<std::endl;
	}


	zmq::context_t context(1);

	DTarecbe *avant = new DTarecbe(1, host, port, is_dummy);
	DTeb  *eb  = new DTeb(2);

	if (buf_size > 0) avant->set_bufsize(buf_size);
	if (quelen > 0) avant->set_quelen(quelen);
	if (nspill > 0) eb->set_nspill(nspill);

	std::vector<DAQTask*> tasks;
	tasks.push_back(avant);
	tasks.push_back(eb);

	for (auto &i : tasks) i->run(&context);

	for (auto &i : tasks) i->clear_is_done();
	for (auto &i : tasks) i->clear_is_good();
	avant->set_state(SM_INIT);
	for (auto &i : tasks) while (i->is_done() != true) usleep(10);
	bool is_good_init = true;
	for (auto &i : tasks) if (i->is_good() != true) is_good_init = false;


	if (is_good_init) {

		avant->set_state(SM_IDLE);
		usleep(100*1000);
		avant->set_state(SM_RUNNING);
		//sleep(1);
		while (true) {
			std::string oneline;
			std::cin >> oneline;
			if (oneline == "run") avant->set_state(SM_RUNNING);
			if (oneline == "idle") avant->set_state(SM_IDLE);
			if (oneline == "end") break;;
			if (oneline == "stop") break;;
			if (oneline == "exit") break;;
			if (oneline == "q") break;;
		}
		avant->set_state(SM_IDLE);
		usleep(100*1000);
		avant->set_state(SM_END);

	} else {
		std::cout << "Initialization fail!" << std::endl;
		avant->set_state(SM_END);
	}

	for (auto &i : tasks) i->get_thread()->join();
	for (auto &i : tasks) delete i;

	std::cout << "main end" << std::endl;

	return 0;
}
