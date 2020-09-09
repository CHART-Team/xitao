/*! @example 
 @brief A program that calculates dotproduct of random two vectors in parallel\n
 we run the example as ./dotprod.out <len> <width> <block> \n
 where
 \param len  := length of vector\n
 \param width := width of TAOs\n
 \param block := how many elements to process per TAO
*/
#include "taos_dotproduct.h"
#include "xitao.h"
using namespace xitao;
int
main(int argc, char *argv[])
{
  double *A, *B, *C, D; 
  if(argc != 4) {
    std::cout << "./a.out <veclength> <TAOwidth> <blocklength>" << std::endl; 
    return 0;
  }

  int len = atoi(argv[1]);
  int width = atoi(argv[2]);
  int block = atoi(argv[3]);

  // For simplicity, only support only perfect partitions
  if(len % block){  
    std::cout << "len is not a multiple of block!" << std::endl;
    return 0;
  }
  
  //cpu_set_t cpu_;
  //CPU_ZERO(&cpu_);
  //for(int i = 0; i < 8; i+=1) {
  //  CPU_SET(i, &cpu_);
  //} 
  //set_xitao_mask(cpu_);
  
  // no topologies in this version
  A = new double[len];
  B = new double[len];
  C = new double[len];

  // initialize the vectors with some numbers
  for(int i = 0; i < len; i++){
    A[i] = (double) (i+1);
    B[i] = (double) (i+1);
  }

  // init XiTAO runtime 
  gotao_init();
  
  // create numvm TAOs 
  int numvm = len / block;

  // static or dynamic internal TAO scheduler
#ifdef STATIC
  VecMulSta *vm[numvm];  
#else
  VecMulDyn *vm[numvm];
#endif  
  VecAdd *va = new VecAdd(C, &D, len, width);
  
  // Create the TAODAG
  for(int j = 0; j < numvm; j++){
#ifdef STATIC
    vm[j] = new VecMulSta(A+j*block, B+j*block, C+j*block, block, width);
#else
    vm[j] = new VecMulDyn(A+j*block, B+j*block, C+j*block, block, width);
#endif
    //Create an edge
    vm[j]->make_edge(va);
    //Push current root to assigned queue
    gotao_push(vm[j], j % gotao_nthreads);
  } 
  //Start the TAODAG exeuction
  gotao_start();
  //Finalize and claim resources back
  gotao_fini();
  std::cout << "Result is " << D << std::endl;
  std::cout << "Done!\n";
  std::cout << "Total successful steals: " << tao_total_steals << std::endl;
}
