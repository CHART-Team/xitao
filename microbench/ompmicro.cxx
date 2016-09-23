
#include <chrono>
#include <iostream>
#include <omp.h>

#define NUM_THREADS 4

struct aligned_int{
           int x __attribute__((aligned(64)));
};

aligned_int counter[NUM_THREADS];

int main()
{
        std::chrono::time_point<std::chrono::system_clock> start, end;
  
        int iter = 1000000;  // total iterations
        int i, j; // index variables
        int threads;

        {
//        threads = omp_get_num_threads();

        std::cout << "Number of threads is " << threads << std::endl;
        start = std::chrono::system_clock::now();

        for(i =0; i < iter; i++) 
#pragma omp parallel for schedule(static,1) 
                for(j=0; j<NUM_THREADS; j++) {
                        counter[j].x++;
                   }

        }
        end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
        std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
        std::cout << "Task Throughput: " << (iter*NUM_THREADS) / elapsed_seconds.count() << " t/sec\n";
        std::cout << "Task Cycle: " << elapsed_seconds.count() / (iter*NUM_THREADS) << " sec/t\n";
}
