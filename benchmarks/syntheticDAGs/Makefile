include ../../makefile.sched

CXXFLAGS += -I../../include 
CXXFLAGS += -O2
CXXFLAGS += --std=c++11  ${EXTRAEINC}

CXXFLAGS += -DDOUBLE
#CXXFLAGS += -DSINGLE

LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
SYNTH_OBJS=synthdag.o  

all: synbench

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

synbench: $(SYNTH_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

chains: $(CHAINS_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: all 
	./synbench 64 4 5000 1000 1000 4

clean:
	rm -rf synbench $(SYNTH_OBJS)  graph.txt 


