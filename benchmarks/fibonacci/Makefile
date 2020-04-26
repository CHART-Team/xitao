
include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=fibonacci.o 

all: fibonacci

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

fibonacci: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: fibonacci
	./fibonacci 45 43

clean:
	rm -rf fibonacci $(PROGRAM_OBJS) html latex
