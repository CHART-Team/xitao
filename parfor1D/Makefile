# small test

include ../makefile.inc
FLAGS=-DLOCK_FREE_QUEUE  -DSPINLOCK  -DSUPERTASK_STEALING

par_test: parallel_for.cxx ../tao_sched.cxx 
	g++ $^ $(FLAGS) -o $@ --std=c++11  -pthread  -lm -I..  -L$(TBBLIB) -ltbbmalloc_proxy -ltbbmalloc -Wl,-rpath=$(TBBLIB)

clean:
	rm par_test
