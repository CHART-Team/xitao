#include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3 -fopenmp
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread -fopenmp
PROGRAM_OBJS=matmul.o 

all: matmul

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

matmul: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: matmul
	./matmul 256 32 --xitao_args="-m0 -p0 -t8"

clean:
	rm -rf matmul $(PROGRAM_OBJS) html latex
