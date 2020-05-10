include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=sparselu.o  

all: sparselu sparselu_serial

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

sparselu: sparselu.o
	$(CXX) $^ -o $@ ${LIBS}

draw_dag: 
	dot -Tps sparselu.dot -o sparselu.eps; evince sparselu.eps

run: sparselu
	./sparselu

clean:
	rm -rf sparselu *.eps *.dot $(PROGRAM_OBJS) html latex


