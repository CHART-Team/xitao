#include "merge_sort.h"

int main(int argc, char** argv)
{
    uint32_t n = 10;
    uint32_t A[] = {13, 2, 45, 3, 6, 55, 32, 10, 67, 28};
        
    // initialize the XiTAO runtime system 
    xitao_init();

    std::cout << "Input array: ";
    for(uint32_t i = 0; i < n; i++)
	std::cout << A[i] << " ";
    std::cout << std::endl;
   
    std::cout << "Building the DAG ..." << std::endl;
    
    // recursively build the DAG
    auto parent = buildDAG(A, n);

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
    std::cout << "Output array: " ;    
    for(int i = 0; i < n; i++)
	std::cout << A[i] << " ";
    std::cout << std::endl;
    // return success
    return 0;
}
