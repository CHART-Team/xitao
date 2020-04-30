include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O2

CXXFLAGS += -fopenmp --std=c++11 -fno-unroll-loops ${EXTRAEINC}

LIBS = -fopenmp -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=dataparallel.o 

all: dataparallel

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

dataparallel: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: dataparallel 
	./dataparallel 1500 4 1 10 1 10

clean:
	rm -rf dataparallel $(PROGRAM_OBJS) html latex
