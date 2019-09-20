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

//extern int wid[XITAO_MAXTHREADS];
#if defined(CRIT_PERF_SCHED)
extern int TABLEWIDTH;
float Synth_MatMul::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
float Synth_MatCopy::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
float Synth_MatStencil::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#endif

int
main(int argc, char *argv[])
{
  if(argc != 7) {
    std::cout << "./synbench <Block Side Length> <Resource Width> <TAO Mul Count> <TAO Copy Count> <TAO Stencil Count> <Degree of Parallelism>" << std::endl; 
    return 0;
  }
	
	const int arr_size = 1 << 28;
  real_t *A = new real_t[arr_size];
  real_t *B = new real_t[arr_size];
  real_t *C = new real_t[arr_size];
  memset(A, rand(), sizeof(real_t) * arr_size);
  memset(B, rand(), sizeof(real_t) * arr_size);
  memset(C, rand(), sizeof(real_t) * arr_size);

	int indx = 0;	

  const int tao_types = 3;
  int len = atoi(argv[1]);
  int resource_width = atoi(argv[2]); 
  int tao_mul = atoi(argv[3]);
  int tao_copy = atoi(argv[4]);
  int tao_stencil = atoi(argv[5]);
  int parallelism = atoi(argv[6]);
  int total_taos = tao_mul + tao_copy + tao_stencil;
  int nthreads = XITAO_MAXTHREADS;
   //DOT graph output
  std::ofstream graphfile;
  graphfile.open ("graph.txt");
  graphfile << "digraph DAG{\n";
  // init XiTAO runtime 
  gotao_init();
  
  int current_type = 0;
  int previous_tao_id = 0;
  int current_tao_id = 0;
  AssemblyTask* previous_tao;
  AssemblyTask* startTAO;
  graphfile << previous_tao_id << "  [fillcolor = lightpink, style = filled];\n";
  // create first TAO
  if(tao_mul > 0) {
    //previous_tao = new Synth_MatMul(len, resource_width);
		previous_tao = new Synth_MatMul(len, resource_width,  A + indx * len * len, B + indx * len * len, C + indx * len * len);
    tao_mul--;
		indx++;
		if((indx + 1) * len * len > arr_size) indx = 0;
  } else if(tao_copy > 0){
    //previous_tao = new Synth_MatCopy(len, resource_width);
		previous_tao = new Synth_MatCopy(len, resource_width,  A + indx * len * len, B + indx * len * len);
    tao_copy--;
		indx++;
		if((indx + 1) * len * len > arr_size) indx = 0;
  } else if(tao_stencil > 0) {
    //previous_tao = new Synth_MatStencil(len, resource_width);
		previous_tao = new Synth_MatStencil(len, resource_width, A + indx * len * len, B + indx * len * len);
    tao_stencil--;
		indx++;
    if((indx + 1) * len * len > arr_size) indx = 0;
  }
  startTAO = previous_tao;
  previous_tao->criticality = 1;
  total_taos--;
  for(int i = 0; i < total_taos; i+=parallelism) {
    AssemblyTask* new_previous_tao;
    int new_previous_id;
    for(int j = 0; j < parallelism; ++j) {
      AssemblyTask* currentTAO;
      switch(current_type) {
        case 0:
          if(tao_mul > 0) { 
            //currentTAO = new Synth_MatMul(len, resource_width);
						currentTAO = new Synth_MatMul(len, resource_width, A + indx * len * len, B + indx * len * len, C + indx * len * len);
            previous_tao->make_edge(currentTAO);                                 
            graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
            graphfile << current_tao_id << "  [fillcolor = lightpink, style = filled];\n";
            tao_mul--;
						indx++;
            if((indx + 1) * len * len > arr_size) indx = 0;
            break;
          }
        case 1: 
          if(tao_copy > 0) {
            //currentTAO = new Synth_MatCopy(len, resource_width);
						currentTAO = new Synth_MatCopy(len, resource_width, A + indx * len * len, B + indx * len * len);
            previous_tao->make_edge(currentTAO); 
            graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
            graphfile << current_tao_id << "  [fillcolor = skyblue, style = filled];\n";
            tao_copy--;
						indx++;
            if((indx + 1) * len * len > arr_size) indx = 0;
            break;
          }
        case 2: 
          if(tao_stencil > 0) {
            //currentTAO = new Synth_MatStencil(len, resource_width);
						currentTAO = new Synth_MatStencil(len, resource_width, A + indx * len * len, B + indx * len * len);
            previous_tao->make_edge(currentTAO); 
            graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
            graphfile << current_tao_id << "  [fillcolor = palegreen, style = filled];\n";
            tao_stencil--;
						indx++;
            if((indx + 1) * len * len > arr_size) indx = 0;
            break;
          }
        default:
          if(tao_mul > 0) { 
            //currentTAO = new Synth_MatMul(len, resource_width);
						currentTAO = new Synth_MatMul(len, resource_width, A + indx * len * len, B + indx * len * len, C + indx * len * len);
            previous_tao->make_edge(currentTAO); 
            graphfile << "  " << previous_tao_id << " -> " << ++current_tao_id << " ;\n";
            graphfile << current_tao_id << "  [fillcolor = lightpink, style = filled];\n";
            tao_mul--;
            break;
          }
      }
      
      if(j == 0) {
        new_previous_tao = currentTAO;
        new_previous_tao->criticality = 1;
        new_previous_id = current_tao_id;
      }
      current_type++;
      if(current_type >= tao_types) current_type = 0;
    }
    previous_tao = new_previous_tao;
    previous_tao_id = new_previous_id;
  }
  //close the output
  graphfile << "}";
  graphfile.close();

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

#if defined(CRIT_PERF_SCHED)  
  Synth_MatMul::print_ptt(Synth_MatMul::time_table, "MatMul");
  Synth_MatCopy::print_ptt(Synth_MatCopy::time_table, "MatCopy");
  Synth_MatStencil::print_ptt(Synth_MatStencil::time_table, "MatStencil");
#endif

  std::cout << total_taos + 1 <<" Tasks completed in "<< elapsed_seconds.count() << "s\n";
  std::cout << "Assembly Throughput: " << (total_taos) / elapsed_seconds.count() << " A/sec\n"; 
  std::cout << "Total number of steals: " <<  tao_total_steals << "\n";
}
