/*
 *
 */

#include <iostream>
#include <fstream>
#include <thread>

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>

#include "zmq.hpp"
#include "koltcp.h"

#include "packed_event.h"
#include "daqtask.cxx"
#include "mstopwatch.cxx"

#if 0
#include "dtfilename.cxx"
#else
const char *dtfilename(const char *name)
{
	static const char fname[] = "/dev/null";
	return fname;
}
#endif

//const char* g_rec_endpoint = "tcp://*:5558";
//const char* g_rec_endpoint = "ipc://./hello";
//const char* g_rec_endpoint = "inproc://hello";

const char* fnhead = "leb";


unsigned long int get_time_ms()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	unsigned long int val =
		(now.tv_sec  & 0x000fffffffffffff) * 1000 + (now.tv_usec / 1000);

	return val;
}


std::atomic<int> g_ebs_depth(0);
void ebs_buf_free(void *buf, void *hint)
{
	unsigned char *bufbuf = reinterpret_cast<unsigned char *>(buf);
	delete bufbuf;
	g_ebs_depth--;
}



class DTeb : public DAQTask
{
public:
	DTeb(int i) : DAQTask(i), m_nspill(1) {};
	int get_nspill() {return m_nspill;};
	void set_nspill(int i) {m_nspill = i;};
	virtual int init(std::vector<struct nodeprop> &);
	void monitor();
protected:
	//virtual void state_machine(void *) override;
	virtual int st_init(void *) override;
	virtual int st_idle(void *) override;
	virtual int st_running(void *) override;
private:
	//int scan_past(std::vector<struct ebevent> &, int);
	int send_data(struct ebevent &, zmq::socket_t &);
	int write_data(struct ebevent &);
	int write_data(unsigned char *, int);
	int pack_data(struct ebevent &, unsigned char * &);
	int m_nspill;
	std::vector<unsigned int> m_nodes;
	std::vector<struct ebevent> m_events;

	std::ofstream m_ofs;
};

#if 0
void DTeb::state_machine(void *context)
{
	c_dtmtx->lock();
	std::cout << "#eb sm start# " << m_id << " : " << c_state << std::endl;
	c_dtmtx->unlock();

	while(true) {
		switch (c_state) {
			case SM_INIT :
				if (!m_is_done) {
					st_init(context);
				} else {
					usleep(1000);
				}
				m_is_done = true;
				break;

			case SM_IDLE :
				usleep(1);
				st_idle(context);
				break;

			case SM_RUNNING :
				st_running(context);
				break;
		}
		if (c_state == SM_END) break;
	}

	std::cout << "Task:" << m_id << " end." << std::endl;

	return;
}
#endif

int DTeb::init(std::vector<struct nodeprop> &nodes)
{
	for (auto &i : nodes) m_nodes.push_back(i.id);
	return m_nodes.size();
}

int DTeb::st_init(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "eb(" << m_id << ") init" << std::endl;
	}

	return 0;
}

int DTeb::st_idle(void *context)
{
	#if 0
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "eb(" << m_id << ") idle" << std::endl;
	}
	#endif
	usleep(100000);

	return 0;
}


struct ebbuf {
	unsigned int id;
	std::deque<int> event_number;
	int discard;
	int prev_en;
};

struct ebevent {
	unsigned int event_number;
	std::vector<unsigned int> node;
	std::vector<std::vector<unsigned char>> data; 
	unsigned long int time_ms;
	bool is_fullev;
};

//static int nspill = 1;

int DTeb::st_running(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "eb(" << m_id << ") running" << std::endl;
	}

	zmq::socket_t receiver(
		*(reinterpret_cast<zmq::context_t *>(context)),
		ZMQ_PULL);
	#if 0
	receiver.setsockopt(ZMQ_RCVBUF, 16 * 1024);
	receiver.setsockopt(ZMQ_RCVHWM, 1000);
	std::cout << "eb: ZMQ_RCVBUF : " << receiver.getsockopt<int>(ZMQ_RCVBUF) << std::endl;
	std::cout << "eb: ZMQ_RCVHWM : " << receiver.getsockopt<int>(ZMQ_RCVHWM) << std::endl;
	#endif
	receiver.bind(g_rec_endpoint);


	zmq::socket_t ebserver(
		*(reinterpret_cast<zmq::context_t *>(context)),
		ZMQ_PUSH);
	#if 0
	ebserver.setsockopt(ZMQ_SNDHWM, 1000);
	std::cout << "ebsrv: ZMQ_SNDBUF : " << ebserver.getsockopt<int>(ZMQ_SNDBUF) << std::endl;
	std::cout << "ebsrv: ZMQ_SNDHWM : " << ebserver.getsockopt<int>(ZMQ_SNDHWM) << std::endl;
	#endif
	ebserver.bind(g_ebsrv_endpoint);

	std::cout << "#D dteb inited." << std::endl;

	//char wfname[128];
	//strncpy(wfname, dtfilename(fnhead), 128);
	//std::ofstream ofs;


	std::vector<struct ebbuf> ebuf;
	int nread_flagment = 0;

	//std::vector<struct ebevent> ebevents;
	std::vector<unsigned int> trigv(m_nodes.size());

	while (true) {
		zmq::message_t message;

		//if (c_state != SM_RUNNING) break;
		if ((c_state != SM_RUNNING) && (g_avant_depth <= 0)) break;

		bool rc;
		try {
			rc = receiver.recv(&message, ZMQ_NOBLOCK);
		} catch (zmq::error_t &e) {
			std::cerr << "#E eb zmq recv err. " << e.what() << std::endl;
			//break;
			continue;
		}
		if (! rc) {
			usleep(100);
			continue;
		}

		unsigned int *head = reinterpret_cast<unsigned int *>(message.data());
		unsigned char *cbody =  reinterpret_cast<unsigned char *>(head + 2);
		unsigned int id = head[0] & 0x000000ff;
		unsigned int trig = head[1] & 0x0fffffff;
		unsigned int data_size = message.size() -  (2 *sizeof(unsigned int));

		#if 0
		{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << std::endl << "### size: " << std::dec << data_size
			<< " header: " << std::hex<< head[0];
		for (int i = 0 ; i < 32 ; i++) {
			if ((i % 8) == 0) std::cout << std:: endl;
			std::cout << " " << std::hex << std::setw(8) << data[i] ;
		}
		std::cout << std::endl << "-";
		unsigned int isize  = data_size / sizeof(unsigned int);
		for (unsigned int i = isize - 16 ; i < isize ; i++) {
			if ((i % 8) == 0) std::cout << std:: endl;
			std::cout << " " << std::hex << std::setw(8) << data[i] ;
		}
		}
		#endif

		#if 0
		{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "### id: " << std::dec << id
			<< " trig: " << trig
			<< " size: " << data_size << std::endl;
		}
		#endif


		bool is_new = true;

		#if 0
		for (unsigned int i = 0 ; i < ebuf.size() ; i++) {
			if (id == ebuf[i].id) {
				is_new = false;
				ebuf[i].event_number.push_back(trig);
			}
		}
		if (is_new) {
			struct ebbuf node;
			node.id = id;
			node.event_number.push_back(trig);
			node.discard = 0;
			ebuf.push_back(node);
		}
		#endif


		is_new = true;
		for (unsigned int i = 0 ; i < m_events.size() ; i++) {
			if (trig == m_events[i].event_number) {
				is_new = false;
				m_events[i].node.push_back(id);
				std::vector<unsigned char> cdata(cbody, cbody + data_size); 
				m_events[i].data.push_back(cdata);

				if (m_events[i].node.size() >= m_nodes.size()) {
					m_events[i].is_fullev = true;

					//send_data(m_events[i]);
					//m_events.erase(m_events.begin() + i);

					//if (i > 0) std::cout << "X" << std::flush;

					//先にきた event_number の若い event は不完全でも送る。
					for (unsigned int j = 0 ; j <= i ; j++) {
						if (m_events[j].event_number
							<= m_events[i].event_number) {
							send_data(m_events[j], ebserver);
						}
					}
					//送ったもののバッファの削除、逆順で消していく。
					for (int j = i ; j >= 0 ; j--) {
						if (m_events[j].event_number
							<= m_events[i].event_number) {
							m_events.erase(m_events.begin() + j);
						}
					}

					break;
				}
			}
		}
		if (is_new) {
			struct ebevent ev;
			ev.event_number = trig;
			ev.node.push_back(id);
			std::vector<unsigned char> cdata(cbody, cbody + data_size); 
			ev.data.push_back(cdata);
			ev.time_ms = get_time_ms();
			ev.is_fullev = false;

			if (m_nodes.size() == 1) {
				send_data(ev, ebserver);
			} else {
				m_events.push_back(ev);
			}
		}


		auto it = std::find(m_nodes.begin(), m_nodes.end(), id);
		int index = std::distance(m_nodes.begin(), it);
		trigv[index] = trig;


		#if 0
		if ((nread_flagment % 100) == 0) {
			std::cout << "\r\e[16C Evb: " << m_events.size();
			for (unsigned int i = 0 ; i < m_nodes.size() ; i++) {
				std::cout << " id:" << m_nodes[i]
				<< " T:" << trigv[i] << "    ";
			}
			std::cout << std::flush;
		}
		#endif

		#if 0
		if ((nread_flagment % 1000) == 0) {
			std::cout << "\r Ev: " << m_events.size() << " : ";
			for (auto &i : m_events) {
				std::cout << ", " << i.event_number
				<< " " << i.node.size();
			}
			std::cout << "    " << std::endl << std::flush;
		}
		#endif

		#if 0
		if ((nread_flagment % 1000) == 0) {
			std::cout << "\rID : ";
			for (unsigned int i = 0 ; i < buf.size() ;i++) {
				std::cout << "  " << i
					<< ": " << buf[i].id;
			}
			std::cout << "      " << std::flush;

		} else {
			//std::cout << "." << std::flush;
		}
		#endif

		nread_flagment++;
	}

	if (m_ofs.is_open())  m_ofs.close();

	return 0;

}

int DTeb::write_data(struct ebevent &event)
{
	static int wcount = 0;
	static mStopWatch sw;

	//static std::ofstream ofs;

	if (! m_ofs.is_open()) {
		char wfname[128];
		strncpy(wfname, dtfilename(fnhead), 128);
		m_ofs.open(wfname, std::ios::out);
		std::cout << wfname << std::endl;
		sw.start();
	}


	for (unsigned int i = 0 ; i < m_nodes.size() ; i++) {
		bool is_match = false;
		for (unsigned int j = 0 ; j < event.data.size() ; j++) {
			if (event.node[j] == m_nodes[i]) {
				m_ofs.write(
					reinterpret_cast<char *>(event.data[j].data()),
					event.data[j].size());
				wcount += event.data[j].size();
				is_match = true;
			}
		}
		if (! is_match) {
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cout << std::endl << "#W No event flagments : "
				<< m_nodes[i] << " En : " << event.event_number
				<< "  " << std::endl;
		}
	}

	static unsigned int ebcount = 0;
	if ((ebcount % 100) == 0) {
		int elapse = sw.elapse();
		if (elapse >= 10000) {
			sw.start();
			double wspeed = static_cast<double>(wcount)
				/ 1024 / 1024
				* 1000 / static_cast<double>(elapse);
			double freq = ebcount * 1000 / static_cast<double>(elapse);

			std::cout << "# Freq: " << freq << " Throughput: "
			<< wcount << " B " << elapse << " ms "
			<< wspeed << " MiB/s"  << std::endl;

			wcount = 0;
			ebcount = 0;
		}
	}
	
	ebcount++;

	return 0;
}

int DTeb::write_data(unsigned char *cdata, int data_size)
{
	static int wcount = 0;
	static mStopWatch sw;

	if (! m_ofs.is_open()) {
		char wfname[128];
		strncpy(wfname, dtfilename(fnhead), 128);
		m_ofs.open(wfname, std::ios::out);
		std::cout << wfname << std::endl;
		sw.start();
	}

	m_ofs.write(
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

/*
struct packed_event {
	uint32_t magic;
	uint16_t length;
	uint16_t id;
	uint16_t nodes;
	uint16_t flag;
};
*/

#define LEB_MAGIC 0xffffffff
#define LEB_FLAG_COMPLETE  0x0001
#define LEB_FLAG_INCOMPLETE  0x0000

#if 0
int DTeb::pack_data(struct ebevent &event, char *cevent)
{
	struct packed_event *pevent = reinterpret_cast<struct packed_event *>(cevent);
	char *data_body = cevent + sizeof(struct packed_event);
	int wcount = 0;
	
	for (unsigned int i = 0 ; i < m_nodes.size() ; i++) {
		bool is_match = false;
		for (unsigned int j = 0 ; j < event.data.size() ; j++) {
			if (event.node[j] == m_nodes[i]) {
				//m_ofs.write(event.data[j].data(), event.data[j].size());
				memcpy(event.data[j].data(), data_body + wcount, event.data[j].size());
				data_body += event.data[j].size();
				wcount += event.data[j].size();
				is_match = true;
			}
		}
		if (is_match) {
			event.is_fullev = true;
		} else {
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cout << std::endl << "#W No event flagments : "
				<< m_nodes[i] << " En : " << event.event_number
				<< "  " << std::endl;
			event.is_fullev = false;
		}
	}

	pevent->magic = htonl(LEB_MAGIC);
	pevent->length = htons(wcount);
	pevent->id = static_cast<uint16_t>(htons(m_id & 0xffff));
	pevent->nodes = htons(event.data.size());
	uint16_t fflag = event.is_fullev;
	fflag = event.is_fullev
		? fflag | LEB_FLAG_COMPLETE : fflag | LEB_FLAG_INCOMPLETE;
	pevent->flag = htonl(fflag);

	return static_cast<int>(event.is_fullev);
}
#endif


int DTeb::pack_data(struct ebevent &event, unsigned char * &pbuf)
{
	struct packed_event header;
	unsigned char *cheader = reinterpret_cast<unsigned char *>(&header);
	int wcount = 0;

	static std::vector<unsigned char> buf;
	buf.clear();
	buf.resize(0);

	header.magic = htonl(LEB_MAGIC);
	header.length = 0;
	header.id = static_cast<uint16_t>(htons(m_id & 0xffff));
	header.nodes = htons(event.data.size());
	header.flag = 0;

	for (unsigned int i = 0 ; i < sizeof(struct packed_event) ; i++) {
		buf.push_back(cheader[i]);
	}


	for (unsigned int i = 0 ; i < m_nodes.size() ; i++) {
		bool is_match = false;
		for (unsigned int j = 0 ; j < event.data.size() ; j++) {
			if (event.node[j] == m_nodes[i]) {
				std::copy(
					event.data[j].begin(), event.data[j].end(),
					std::back_inserter(buf));

				wcount += event.data[j].size();
				is_match = true;
			}
		}
		if (is_match) {
			event.is_fullev = true;
		} else {
			#if 0
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cout << std::endl << "#W No event flagments : "
				<< m_nodes[i] << " En : " << event.event_number
				<< "  " << std::endl;
			#else
			//std::lock_guard<std::mutex> lock(*c_dtmtx);
			//std::cout << "X" << std::flush;
			#endif
			event.is_fullev = false;
		}
	}

	//std::cout << "#D " << buf.size() - sizeof(struct packed_event)
	//	<< ", " << wcount << std::endl;;

	pbuf = buf.data();
	struct packed_event *pevent = reinterpret_cast<struct packed_event *>(pbuf);
	pevent->length = htons(wcount);
	//pevent->nodes = htons(event.data.size());
	uint16_t fflag = event.is_fullev;
	fflag = event.is_fullev
		? fflag | LEB_FLAG_COMPLETE : fflag | LEB_FLAG_INCOMPLETE;
	pevent->flag = htonl(fflag);

	return buf.size();
}


int DTeb::send_data(struct ebevent &event, zmq::socket_t &ebserver)
{
	unsigned char *ebdata;
	int nsize = pack_data(event, ebdata);

	#if 0
	{
	std::lock_guard<std::mutex> lock(*c_dtmtx);

	std::cout << "#D node ids:";
	for (auto i : event.node) std::cout << " " << i;
	std::cout << std::endl;

	std::cout << "#D " << " datasize: " << nsize << std::endl;
	struct packed_event *pevent = reinterpret_cast<struct packed_event *>(ebdata);
	std::cout << " magic: " << std::hex << pevent->magic
		<< " len: " << std::dec << ntohs(pevent->length)
		<< " id: " << ntohs(pevent->id)
		<< " nodes: " << ntohs(pevent->nodes)
		<< " flag: " << ntohl(pevent->flag)
		<< std::endl;

	#if 0
	std::cout << std::hex;
	for (int i = 0 ; i < 128 ; i++) {
		if ((i % 16) == 0) std::cout << std::endl;
		std::cout << " "
			<< std::setw(2) << std::setfill('0')
			<< static_cast<int>(ebdata[i]);
	}
	std::cout << std::dec << std::endl;
	#endif
	}
	#endif

	#if 1
	if (g_ebs_depth < 1000) {
	
	unsigned char *sendbuf = new unsigned char[nsize];
	memcpy(sendbuf, ebdata, nsize);
	zmq::message_t message(
		reinterpret_cast<void *>(sendbuf),
		nsize,
		ebs_buf_free,
		NULL);
	try {
		ebserver.send(message);
		g_ebs_depth++;
	} catch (zmq::error_t &e) {
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cerr << "#E DTeb send_data zmq err. " << e.what() << std::endl;
		return -1;
	}

	}
	#endif


	static int lcount = 0;
	if ((lcount % 10) == 0) {
		std::cout << "\r Sent : " << lcount << "  " << std::flush;
	}
	lcount++;

	//write_data(event);
	write_data(ebdata, nsize);

	return 0;
}


#if 0
int DTeb::scan_past(std::vector<struct ebevent> &events, int evn)
{
	unsigned int en_max = events[evn].event_number;
	for(int i = evn ; i >= 0 ; i--) {
		if (events[i].event_number < en_max) {
			send_data(events[i], ebserver);
			events.erase(events.begin() + i);
		}
	
	}
	
	return 0;
}
#endif


void DTeb::monitor()
{

	std::cout << "\r Ev Buf: " << m_events.size() << " : " << std::endl;
	for (auto &i : m_events) {
		std::cout << ", " << i.event_number
		<< " " << i.node.size();
	}
	std::cout << std::endl;

	return;
}
