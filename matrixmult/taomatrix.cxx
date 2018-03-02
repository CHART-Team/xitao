//
//
//

#include "tao.h"
#include "taomatrix.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include "config-matrix.h"


extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}




/*
// Matrix multiplication TAO
class TAO_matrix : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAO_matrix(int res, int mini, int maxi, int minj, int maxj, int steps,  int **m_a, int **m_b, int **m_c) 
                        : _res(res), imax(maxi), jmin(minj), jmax(maxj), AssemblyTask(res) 
                {   

                  a = m_a;
                  b = m_b;
                  c = m_c;
                  i = mini;
                
                }

                int cleanup(){ 
                    }

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {

                    int temp_j, temp_i;
                      while (1){ 
                        //mutex for i to make each Processing unit work on one row.
                        LOCK_ACQUIRE(i_lock); //Maybe Test-and-set would be better
                        temp_i = i++;
                        LOCK_RELEASE(i_lock)
                        
                        if (temp_i >= imax) { // no more work to be done
                          break;
                        }
                        
                        //for each column, calculate the output of c[i][j]
                        for(temp_j = jmin; temp_j < jmax; temp_j++){ 
                          int temp_output = 0;
                          for (int k = 0; k < ROW_SIZE ; k++){ 
                            temp_output += (a[temp_i][k] * b[k][temp_j]);
                          }
                          c[temp_i][temp_j] = temp_output;
                        }
                
                    
                  }
                }
              
              GENERIC_LOCK(i_lock);
              
              //Variable declaration
              int i, jmin, jmax, imax, _res;
              int **a;
              int **b;
              int **c;
};
*/

void fill_arrays(int **a, int **b, int **c);


// MAIN 
int
main(int argc, char* argv[])
{

   int thread_base; int nthreads; int nctx; //Do we need the last parameter?
   int c_size; int r_size; int stepsize;
   int nqueues;  // how many queues to fill: What is this??
   int nas;      // number of assemblies per queue: what is this??

 std::cout << "lol" << std::endl;
  
  int **a = (int**) malloc(sizeof *a * ROW_SIZE);//[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
  int **b = (int**) malloc(sizeof *a * ROW_SIZE);//[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
  int **c = (int**) malloc(sizeof *a * ROW_SIZE);//[4][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
  

  if (a && b && c){
    for (int i = 0; i < ROW_SIZE; i++){
      a[i] = (int*) malloc(sizeof *a[i] * COL_SIZE);
      b[i] = (int*) malloc(sizeof *a[i] * COL_SIZE);
      c[i] = (int*) malloc(sizeof *a[i] * COL_SIZE); 
    }
  }

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

   if(getenv("COL_SIZE"))
	c_size=atoi(getenv("COL_SIZE"));
   else
	c_size=COL_SIZE;

   if(getenv("STEP_SIZE"))
	stepsize=atoi(getenv("STEP_SIZE"));
   else
	stepsize=STEP_SIZE;

   if((COL_SIZE != ROW_SIZE) || (0!=(COL_SIZE % stepsize))){
	std::cout << "Incompatible matrix parameters, please choose ROW_SIZE=COL_SIZE and a stepsize that is a divisior of the ROW_SIZE&&COL_SIZE";
	return 0;
   }


    std::cout << "gotao_init parameters are: " << nthreads <<"," << thread_base << ","<< nctx << std::endl;

   gotao_init();


   std::chrono::time_point<std::chrono::system_clock> start, end;
   start = std::chrono::system_clock::now();


   // fill the input arrays and empty the output array
   fill_arrays(a, b , c);

   //Spawn TAOs for every seperately written too value, equivilent to for(i->i+stepsize){for(j->j+stepsize) generation}
   int total_assemblies = (c_size/stepsize)*(r_size/stepsize);
   TAO_matrix *ao[total_assemblies];

   //Spawn a number of TAOs depending on matrix size and step size
   int i = 0;
   	for(int x=0; x<ROW_SIZE; x+=stepsize){
  		for(int y=0; y<COL_SIZE; y+=stepsize){
           		ao[i] = new TAO_matrix(1, x, x+stepsize, y, y+stepsize, stepsize, a, b, c); 
  	  	 	gotao_push_init(ao[i], i % nthreads);
          i++;
  		}	
    }
   
   
   


   // measure execution time

   goTAO_start();

   goTAO_fini();
   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";
   std::cout << "Assembly Throughput: " << total_assemblies / elapsed_seconds.count() << " A/sec\n";
   std::cout << "Assembly Cycle: " << elapsed_seconds.count() / total_assemblies  << " sec/A\n";


  std::cout << c[0][0]<< " " << c[0][1] << " " << c[0][2] << " " << c[0][3] << "\n";
  std::cout << c[1][0] << c[1][1] << c[1][2] << c[1][3] << "\n";
  std::cout << c[2][0] << c[2][1] << c[2][2] << c[2][3] << "\n";
  std::cout << c[3][0] << c[3][1] << c[3][2] << c[3][3] << "\n";
   return (0);
}

void fill_arrays(int **a, int **b, int **c)
{

     //Fills the input matrices a and b with random integers and output matrix with 0
     for (int i = 0; i < ROW_SIZE; ++i) { //each row
        for (int j = 0; j  < COL_SIZE; j++){ //each column
          a[i][j] = (rand() % 111);
          b[i][j] = (rand() % 111);
          c[i][j] = 0;
        }
     }
}

