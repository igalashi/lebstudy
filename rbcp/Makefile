#
#
#

CC = gcc
CFLAGS = -Wall -g -O
INCLUDES =
LIBS =

CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -g -O

PROGS = udpreg rbcp rbcpserver rbcprelay
all: $(PROGS)

udpreg: udpreg.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

rbcp: rbcp.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DTEST_MAIN \
		-o $@ $< $(LIBS)
		
rbcpserver: rbcpserver.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DTEST_SERVER \
		-o $@ $< $(LIBS)

rbcprelay: rbcprelay.cxx rbcp.cxx rbcpserver.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DTEST_RELAY \
		-o $@ $< $(LIBS)

clean:
	rm -f $(PROGS)
