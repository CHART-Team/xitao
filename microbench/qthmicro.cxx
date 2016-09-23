
#include <chrono>
#include <iostream>
#include <qthread.h>

// environment variables that affect this benchmark
// QTHREAD_NUM_SHEPHERDS: set the number of shepherds explicitly
// QTHREAD_NUM_WORKERS_PER_SHEPHERD: set the number of workers per shepherd
// QTHREAD_HWPAR: total amount of hardware parallelism
// QTHREAD_INFO: just print some info on what is going on

aligned_t returnzero(void *args){
        // this function does not do anything except always returning the value 0
        return 0;
}


int main(int argc, char *argv[])
{
        const qthread_shepherd_id_t * sp;
        std::chrono::time_point<std::chrono::system_clock> start, end;
        aligned_t result;
  
        qthread_initialize();                           // initialize qthreads
        int nw = qthread_num_workers();               // the number of workers     
        int ns = qthread_num_shepherds();               // the number of shepherds
        sp = qthread_sorted_sheps ();                   // list of shepherds

        int tt;  // total tasks
	if(argc == 2)
          tt = atoi(argv[1]);
	else 
	  tt = 1000000;
        aligned_t * ret = new aligned_t[tt];

        start = std::chrono::system_clock::now();

        // create a large bunch of empty tasks and time it
        for(int i = 0; i<tt; i++){
                if(i%ns) qthread_fork_to(returnzero, NULL, ret+i, sp[i%ns -1]);
                else qthread_fork(returnzero, NULL, ret+i);
        }

        // now we collect all the results
        for(int i = 0; i<tt; i++){
                aligned_t retm;
                qthread_readFF(&retm, ret+i);        
        }

        end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
        std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
        std::cout << "Assembly Throughput: " << tt / elapsed_seconds.count() << " A/sec\n";
        std::cout << "Assembly Cycle: " << elapsed_seconds.count() / tt  << " sec/A\n";
}
