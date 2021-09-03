/*
 *
 */

#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "zmq.hpp"
#include "koltcp.h"


#include "daqtask.cxx"
#include "recbe.h"



const int default_bufsize = 1024 * sizeof(unsigned int);
const int default_quelen = 10000;

std::atomic<int> g_avant_depth(0);


//const char* g_snd_endpoint = "tcp://localhost:5558";
//const char* g_snd_endpoint = "ipc://./hello";
//const char* g_snd_endpoint = "inproc://hello";

void buf_free(void *buf, void *hint)
{
	char *bufbuf = reinterpret_cast<char *>(buf);
	delete bufbuf;
	g_avant_depth--;
}



class DTArecbe : public DAQTask
{
public:
	DTArecbe(int i) : DAQTask(i) {};
	DTArecbe(int, char *, int, bool);
	DTArecbe(struct nodeprop &);
	int get_bufsize() {return m_bufsize;};
	void set_bufsize(int size) {m_bufsize = size;};
	int get_quelen() {return m_quelen;};
	void set_quelen(int len) {m_quelen = len;};
protected:
	virtual int st_init(void *) override;
	virtual int st_idle(void *) override;
	virtual int st_running(void *) override;
private:
	char *m_host;
	int m_port;
	kol::TcpClient *tcp;
	bool m_is_dummy;
	int m_bufsize;
	int m_quelen;
	int m_trig_num;
};

DTArecbe::DTArecbe(int i, char *host, int port, bool is_dummy)
	: DAQTask(i), m_host(host), m_port(port),
	m_is_dummy(is_dummy), m_bufsize(default_bufsize), m_quelen(default_quelen),
	m_trig_num(0)
{
}

DTArecbe::DTArecbe(struct nodeprop &node)
	: DAQTask(node.id),
	m_host(const_cast<char *>(node.host.c_str())), m_port(node.port),
	m_is_dummy(node.is_dummy), m_bufsize(default_bufsize), m_quelen(default_quelen),
	m_trig_num(0)
{
}


int DTArecbe::st_init(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "avant(" << m_id << ") init" << std::endl;
	}

	if (! m_is_dummy) {
		try {
			tcp = new kol::TcpClient(m_host, m_port);
		} catch(kol::SocketException &e) {
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cout << "Socket open error. (" << m_host << ", " << m_port
				<< ") "  << e.what() << std::endl;
			m_is_good = false;
			return 1;
		}
	} else {
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "Dummy mode" << std::endl;
	}

	return 0;
}

int DTArecbe::st_idle(void *context)
{
	#if 0
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "avant(" << m_id << ") idle" << std::endl;
	}
	#endif
	usleep(100000);

	return 0;
}

int DTArecbe::st_running(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "avant(" << m_id << ") running" << std::endl;
	}

	zmq::socket_t sender(
		*(reinterpret_cast<zmq::context_t *>(context)),
		ZMQ_PUSH);
	//sender.setsockopt(ZMQ_SNDBUF, m_bufsize + 8); //ireruto tcp no toki osokunaru ??
	//sender.setsockopt(ZMQ_SNDHWM, 512*1024);
	sender.setsockopt(ZMQ_SNDHWM, m_quelen);
	#if 1
	std::cout << "avant: ZMQ_SNDBUF : " << sender.getsockopt<int>(ZMQ_SNDBUF) << std::endl;
	std::cout << "avant: ZMQ_SNDHWM : " << sender.getsockopt<int>(ZMQ_SNDHWM) << std::endl;
	#endif

	sender.connect(g_snd_endpoint);

	int segnum = 0;
	while (1) {

		if (c_state != SM_RUNNING) break;

		char *buf;
		try {
			buf = new char[m_bufsize + 2 * sizeof(unsigned int)];
		} catch (std::exception &e){
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cerr << "avant; Memory allocation fail. " << e.what() << std:: endl;
			return -1;
		}
		g_avant_depth++;

		unsigned int *ebheader = reinterpret_cast<unsigned int*>(buf);
		char *cheader = buf + 2 * sizeof(unsigned int);
		struct recbe_header *header = reinterpret_cast<struct recbe_header *>(cheader);
		char *cbody = buf + (2 * sizeof(unsigned int)) + sizeof(struct recbe_header);
		unsigned short *body = reinterpret_cast<unsigned short *>(cbody);

		int nread = 0;
		if (! m_is_dummy) {
			try {
				tcp->read(cheader, sizeof(struct recbe_header));
			} catch (kol::SocketException &e) {
				std::lock_guard<std::mutex> lock(*c_dtmtx);
				std::cerr << "#E tcp read header err. " << e.what() << std::endl;
				break;
			}
			nread += tcp->gcount();
		} else {
			//static unsigned int m_trig_num = 0;
			header->type = 0xaa;
			header->id = m_id;
			header->sent_num = htons(m_trig_num & 0xffff);
			header->time = htons(time(NULL) & 0xffff);
			header->len = htons(128);
			header->trig_count = htonl(m_trig_num++);
			nread += sizeof(struct recbe_header);
		}

		ebheader[0] = 0xff000000 | header->id;
		ebheader[1] = 0xf0000000 | ntohl(header->trig_count);
		int body_length = static_cast<int>(ntohs(header->len));

		if (! m_is_dummy) {
			try {
				tcp->read(cbody, body_length);
			} catch (kol::SocketException &e) {
				std::lock_guard<std::mutex> lock(*c_dtmtx);
				std::cerr << "#E tcp read body err. " << e.what() << std::endl;
				break;
			}
			nread += tcp->gcount();
		} else {
			for (unsigned int i = 0 ;
				i < (body_length / sizeof(unsigned short)) ; i++) {
				 body[i] = ntohs(i & 0xffff);
			}
			nread += body_length;
			usleep(100);
		}

		zmq::message_t message(
			reinterpret_cast<void *>(buf),
			nread + (2 * sizeof(unsigned int)),
			buf_free,
			NULL);
		try {
			sender.send(message);
		} catch (zmq::error_t &e) {
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cerr << "#E send zmq err. " << e.what() << std::endl;
			return -1;
		}
	
		#if 0
		if ((segnum % 1000) == 0) {
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cout << "\rLSQue: "
				<< std::setw(4) << g_avant_depth << "   " << std::flush;
		}
		#endif

		segnum++;
	}


	return 0;
}
