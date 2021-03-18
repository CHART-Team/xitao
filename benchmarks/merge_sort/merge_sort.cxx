#include "merge_sort.h"
#include <vector>
using namespace std;
int main(int argc, char** argv)
{
    if(argc < 3) { 
      cout << "usage: ./merge_sort matrix_size leaf_cell_size [use_omp use_sta_in_xitao use_workload_hint]" <<endl;
      exit(0);
    }
    uint32_t n = atoi(argv[1]) ;
    N = n;
    leaf = atoi(argv[2]);
    uint32_t use_omp = 0;
    if(argc > 3) use_omp = atoi(argv[3]);
    if(argc > 4) use_sta = atoi(argv[4]);
    if(argc > 5) use_workload_hint = atoi(argv[5]);

    config::formatted_print("N", N);
    config::formatted_print("Leaf Size", leaf);
    config::formatted_print("Using workload hint", use_workload_hint);
    config::formatted_print("Using STA", use_sta);
    config::formatted_print("Using OpenMP", use_omp);

    vector<uint32_t> A (n);
    //uint32_t* A = new uint32_t[n];
    for(int i = 0; i < n; ++i) A[i] = rand() % n;
    if(use_omp != 0) {
#pragma omp parallel 
{
#pragma omp single
{ 
        xitao::start("Time in OpenMP");
        mergesort_par(&A[0], n);
        xitao::stop("Time in OpenMP");
}
}
    } else { 
        // initialize the XiTAO runtime system 
        xitao_init(argc, argv);

        std::cout << "Building the DAG ..." << std::endl;
        

        // recursively build the DAG
        auto parent = buildDAG(&A[0], n, 0);

        std::cout << "DAG is ready, execute ..." << std::endl;
        
        // start the timer
        xitao::start("Time in XiTAO");
        // fire the XiTAO runtime system                    
        xitao_start();
        std::cout << "Started ... " << std::endl;    
        // end the XiTAO runtime system                       
        xitao_fini();
        std::cout << "Finished!" << std::endl;        
        // stop the timer
        xitao::stop("Time in XiTAO");
    }
    // print out the result
    if(is_sorted(A.begin(), A.end())) cout << "Success" << endl;
    else cout << "Failure " << endl;
    // return success
    return 0;
}
