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
	int scan_past(std::vector<struct ebevent> &, int);
	int send_data(struct ebevent &);
	int write_data(char*, int);
	int m_nspill;
	std::vector<int> m_ids;
	std::vector<struct ebevent> m_events;
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
	for (auto &i : nodes) m_ids.push_back(i.id);
	return m_ids.size();
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
	std::vector<std::vector<char>> data; 
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

	//receiver.setsockopt(ZMQ_RCVBUF, 16 * 1024);
	//receiver.setsockopt(ZMQ_RCVHWM, 1000);
	#if 0
	std::cout << "eb: ZMQ_RCVBUF : " << receiver.getsockopt<int>(ZMQ_RCVBUF) << std::endl;
	std::cout << "eb: ZMQ_RCVHWM : " << receiver.getsockopt<int>(ZMQ_RCVHWM) << std::endl;
	#endif

	receiver.bind(g_rec_endpoint);

	zmq::message_t message;

	char wfname[128];
	strncpy(wfname, dtfilename(fnhead), 128);
	std::ofstream ofs;


	std::vector<struct ebbuf> ebuf;
	int nread_flagment = 0;

	//std::vector<struct ebevent> ebevents;

	while (true) {

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
		char *cbody =  reinterpret_cast<char *>(head + 2);
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
				std::vector<char> cdata(cbody, cbody + data_size); 
				m_events[i].data.push_back(cdata);

				if (m_events[i].node.size() >= m_ids.size()) {
					//std::cout << "x" << std::flush;
					m_events[i].is_fullev = true;

					//send_data(m_events[i]);
					//m_events.erase(m_events.begin() + i);

					if (i > 0) std::cout << "X" << std::flush;
					//先にきた event_number の若い event は不完全でも送る。
					for (unsigned int j = 0 ; j <= i ; j++) {
						if (m_events[j].event_number
							<= m_events[i].event_number) {
							send_data(m_events[j]);
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
			std::vector<char> cdata(cbody, cbody + data_size); 
			ev.data.push_back(cdata);
			ev.time_ms = get_time_ms();
			ev.is_fullev = false;

			if (m_ids.size() == 1) {
				send_data(ev);
			} else {
				m_events.push_back(ev);
			}
		}


		write_data(cbody, data_size);


		if ((nread_flagment % 1000) == 0) {
			std::cout << "\r Ev size: " << m_events.size() << "    " << std::flush;
		}

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

	ofs.close();

	return 0;

}

int DTeb::write_data(char *cdata, int data_size)
{
	unsigned int *data = reinterpret_cast<unsigned int *>(cdata);

	static int spillcount = 0;
	static int wcount = 0;
	static mStopWatch sw;


	static std::ofstream ofs;

	if (! ofs.is_open()) {
		char wfname[128];
		strncpy(wfname, dtfilename(fnhead), 128);
		ofs.open(wfname, std::ios::out);
		std::cout << wfname << std::endl;
		sw.start();
	}

	while (data_size > 0) {
		bool is_spill_end = false;
		for (unsigned int i = 0 ; i < (data_size / sizeof(unsigned int)) ; i++) {
			if (data[i] == 0xffff5555) {
				time_t now = time(NULL);
				ofs.write(reinterpret_cast<char *>(data),
					sizeof(unsigned int) * (i + 1));
				ofs.write(reinterpret_cast<char *>(&now), sizeof(time_t));
				data = &(data[i + 1]);
				data_size = data_size - ((i + 1) * sizeof(unsigned int));
				spillcount++;
				wcount += sizeof(unsigned int) * (i + 1) + sizeof(time_t);

				if ((spillcount % m_nspill) == 0) {
					char wfname[128];
					ofs.close();

					int elapse = sw.elapse();
					sw.start();
					double wspeed = static_cast<double>(wcount)
						/ 1024 / 1024
						* 1000 / static_cast<double>(elapse);
					std::cout << wfname << " "
						<< wcount << " B " << elapse << " ms "
						<< wspeed << " MiB/s"  << std::endl;

					wcount = 0;
					strncpy(wfname, dtfilename(fnhead), 128);
					ofs.open(wfname, std::ios::out);
				}
				is_spill_end = true;
				#if 0
				{
				std::lock_guard<std::mutex> lock(*c_dtmtx);
				std::cout << "# left data : " << data_size << std::endl;
				}
				#endif
				if (data_size > 0) {
					ofs.write(cdata, data_size);
					wcount += data_size;
				}
			}
		}
		if (! is_spill_end) {
			ofs.write(reinterpret_cast<char *>(data), data_size);
			wcount += data_size;
			break;
		}
	}

	return data_size;

}


int DTeb::send_data(struct ebevent &event)
{

	static int lcount = 0;
	if ((lcount % 100) == 0) std::cout << "." << std::flush;

	return 0;
}


int DTeb::scan_past(std::vector<struct ebevent> &events, int evn)
{
	unsigned int en_max = events[evn].event_number;
	for(int i = evn ; i >= 0 ; i--) {
		if (events[i].event_number < en_max) {
			send_data(events[i]);
			events.erase(events.begin() + i);
		}
	
	}
	
	return 0;
}


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
