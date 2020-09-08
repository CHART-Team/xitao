include ../../makefile.sched
CXXFLAGS += -I../../include -I.

ifeq ($(DEBUG),1)
	CXXFLAGS += -g
else
	CXXFLAGS += -O3 -DINIT_TAO
endif

#CXXFLAGS += -g3 # -DINIT_TAO
CXXFLAGS += --std=c++11  ${EXTRAEINC}
#CXX=g++
LIBS += -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread $(LDFLAGS)  
PROGRAM_OBJS=sparselu.o  

all: sparselu 

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

 
