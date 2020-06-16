# C compiler
include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O0 
CXXFLAGS += -g3 -gdwarf-4 -fopenmp --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread -lgomp

PROGRAM_OBJS=hotspot.o

all: hotspot 

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

hotspot: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: hotspot
	./hotspot 64 64 2 4 4 0 0 ./data/temp_64 ./data/power_64 output.out

clean:
	rm -rf hotspot $(PROGRAM_OBJS)
