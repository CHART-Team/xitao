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
#include "synthstencil.h"
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
float Synth_MatStencil::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
#endif

int
main(int argc, char *argv[])
{
  if(argc != 7) {
    std::cout << "./synbench <Block Side Length> <Resource Width> <TAO Mul Count> <TAO Copy Count> <TAO Stencil Count> <Degree of Parallelism>" << std::endl; 
    return 0;
  }
  const int tao_types = 3;
  int len = atoi(argv[1]);
  int resource_width = atoi(argv[2]); 
  int tao_mul = atoi(argv[3]);
  int tao_copy = atoi(argv[4]);
  int tao_stencil = atoi(argv[5]);
  int parallelism = atoi(argv[6]);
  int total_taos = tao_mul + tao_copy + tao_stencil;
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
  } else if(tao_stencil > 0) {
    previousTAO = new Synth_MatStencil(len, resource_width);
    tao_copy--;
  }
  startTAO = previousTAO;
  previousTAO->criticality = 1;
  total_taos--;
  for(int i = 0; i < total_taos; i+=parallelism) {
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
        case 2: 
          if(tao_stencil > 0) {
            currentTAO = new Synth_MatStencil(len, resource_width);
            previousTAO->make_edge(currentTAO); 
            tao_stencil--;
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

  std::cout << total_taos + 1 <<" Tasks completed in "<< elapsed_seconds.count() << "s\n";
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
  std::cout<<std::endl<<"TAO MMul PTT| ";
  //std::cout<< std::setfill(' ') << std::setw(15) << " ";
  for(int threads =0; threads<nthreads; threads++)
  {
    std::cout << "Th " << std::setfill('0') << std::setw(4) << threads << "   | "; 
  }
  std::cout<<std::endl<<"---------------------------------------------------------------------------------------------------------------"<<std::endl;


  for (int count=0; count < TABLEWIDTH; count++)
  {
    std::cout << std::setfill(' ') << std::setw(11) << "width = " << wid[count] << "|";
    for(int threads =0; threads<nthreads; threads++)
    {
      std::cout << std::setfill(' ') << std::setw(11) << Synth_MatMul::time_table[count][threads] << "|";
    }
    std::cout<<std::endl<<"---------------------------------------------------------------------------------------------------------------"<<std::endl;

  }

  std::cout<<std::endl<<"TAO Copy PTT| ";
  //std::cout<< std::setfill(' ') << std::setw(15) << " ";
  for(int threads =0; threads<nthreads; threads++)
  {
    std::cout << "Th " << std::setfill('0') << std::setw(4) << threads << "   | ";  
  }
  std::cout<<std::endl<<"---------------------------------------------------------------------------------------------------------------"<<std::endl;

  for (int count=0; count < TABLEWIDTH; count++)
  {
    std::cout << std::setfill(' ') << std::setw(11) << "width = " << wid[count] << "|";
    for(int threads =0; threads< nthreads; threads++)
    {
      std::cout << std::setfill(' ') << std::setw(11) << Synth_MatCopy::time_table[count][threads] << "|";
    }
    std::cout<<std::endl<<"---------------------------------------------------------------------------------------------------------------"<<std::endl;
  }

  std::cout<<std::endl<<"TAO Sten PTT| ";
  //std::cout<< std::setfill(' ') << std::setw(15) << " ";
  for(int threads =0; threads<nthreads; threads++)
  {
    std::cout << "Th " << std::setfill('0') << std::setw(4) << threads << "   | "; 
  }
  std::cout<<std::endl<<"---------------------------------------------------------------------------------------------------------------"<<std::endl;


  for (int count=0; count < TABLEWIDTH; count++)
  {
    std::cout << std::setfill(' ') << std::setw(11) << "width = " << wid[count] << "|";
    for(int threads =0; threads< nthreads; threads++)
    {
      std::cout << std::setfill(' ') << std::setw(11) << Synth_MatStencil::time_table[count][threads] << "|";
    }
    std::cout<<std::endl<<"---------------------------------------------------------------------------------------------------------------"<<std::endl;
  }
  
#endif
}
