include ../../makefile.sched
include makefile.dot
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=dotprod.o 

all: dotprod

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

dotprod: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: dotprod
	./dotprod 8192 2 16

clean:
	rm -rf dotprod $(PROGRAM_OBJS) html latex


