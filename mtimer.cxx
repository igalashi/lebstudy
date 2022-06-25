/*
 *
 *
 */

#ifndef INC_MSTOPWATCH
#define INC_MSTOPWATCH

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

class mTimer {
	public:
		#if 0
		mTimer();
		virtual ~mTimer();
		#endif
		int set(int);
		int check();
		int period();
	protected:
	private:
		int set_time;
		int count_time = 0;
		struct timeval at_start;
		struct timeval now;
};

#if 0
mTimer::mTimer()
{
	return;
}

mTimer::~mTimer()
{
	std::cout << "# StopWatch destructed" << std::endl;
	return;
}
#endif

int mTimer::set(int time)
{
	set_time = time;
	int ret = gettimeofday(&at_start, NULL);
	if (ret != 0) perror("mTimer::set");
	
	return ret;
}

int mTimer::check()
{
	int ret = gettimeofday(&now, NULL);
	if (ret != 0) {
		perror("mTimer::check");
		return ret;
	}
	int diff = period();
	bool retval = false;
	if (diff >= count_time) {
		retval =  true;
		count_time += set_time;
		while(count_time < period()) count_time += set_time;
	}
	
	return retval;
}

int mTimer::period()
{
	int dsec = now.tv_sec - at_start.tv_sec;
	int dusec = now.tv_usec - at_start.tv_usec;
	int diff = dsec * 1000 + (dusec / 1000);
	return diff ;
}

#ifdef TEST_MAIN
int main(int argc, char *argv[])
{
	mTimer tm;
	tm.set(100);
	for (int i = 0 ; i < 100 ; i++) {
		usleep(10 * 1000);
		if (tm.check()) {
			std::cout << "bow : " << tm.period() << std::endl;;
		} else {
			std::cout << "    : " << tm.period() << std::endl;;
		}
	}

	return 0;
}
#endif

#endif
