//
//
//


#include <chrono>
#include <fstream>
#include <iostream>
#include <atomic>
#include <vector>
#include <algorithm>
#include "../../tao.h"
#include "../tabletest/taomatrix_table.h"
#include "../tabletest/solver-tao_table.h"
#include "../tabletest/taosort_table.h"
#include "randombench.h"
#include "config-random-bench.h"



extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}


#define BLOCKSIZE (2*1024)


void fill_arrays(int **a, int **c, int ysize, int xsize);

  //vector used to contain nodes
   std::vector<node> nodes;
#ifdef TIME_TRACE
double TAO_matrix::time_table[GOTAO_NTHREADS][3];
double TAOQuickMergeDyn::time_table[GOTAO_NTHREADS][3];
double copy2D::time_table[GOTAO_NTHREADS][3];
#endif
#ifdef INT_SOL
uint64_t TAO_matrix::time_table[GOTAO_NTHREADS][3];
uint64_t TAOQuickMergeDyn::time_table[GOTAO_NTHREADS][3];
uint64_t copy2D::time_table[GOTAO_NTHREADS][3];
#endif

// MAIN 
int
main(int argc, char* argv[])
{

  int matrix_count; //number of matrix multiplication TAOs
  int sort_count; //sort TAOs count
  int heat_count; //heat/copy TAO count
  int dag_width; //average width of the DAG
  int sort_size; 
  int heat_resolution; // size of copy/heat
  int xdecomp; //internal decomposition of copy/heat
  int ydecomp;
  int ma_width; //matrix assembly width
  int sa_width; //sort assembly width
  int ha_width; //heat/copy assembly width

  //SEED for randomization
  srand(R_SEED);

  //used to keep track of number of generated TAOs of each class in the DAG generation
  int currentheatcount;
  int currentsortcount;
  int currentmatrixcount;

  int edge_rate; //rate in % at which edges will be created between TAOs
  int level_width; //average number of TAOs spawned in parallel


   int thread_base; int nthreads; int nctx; //Do we need the last parameter?
   int r_size;  //size of matric multiplication
   int nqueues;  // how many queues to fill: What is this??
   int nas;      // number of assemblies per queue: what is this??


  

 // set the number of threads and thread_base

   if(getenv("GOTAO_NTHREADS"))
        nthreads = atoi(getenv("GOTAO_NTHREADS"));
   else
        nthreads = GOTAO_NTHREADS;

   if(getenv("GOTAO_THREAD_BASE"))
        thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
   else
        thread_base = GOTAO_THREAD_BASE;

   if(getenv("GOTAO_HW_CONTEXTS"))
        nctx = atoi(getenv("GOTAO_HW_CONTEXTS"));
   else
        nctx = GOTAO_HW_CONTEXTS;

   if(getenv("ROW_SIZE"))
	r_size=atoi(getenv("ROW_SIZE"));
 
   else
	r_size=ROW_SIZE;

   if(getenv("EDGE_RATE"))
  edge_rate=atoi(getenv("EDGE_RATE"));
   else
  edge_rate=EDGE_RATE;

   if(getenv("*LEVEL_WIDTH"))
  level_width=atoi(getenv("LEVEL_WIDTH"));
   else
  level_width=LEVEL_WIDTH;

   if(getenv("MATRIX_COUNT"))
  matrix_count=atoi(getenv("MATRIX_COUNT"));
   else
  matrix_count=MATRIX_COUNT;

  if(getenv("SORT_COUNT"))
  sort_count=atoi(getenv("SORT_COUNT"));
   else
  sort_count=SORT_COUNT;

  if(getenv("SORT_SIZE"))
  sort_size=atoi(getenv("SORT_SIZE"));
   else
  sort_size=SORT_SIZE;

  if(getenv("HEAT_COUNT"))
  heat_count=atoi(getenv("HEAT_COUNT"));
   else
  heat_count=HEAT_COUNT;

  if(getenv("HEAT_RESOLUTION"))
  heat_resolution=atoi(getenv("HEAT_RESOLUTION"));
   else
  heat_resolution=HEAT_RESOLUTION;

  if(getenv("INTERNAL_XDECOMP"))
  xdecomp=atoi(getenv("INTERNAL_XDECOMP"));
   else
  xdecomp=INTERNAL_XDECOMP;

  if(getenv("INTERNAL_YDECOMP"))
  ydecomp=atoi(getenv("INTERNAL_YDECOMP"));
   else
  ydecomp=INTERNAL_YDECOMP;

  if(getenv("M_ASSEMBLY_WIDTH"))
  ma_width=atoi(getenv("M_ASSEMBLY_WIDTH"));
   else
  ma_width=M_ASSEMBLY_WIDTH;

  if(getenv("S_ASSEMBLY_WIDTH"))
  sa_width=atoi(getenv("S_ASSEMBLY_WIDTH"));
   else
  sa_width=S_ASSEMBLY_WIDTH;

  if(getenv("H_ASSEMBLY_WIDTH"))
  ha_width=atoi(getenv("H_ASSEMBLY_WIDTH"));
   else
  ha_width=H_ASSEMBLY_WIDTH;

  if(getenv("R_SEED"))
    srand(R_SEED);

   if(level_width<1){//cant have level width less than one
  std::cout << "level_width must be > 0";
  return 0;
   }

   currentmatrixcount = matrix_count;
   currentsortcount   = sort_count;
   currentheatcount   = heat_count;

   //nodecount to keep track of spwaned nodes
   int nodecount = 0;
   int ic = 0;


   std::cout << "prewhile \n";

   //vectors used to keep track of which input and output data location is free and occupied.
   std::vector<int> matrix_mem;
   std::vector<int> sort_mem;
   std::vector<int> heat_mem;

   //DOT graph output
   std::ofstream graphfile;
   graphfile.open ("graph.txt");
   graphfile << "digraph DAG{\n";

  //lopp while created nodes is less than the amount to spawn
  while(nodecount < (heat_count + sort_count + matrix_count)){

    //w is randomized number of TAOs to spawn for this level in the DAG
    int w = (rand() % (level_width*2-1)+1);
    //current node in this level
    ic = 0;

    //loop while ic < w (and we still have TAOs to spawn)
    while (ic<w && (currentmatrixcount+currentsortcount+currentheatcount)){

      Taotype nodetype;
      int taonumber;

      //generate a number which decides what TAO class curren node is.
      int x = rand() % (currentheatcount + currentsortcount + currentmatrixcount);


      //check x and set the correct TAO class to current node
      if ( ((0 <= x) && (x < currentheatcount)) ){
        nodetype = heat;
        taonumber = heat_count - currentheatcount;
        currentheatcount--;
      }else if ((currentheatcount <= x) && (x < (currentheatcount + currentsortcount))){
        nodetype = sort_;
        taonumber = sort_count - currentsortcount;
        currentsortcount--;
      }else { 
        nodetype = matrix;
        taonumber = matrix_count - currentmatrixcount;
        currentmatrixcount--;
      }

      //create the new node and add it to our nodes vector
      nodes.push_back (new_node(nodetype, //matrix, sort or heat
                                nodecount+ic,
                                taonumber));


      //Check previous nodes(max 30 previous)
      for (int y = 1; y <= (min(30, nodecount)); y++){
        //use edge rate and rand to decide whether to create edge or not
        if ((rand() % 100) < edge_rate){
          //check if that edge is unnecessary to create(by checking if we already are a successor of the node)
          if (!edge_check(nodes[nodecount-y].nodenr, nodes[nodecount+ic])){
            //Add the node to our edge vector
            add_edge(nodes[nodecount+ic], //the newly created node
                     nodes[nodecount-y]); // the node to add
            //update DOT graph
            graphfile << "  " << nodecount-y << " -> " << nodecount+ic << " ;\n";
          }
        } 
      }

      int a;
      //check our edges to find a input/output memory slot to reuse, if we can find it expand the vector with slots
      switch(nodetype){
        case matrix :
          a = find_mem(nodetype, nodecount+ic, nodes[nodecount+ic], matrix_mem);
          add_space(nodes[nodecount+ic], matrix_mem, a);
          break;
        case sort_ :
          a = find_mem(nodetype, nodecount+ic, nodes[nodecount+ic], sort_mem);
          add_space(nodes[nodecount+ic], sort_mem, a);
          break;
        case heat :
          a = find_mem(nodetype, nodecount+ic, nodes[nodecount+ic], heat_mem);
          add_space(nodes[nodecount+ic], heat_mem, a);
      } 


      ic++;

    }

    nodecount = nodecount + ic;
    //std::cout <<"end of while iter\n";
    //std::cout <<"ic: " << ic;
    //std::cout <<"\nw: " << w << "\n";
    std::cout << "nodecount: " << nodecount << "\n";
  }

  //set colors of our nodes in the DOT graph
  for (int i = 0; i < nodes.size(); i++){
    switch(nodes[i].ttype) {
      case matrix :
        graphfile << nodes[i].nodenr << "  [fillcolor = lightpink, style = filled];\n";
        break;
      case sort_ : 
        graphfile << nodes[i].nodenr << "  [fillcolor = skyblue, style = filled];\n";
        break;
      case heat :
        graphfile << nodes[i].nodenr << "  [fillcolor = palegreen, style = filled];\n";
        break;
    }
  }
  //close the output
  graphfile << "}";
  graphfile.close();


std::cout << "martix_mem size: " << matrix_mem.size() << "\n";
std::cout << "sort_mem size:   " << sort_mem.size() << "\n";
std::cout << "heat_mem size:   " << heat_mem.size() << "\n";


//////Memory allocation
// Creates a 2d array for each TAO class with a size depending on 
// the size of the TAOs and the corresponding mem vector(matrix_mem, sort_mem, heat_mem)
  if (matrix_mem.size() == 0){
    r_size = 0;
  }
  int m_ysize = r_size;

  int m_xsize = (r_size * matrix_mem.size() * 2);

int** matrix_input_a = new int* [m_ysize];
int** matrix_output_c = new int* [m_ysize];

for (int i = 0; i < m_ysize; ++i)
{
  matrix_input_a[i] = new int[m_xsize];
  matrix_output_c[i] = new int[m_xsize];
}



  int s_ysize = (sort_mem.size()) + 4;
  int s_xsize = sort_size*BLOCKSIZE;

int** sort_input_a = new int* [s_ysize];
int** sort_output_c = new int* [s_ysize];

for (int i = 0; i < s_ysize; ++i)
{
  sort_input_a[i] = new int[s_xsize];
  sort_output_c[i] = new int[s_xsize];
}



  int h_ysize = (heat_mem.size())+4;
  int h_xsize = heat_resolution*heat_resolution;


int** heat_input_a = new int* [h_ysize];
int** heat_output_c = new int* [h_ysize];

for (int i = 0; i < h_ysize; ++i)
{
  heat_input_a[i] = new int[h_xsize];
  heat_output_c[i] = new int[h_xsize];
}


////END memory alllocation


    std::cout << "gotao_init parameters are: " << nthreads <<"," << thread_base << ","<< nctx << std::endl;

   gotao_init();





   // fill the input arrays and empty the output array
    fill_arrays(matrix_input_a , matrix_output_c, m_ysize, m_xsize);
    fill_arrays(sort_input_a , sort_output_c, s_ysize, s_xsize);
    fill_arrays(heat_input_a , heat_output_c, h_ysize, h_xsize);


    std::cout <<"arrays filled\n";

   
   TAO_matrix *matrix_ao[matrix_count];

   TAOQuickMergeDyn *sort_ao[sort_count];

   copy2D *heat_ao[heat_count];






   //infices to keep track of spawned TAOs
  int i = 0;
  int j = 0;
  int k = 0;


  //for each node in our nodes vector
  for (int x = 0; x < nodecount; x++)
  {
    // alternate input and output between steps of the DAG to make heat and matrix mult data-dependent
     
     //spawn Matrix multiplication tao
    if (nodes[x].ttype == matrix)
    {
      matrix_ao[i] = new TAO_matrix(ma_width, //taowidth
                                    0, //start y
                                    r_size, //stop y
                                    (nodes[x].mem_space)*r_size*2, //start x
                                    r_size+(nodes[x].mem_space)*r_size*2, //stop x
                                    0, //output offset (if needed)
                                    ROW_SIZE, 
                                    matrix_input_a,
                                    matrix_output_c);
      //if our node has no input edges
      if (nodes[x].edges.size() == 0) {
        gotao_push(matrix_ao[i]);
      } else {
        //for each input edge of our node
        for (int y = 0; y < (nodes[x].edges).size(); y++) {
          //create edge from corresponding TAO class
          switch (nodes[((nodes[x]).edges[y])].ttype) {
            case matrix :
              matrix_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(matrix_ao[i]);
              break;
            case sort_ :
              sort_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(matrix_ao[i]);
              break;
            case heat :
              heat_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(matrix_ao[i]);
              break;
          }
        }
      }
      i++;
    }

    //spawn sort TAO
    if (nodes[x].ttype == sort_)
    {
      sort_ao[j] = new TAOQuickMergeDyn(sort_input_a[nodes[x].mem_space],
                                        sort_output_c[nodes[x].mem_space], 
                                        sort_size*BLOCKSIZE,
                                        sa_width);
      //if our node has no input edges
      if (nodes[x].edges.size() == 0) {
        gotao_push(sort_ao[j]);
      } else {
        //for each input edge of our node
        for (int y = 0; y < (nodes[x].edges).size(); y++) {
          //create edge from corresponding TAO class
          switch (nodes[((nodes[x]).edges[y])].ttype) {
            case matrix :
              matrix_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(sort_ao[j]);
              break;
            case sort_ :
              sort_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(sort_ao[j]);
              break;
            case heat :
              heat_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(sort_ao[j]);
              break;
          }
        }
      }
      j++;
    }

    //spawn copy TAO
    if (nodes[x].ttype == heat)
    {
      heat_ao[k] = new copy2D(
                             heat_input_a[(nodes[x].mem_space)+1],
                             heat_output_c[(nodes[x].mem_space)+1],
                             heat_resolution, heat_resolution,
                             0, // x*((np + exdecomp -1) / exdecomp),
                             0, //y*((np + eydecomp -1) / eydecomp),
                             heat_resolution, //(np + exdecomp - 1) / exdecomp,
                             heat_resolution, //(np + eydecomp - 1) / eydecomp, 
                             gotao_sched_2D_static,
                             heat_resolution/xdecomp, // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                             heat_resolution/ydecomp, //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                             ha_width);
      //if our node has no input edges
      if (nodes[x].edges.size() == 0) {
        gotao_push(heat_ao[k]);
      } else {
        //for each input edge of our node
        for (int y = 0; y < (nodes[x].edges).size(); y++) {
          //create edge from corresponding TAO class
          switch (nodes[((nodes[x]).edges[y])].ttype) {
            case matrix :
              matrix_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(heat_ao[k]);
              break;
            case sort_ :
              sort_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(heat_ao[k]);
              break;
            case heat :
              heat_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(heat_ao[k]);
              break;
          }
        }
      }
      k++;
    }

  }
      




      /*
   	for(int x=0; x<ROW_SIZE; x+=stepsize){
  		for(int y=0; y<COL_SIZE; y+=stepsize){
              //level1[i] = new TAOMergeDyn(array1 + total_assemblies*i*2048, tmp + total_assemblies*i*2048, total_assemblies*2048, 2);

           		ao[i] = new TAO_matrix(2, x, x+stepsize, y, y+stepsize, ROW_SIZE, a, b, c); 
  	  	 	gotao_push_init(ao[i], i % nthreads);
          i++;
  		}	
    }
   
   */
std::cout << "i = " <<  i << "\n";
std::cout << "matrix_count = " << matrix_count << "\n";
std::cout << "j = " <<  j << "\n";
std::cout << "sort_count = " << sort_count << "\n";
std::cout << "k = " <<  k << "\n";
std::cout << "heat_count = " << heat_count << "\n";

   // measure execution time
std::cout << "starting \n";
   std::chrono::time_point<std::chrono::system_clock> start, end;
   start = std::chrono::system_clock::now();

   goTAO_start();

   goTAO_fini();

   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";
   std::cout << "Assembly Throughput: " << (nodecount) / elapsed_seconds.count() << " A/sec\n";
   std::cout << "Assembly Cycle: " << elapsed_seconds.count() / (nodecount)  << " sec/A\n";



//Free memory
  for (int i = 0; i < m_ysize; ++i)
{
  delete matrix_input_a[i];
  delete matrix_output_c[i];
}
delete[] matrix_input_a;
delete[] matrix_output_c;


  for (int i = 0; i < s_ysize; ++i)
{
  delete sort_input_a[i];
  delete sort_output_c[i];
}
delete[] sort_input_a;
delete[] sort_output_c;


  for (int i = 0; i < h_ysize; ++i)
{
  delete heat_input_a[i];
  delete heat_output_c[i];
}
delete[] heat_input_a;
delete[] heat_output_c;


   return (0);
}




void fill_arrays(int **a, int **c, int ysize, int xsize)
{

     //Fills the matrices a and c with random integers
     for (int i = 0; i < ysize; ++i) { //each row
        for (int j = 0; j  < ysize; j++){ //each column
          a[i][j] = (rand() % 111);

          c[i][j] = (rand() % 111);
        }
     }
}


//Creates and returns a node
node new_node(Taotype type, int nodenr, int taonr){
  node n;
  n.ttype = type;
  n.nodenr = nodenr;
  n.taonr = taonr;
  return n;
}


//add edge to node
void add_edge(node &n, node const &e){
  (n.edges).push_back (e.nodenr);
}


//add memory slot for input and output data to a node
void add_space(node &n, std::vector<int> &v, int a){
  if (a == -1){
    v.push_back (n.nodenr);
    n.mem_space = (v.size() + 1);
  } else {
  n.mem_space = a;
  }
}


//find a memory slot for input and output data amongst predecessors
//nodenr is of the node we are trying find a memory slot for
//node n is a predecessor of our node, we are searching node n to find if node n was the last node to use a memory slot
int find_mem(Taotype type, int nodenr, node const &n, std::vector<int> &v){
  //a minnr to make sure not to search too far
  int minnr = nodenr;
    //for all entries in mem vector
    for (int i = 0; i < v.size(); i++) {
      //set minnr to the lowest entry in mem vector, we dont need to search nodes with lower nodenr than this
      minnr = min(minnr, v[i]);
      //if our the node we are searching matches one of the entries in the mem vector
      if (n.nodenr == v[i] && n.ttype == type) {
        //set the new value of that index to the nodenr of the node we are looking to find a slot for.
        v[i] = nodenr;
        //return the index of the mem vector, this corresponds to the memory slot we acquired
        return i;
      }
    }

  int x;
  //for all edges of node n
  for (int i = 0; i < (n.edges).size(); i++) {
    //if the nodenr of the edges is more than minnr(else there is no possibility to find a match)
    if (nodes[n.edges[i]].nodenr >= minnr) {
      //recursively call find_mem
      x = find_mem(type, nodenr, nodes[n.edges[i]], v);
      //if we found a memory slot
      if (x > -1) {
        //return the found slot
        return x;
      }
    
    }
  }
  //if nothing is found, return -1
  return -1;

}


//check if the node we are trying to add as edge is already a predessecor of our node n
bool edge_check(int edge, node const &n){
  //check if edge is less or equal to nodenr, if its not -> no point in continuing
  if (edge <= n.nodenr){
    //check if we have a match
    if (edge == n.nodenr){
      //return true , It is a predecessor (or the same)
      return true;
    }
    else{
      //for all edges of node n
      for (int i = 0; i < (n.edges).size(); i++) {
        //recursively call edge_check
        if (edge_check(edge, nodes[n.edges[i]])) {
          //return true if we found a match
          return true;
        }
      }
    }
  }
  //return false if we could not find the node at all.
  return false;
}


