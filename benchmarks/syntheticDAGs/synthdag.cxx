/*! @example 
 @brief A program that calculates dotproduct of random two vectors in parallel\n
 we run the example as ./dotprod.out <len> <width> <block> \n
 where
 \param len  := length of vector\n
 \param width := width of TAOs\n
 \param block := how many elements to process per TAO
*/
#include "synthmat.h"
#include "synthcopy.h"
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <vector>
#include <algorithm>

extern int wid[GOTAO_NTHREADS];
#if defined(CRIT_PERF_SCHED)
extern int TABLEWIDTH;
float Synth_MatMul::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
float Synth_MatCopy::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
#endif

int
main(int argc, char *argv[])
{
  if(argc != 6) {
    std::cout << "./a.out <Block Side Length> <Resource Width> <TAO Mul Count> <TAO Copy Count> <Degree of Parallelism>" << std::endl; 
    return 0;
  }
  const int tao_types = 2;
  int len = atoi(argv[1]);
  int resource_width = atoi(argv[2]); 
  int tao_mul = atoi(argv[3]);
  int tao_copy = atoi(argv[4]);
  int parallelism = atoi(argv[5]);
  int total_taos = tao_mul + tao_copy;
  int nthreads = GOTAO_NTHREADS;

  // init XiTAO runtime 
  gotao_init();
  
  int current_type = 0;
  AssemblyTask* previousTAO;
  AssemblyTask* startTAO;


  // create first TAO
  if(tao_mul > 0) {
    previousTAO = new Synth_MatMul(len, resource_width);
    tao_mul--;
  } else if(tao_copy > 0){
    previousTAO = new Synth_MatCopy(len, resource_width);
    tao_copy--;
  }
  startTAO = previousTAO;
  previousTAO->criticality = 1;
  total_taos--;
  for(int i = 0; i < total_taos; ++i) {
    for(int j = 0; j < parallelism; ++j) {
      AssemblyTask* currentTAO;
      switch(current_type) {
        case 0:
          if(tao_mul > 0) { 
            currentTAO = new Synth_MatMul(len, resource_width);
            previousTAO->make_edge(currentTAO); 
            tao_mul--;
            break;
          }
        case 1: 
          if(tao_copy > 0) {
            currentTAO = new Synth_MatCopy(len, resource_width);
            previousTAO->make_edge(currentTAO); 
            tao_copy--;
            break;
          }
        default:
          if(tao_mul > 0) { 
            currentTAO = new Synth_MatMul(len, resource_width);
            previousTAO->make_edge(currentTAO); 
            tao_mul--;
            break;
          }
      }
      if(j == 0) {
        previousTAO = currentTAO;
        previousTAO->criticality = 1;
      }

      current_type++;
      if(current_type >= tao_types) current_type = 0;
    }
  }
  gotao_push(startTAO, 0);
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  auto start1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(start);
  auto epoch1 = start1_ms.time_since_epoch();

  goTAO_start();

  goTAO_fini();

  end = std::chrono::system_clock::now();

  auto end1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(end);
  auto epoch1_end = end1_ms.time_since_epoch();
  std::chrono::duration<double> elapsed_seconds = end-start;

  std::cout << total_taos <<" Tasks completed in "<< elapsed_seconds.count() << "s\n";
  std::cout << "Assembly Throughput: " << (total_taos) / elapsed_seconds.count() << " A/sec\n"; 
  std::cout << "Total number of steals: " <<  tao_total_steals << "\n";

#if defined(CRIT_PERF_SCHED)
  int cut = 2;
  for(int count = 2; count < nthreads; count++)
  {
    if(nthreads % count == 0)
    {
      cut++;  
    }
  }
  std::cout<<"TAO Matrix PTT:\n";
  std::cout<< std::setfill(' ') << std::setw(15) << " ";
  for(int threads =0; threads<nthreads; threads++)
  {
    std::cout << "Th " << threads << std::setfill(' ') << std::setw(9) << " " ; 
  }
  std::cout<<"\n";

  for (int count=0; count < TABLEWIDTH; count++)
  {
    std::cout << std::setfill(' ') << std::setw(11) << "width=" << wid[count] << "  ";
    for(int threads =0; threads<nthreads; threads++)
    {
      std::cout << std::setfill(' ') << std::setw(11) << Synth_MatMul::time_table[count][threads] << "  ";
    }
    std::cout << "\n";
  }

  std::cout<<"TAO Copy PTT:\n";
  std::cout<< std::setfill(' ') << std::setw(15) << " ";
  for(int threads =0; threads<nthreads; threads++)
  {
    std::cout << "Th " << threads << std::setfill(' ') << std::setw(9) << " " ; 
  }
  std::cout<<"\n";

  for (int count=0; count < TABLEWIDTH; count++)
  {
    std::cout << std::setfill(' ') << std::setw(11) << "width=" << wid[count] << "  ";
    for(int threads =0; threads< nthreads; threads++)
    {
      std::cout << std::setfill(' ') << std::setw(11) << Synth_MatCopy::time_table[count][threads] << "  ";
    }
    std::cout << "\n";
  }
#endif
  std::cout << "***************** XITAO RUNTIME *****************\n";


}
