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

class mStopWatch {
	public:
		#if 0
		mStopWatch();
		virtual ~mStopWatch();
		#endif
		int start();
		int period();
		int elapse();
	protected:
	private:
		struct timeval at_start;
		struct timeval now;
};

#if 0
mStopWatch::mStopWatch()
{
	return;
}

mStopWatch::~mStopWatch()
{
	std::cout << "# StopWatch destructed" << std::endl;
	return;
}
#endif

int mStopWatch::start()
{
	int ret = gettimeofday(&at_start, NULL);
	if (ret != 0) perror("StopWatch::start");
	
	return ret;
}

int mStopWatch::elapse()
{
	int ret = gettimeofday(&now, NULL);
	if (ret != 0) {
		perror("StopWatch::stop");
		return ret;
	}
	
	return period();
}

int mStopWatch::period()
{
	int dsec = now.tv_sec - at_start.tv_sec;
	int dusec = now.tv_usec - at_start.tv_usec;
	int diff = dsec * 1000 + (dusec / 1000);
	return diff ;
}

#ifdef TEST_MAIN
int main(int argc, char *argv[])
{
	mStopWatch sw;
	for (int i = 0 ; i < 10 ; i++) {
		sw.start();
		usleep(i*10000);
		int elapse = sw.elapse();
		std::cout << "elapse : " << elapse << std::endl;;
	}

	return 0;
}
#endif

#endif
