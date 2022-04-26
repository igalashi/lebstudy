#
#
#
CC = gcc
CFLAGS = -Wall -g -O
CLIBS = -lzmq

CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -g -O
CXXLIBS = -lzmq

INCLUDES = -Ikol
#LIBS = kol/kolsocket.o kol/koltcp.o
LIBS = -Lkol -lkol

PROGS = kol/libkol.a drecbe checkrecbe daqtask dtmain \
	leb nodelist triggen trigrecv ebreceiver checkleb
all: $(PROGS)

kol/libkol.a: kol/kolsocket.h kol/kolsocket.cpp kol/koltcp.h kol/koltcp.cpp
	(cd kol; $(MAKE))

fe: fe.cxx zportname.cxx kol/libkol.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

be: be.cxx filename.cxx zportname.cxx  mstopwatch.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

fctreader: fctreader.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		-Ikol \
		fctreader.cxx \
		kol/kolsocket.o kol/koltcp.o

thtdcreader: thtdcreader.cxx filename.cxx mstopwatch.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

benb: benb.cxx filename.cxx zportname.cxx  mstopwatch.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

daqtask: daqtask.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -D TEST_MAIN -o $@ $< $(CXXLIBS) $(LIBS)

dtmain: dtmain.cxx daqtask.cxx dtavant.cxx dtrear.cxx dtfilename.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

leb: leb.cxx daqtask.cxx dtarecbe.cxx dteb.cxx dtfilename.cxx nodelist.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

hulssm: dtmain.cxx daqtask.cxx dtavant.cxx dtrearhul.cxx dtfilename.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -D HUL -o $@ $< $(CXXLIBS) $(LIBS)

drecbe: drecbe.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

triggen: triggen.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

trigrecv: trigrecv.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

ebreceiver: ebreceiver.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

checkrecbe: checkrecbe.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

checkleb: checkleb.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

nodelist: nodelist.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -D TEST_MAIN -o $@ $< $(CXXLIBS) $(LIBS)

kol/kollib.a:
	(cd kol; $(MAKE))

clean:
	rm -f $(PROGS)

cleandata:
	rm -f tdc*.dat hul*.dat leb*.dat
