#
#
#

CC = gcc
CFLAGS = -Wall -g -O
INCLUDES =
LIBS =

CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -g -O


all: udpreg rbcp rbcpserver

udpreg: udpreg.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

rbcp: rbcp.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DTEST_MAIN \
		-o $@ $< $(LIBS)
		
rbcpserver: rbcpserver.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DTEST_SERVER \
		-o $@ $< $(LIBS)
clean:
	rm -f udpreg rbcp rbcpserver
