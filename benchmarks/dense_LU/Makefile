include ../../makefile.sched
CC=gcc
CXX=g++
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=lud.o 

all: lud

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lud: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: lud
	./lud

clean:
	rm -rf lud $(PROGRAM_OBJS) html latex


