CXXFLAGS += -I../../include -I.
CXXFLAGS += -O3
CXXFLAGS += --std=c++11  ${EXTRAEINC}
CXX=g++
LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=dataparallel_dag.o 

all: dataparallel_dag

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

dataparallel_dag: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: dataparallel_dag 
	./dataparallel_dag 1500 4 1 10 1 10 

clean:
	rm -rf dataparallel_dag $(PROGRAM_OBJS) html latex
