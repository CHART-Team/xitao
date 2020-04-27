include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=helloworld.o 

all: helloworld

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

helloworld: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: helloworld
	./helloworld 

clean:
	rm -rf helloworld $(PROGRAM_OBJS) html latex
