#include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3 -fopenmp
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread -fopenmp
PROGRAM_OBJS=merge_sort.o 

all: merge_sort

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

merge_sort: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: merge_sort
	./merge_sort 10000000 10000 0 --xitao_args="-m0 -p0 -t8"

clean:
	rm -rf merge_sort $(PROGRAM_OBJS) html latex
