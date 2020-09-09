include ../../makefile.sched
CXXFLAGS += -I../../include 
CXXFLAGS += -O0
CXXFLAGS += --std=c++11  ${EXTRAEINC}

LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
RANDOM_OBJS=randombench.o sortlib.o 
CHAINS_OBJS=chains.o sortlib.o 

all: randombench

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

randombench: $(RANDOM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

chains: $(CHAINS_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: all 
	./randombench 10 5 1000 1000 1000

clean:
	rm -rf randombench chains $(RANDOM_OBJS) $(CHAINS_OBJS) graph.txt 


