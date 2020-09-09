include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3 -Wall -Wno-unknown-pragmas -fopenmp
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread -fopenmp
PROGRAM_OBJS=matvec.o 

all: matvec

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

matvec: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: matvec
	./matvec 1024

clean:
	rm -rf matvec $(PROGRAM_OBJS) html latex


