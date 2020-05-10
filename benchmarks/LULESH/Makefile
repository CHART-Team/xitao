
include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3 -g -DUSE_MPI=0 -DXITAO
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=lulesh-init.o  lulesh.o  lulesh-util.o  lulesh-viz.o

all: lulesh-xitao

%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

lulesh-xitao: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

clean:
	rm -rf lulesh-xitao $(PROGRAM_OBJS) 
