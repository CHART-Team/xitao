XITAO_HOME=../../
#CXX = g++ 
CPPFLAGS+= -g -O3 -std=c++11 -Wall -Wfatal-errors -fopenmp  
LDFLAGS+=-lhwloc -lnuma
all:
	@make fmm-xitao
	@make fmm-omp

kernel: kernel.cxx
	$(CXX) $? -o $@
	./kernel 10
	./kernel 20
	./kernel 30

fmm-xitao: fmm.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DUSE_XITAO -I${XITAO_HOME}/include $? -o $@ -L${XITAO_HOME}/lib -lxitao ${LDFLAGS}

fmm-omp: fmm.cxx
	$(CXX) $(CPPFLAGS) $? -o fmm-omp 

run-xitao: fmm-xitao
	./fmm-xitao 100000 32 --xitao_args="-p0 -i10"

run-omp: fmm-omp
	./fmm-omp 100000 32

clean:
	$(RM) ./*.o ./kernel ./fmm ./fmm-omp ./fmm-xitao
