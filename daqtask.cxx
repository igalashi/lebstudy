/*
 *
 */

#ifndef INC_DAQ_TASK
#define INC_DAQ_TASK

#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "zmq.hpp"
#include "koltcp.h"


enum RunState {
	SM_INIT,
	SM_IDLE,
	SM_RUNNING,
	SM_END,
};


class DAQTask
{
public:
	DAQTask(int);
	virtual ~DAQTask();
	virtual std::thread* run(void*);
	int get_state() {return c_state;};
	void set_state(int state) {c_state = state; return;};
	std::thread* get_thread() {return m_sm;};
	void set_id(int id) {m_id = id;};
	int get_id() {return m_id;};
	void clear_is_done() {m_is_done = false;};
	bool is_done() {return m_is_done;};
	void clear_is_good() {m_is_good = true;};
	bool is_good() {return m_is_good;};
protected:
	static int c_state;
	int m_id;
	bool m_is_done;
	bool m_is_good;
	std::thread *m_sm;
	static std::mutex *c_dtmtx;

	virtual void state_machine(void *);
	virtual int st_init(void *);
	virtual int st_idle(void *);
	virtual int st_running(void *);
private:
};
static std::mutex g_dtmtx;
std::mutex* DAQTask::c_dtmtx = &g_dtmtx;
int DAQTask::c_state = SM_INIT;


DAQTask::DAQTask(int id) : m_id(id), m_is_done(false)
{
	return;
}

DAQTask::~DAQTask()
{
	std::cout << "DAQTask " << m_id << " destructed." << std::endl;
	return;
}

std::thread* DAQTask::run(void *context)
{
	//m_sm = new std::thread(&DAQTask::state_machine, this, context);
	m_sm = new std::thread([this](void *c){state_machine(c);}, context);
	//m_sm->detach();

	return m_sm;
}

void DAQTask::state_machine(void *context)
{
	//c_dtmtx->lock();
	//std::cout << "#sm start# " << m_id << " : " << c_state << std::endl;
	//c_dtmtx->unlock();

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

int DAQTask::st_init(void *context)
{
	c_dtmtx->lock();
	std::cout << "Th." << m_id << " init" << std::endl;
	c_dtmtx->unlock();

	m_is_done = true;
	return 0;
}

int DAQTask::st_idle(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "Th." << m_id << " " << c_state << " idle" << std::endl;
	}
	usleep(100000);

	m_is_done = true;
	return 0;
}

int DAQTask::st_running(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "Th." << m_id << " running" << std::endl;
	}
	usleep(100000);

	m_is_done = true;
	return 0;
}


#ifdef TEST_MAIN
class SDAQTask : public DAQTask
{
	//using DAQTask::DAQTask;
public:
	SDAQTask(int i) : DAQTask(i) {};
	//virtual ~SDAQTask() override;
protected:
	virtual int st_idle(void *) override;
	virtual int st_running(void *) override;
private:
};

/*
SDAQTask::~SDAQTask()
{
	std::cout << "SDAQTask " << m_id << " destructed." << std::endl;
	return;
}
*/

int SDAQTask::st_idle(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "Sub Th." << m_id << " idle" << std::endl;
	}
	usleep(100000);

	m_is_done = true;
	return 0;
}
int SDAQTask::st_running(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "Sub Th." << DAQTask::m_id << " running" << std::endl;
	}
	usleep(100000);

	m_is_done = true;
	return 0;
}


int main(int argc, char* argv[])
{
	zmq::context_t context(1);


	DAQTask *avant = new DAQTask(1);
	SDAQTask *mid  = new SDAQTask(2);
	SDAQTask *rear = new SDAQTask(3);

	std::vector<DAQTask*> tasks;
	tasks.push_back(avant);
	tasks.push_back(mid);
	tasks.push_back(rear);

	for (auto &i : tasks)  i->run(&context);
	for (auto &i : tasks)  i->clear_is_done();

	avant->set_state(SM_INIT);
	for (auto &i : tasks) while (i->is_done() != true) usleep(10);


	mid->set_state(SM_IDLE);
	sleep(1);
	mid->set_state(SM_RUNNING);
	sleep(2);
	mid->set_state(SM_IDLE);
	sleep(1);
	mid->set_state(SM_END);


	for (auto &i : tasks) i->get_thread()->join();
	for (auto &i : tasks) delete i;

	std::cout << "main end" << std::endl;

	return 0;
}
#endif

#endif // INC_DAQ_TASK
