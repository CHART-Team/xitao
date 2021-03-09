#include "merge_sort.h"
#include <vector>
using namespace std;
int main(int argc, char** argv)
{
    if(argc < 3) { 
      cout << "usage: ./merge_sort matrix_size leaf_cell_size" <<endl;
      exit(0);
    }
    uint32_t n = atoi(argv[1]) ;
    leaf = atoi(argv[2]);
   // vector<uint32_t> A (n);
    uint32_t* A = new uint32_t[n];
    for(int i = 0; i < n; ++i) A[i] = rand() % n; 
    // initialize the XiTAO runtime system 
    xitao_init(argc, argv);

    std::cout << "Building the DAG ..." << std::endl;
    
    // recursively build the DAG
    auto parent = buildDAG(&A[0], n);

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

    // print out the result
    //if(is_sorted(A.begin(), A.end())) cout << "Success" << endl;
    if(is_sorted(A, A + n)) cout << "Success" << endl;
    
    else cout << "Failure " << endl;
    // return success
    return 0;
}
