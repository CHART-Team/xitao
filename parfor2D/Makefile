# small test

include ../makefile.inc

FLAGS=-DLOCK_FREE_QUEUE -DTAO_PLACES -DSPINLOCK  -DSUPERTASK_STEALING 


par_test: parallel_for_2D.cxx  ../tao_sched.cxx
	g++  $^ -o $@ --std=c++11  -pthread  -lm  -L$(TBBLIB) -ltbbmalloc_proxy -ltbbmalloc -Wl,-rpath=$(TBBLIB) -I..

clean:
	rm par_test
