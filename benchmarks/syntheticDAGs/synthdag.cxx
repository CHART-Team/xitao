/* BSD 3-Clause License

* Copyright (c) 2019-2021, contributors
* All rights reserved.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.

* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.

* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
using namespace xitao;

int main(int argc, char *argv[])
{
  if(argc < 7) {
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
  // xitao::config::init_config(argc, argv);
  // init XiTAO runtime 
  xitao_init(argc, argv);
  
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

  xitao_push(startTAO, 0);
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  auto start1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(start);
  auto epoch1 = start1_ms.time_since_epoch();

  xitao_start();

  xitao_fini();

  end = std::chrono::system_clock::now();

  auto end1_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(end);
  auto epoch1_end = end1_ms.time_since_epoch();
  std::chrono::duration<double> elapsed_seconds = end-start;

  if(xitao::config::use_performance_modeling) {
    xitao_ptt::print_table<Synth_MatMul>("MatMul");
    xitao_ptt::print_table<Synth_MatCopy>("MaCopy");
    xitao_ptt::print_table<Synth_MatStencil>("MatStencil");
  }
  std::cout << total_taos + 1 <<" Tasks completed in "<< elapsed_seconds.count() << "s\n";
  std::cout << "Assembly Throughput: " << (total_taos) / elapsed_seconds.count() << " A/sec\n"; 
  std::cout << "Total number of steals: " <<  tao_total_steals << "\n";
}
