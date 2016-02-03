# small test

all:
	g++ parallel_for.cxx  ../tao_sched.cxx  -DDISTRIBUTED_QUEUES  -DLOCK_FREE_QUEUE  -DSPINLOCK  -DSUPERTASK_STEALING -o par_test --std=c++11  -pthread  -lm  -L/usr/lib -ltbbmalloc_proxy -ltbbmalloc -Wl,-rpath=/usr/lib -I..

clean:
	rm par_test
