include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=sparselu.o sparselu_serial.o 

all: sparselu sparselu_serial

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

sparselu: sparselu.o
	$(CXX) $^ -o $@ ${LIBS}

sparselu_serial: sparselu_serial.o
	$(CXX) $^ -o $@ ${LIBS}

run: sparselu
	./sparselu

clean:
	rm -rf sparselu sparselu_serial $(PROGRAM_OBJS) html latex


