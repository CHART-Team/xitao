#include <stdio.h>
#include <stdlib.h> 
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include "sparselu_taos.h"  
#include "sparselu.h"                                                      
#include "xitao.h"                                                                  
#include <assert.h>
#include <set>
#include <fstream>      
#include <string>
#include <sstream>

int NB = 10;
int BSIZE = 512;

typedef double ELEM;
vector<vector<ELEM*>> A;
vector<vector<AssemblyTask*>> scoreboard;

// Enable to output dot file. Recommended to use with NB < 16 
//#define OUTPUT_DOT
#define FALSE (0)
#define TRUE (1)
#define TAO_WIDTH 1
using namespace xitao;
using namespace std;

string get_tao_name(AssemblyTask* task) {
  stringstream st; 
  stringstream st_address; 
  string address;
  BDIV* bdiv = dynamic_cast<BDIV*>(task);
  BMOD* bmod = dynamic_cast<BMOD*>(task);
  LU0* lu0   = dynamic_cast<LU0*>(task);
  FWD* fwd   = dynamic_cast<FWD*>(task);
  const int digits = 3;
  if(bdiv) {
    st_address << bdiv;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "bdiv_"<< address;
  } else if (bmod){
    st_address << bmod;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "bmod_"<< address;
  } else if (lu0) {
    st_address << lu0;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "lu0_"<< address;
  } else if (fwd) {
    st_address << fwd;
    address = st_address.str();
    address = address.substr(address.size() - digits, address.size());
    st << "fwd_"<< address;
  } else {
    std::cout << "error: unknown TAO type" << std::endl;
    exit(0);
  }  
  return st.str();                  
}

inline void init_dot_file(ofstream& file, const char* name) {
#ifdef OUTPUT_DOT
  file.open(name);
  file << "digraph G {" << endl;
#endif
}

inline void close_dot_file(ofstream& file) {
#ifdef OUTPUT_DOT  
  file << "}" << endl;
  file.close();
#endif
}

inline void write_edge(ofstream& file, string const src, int indxsrc, int indysrc, string const dst, int indxdst, int indydst) {
#ifdef OUTPUT_DOT  
  stringstream st;
  st << src <<"_" << indxsrc << "_" << indysrc;
  file << st.str() << " -> ";
  st.str("");
  st << dst <<"_" << indxdst << "_" << indydst << ";" << endl;
  file << st.str();
#endif
}

void generate_matrix_structure (int& bcount)
{
   int null_entry;
   for (int ii=0; ii<NB; ii++)
      for (int jj=0; jj<NB; jj++){
         null_entry=FALSE;
         if ((ii<jj) && (ii%3 !=0)) null_entry =TRUE;
         if ((ii>jj) && (jj%3 !=0)) null_entry =TRUE;
         if (ii%2==1) null_entry=TRUE;
         if (jj%2==1) null_entry=TRUE;
         if (ii==jj) null_entry=FALSE;
         if (null_entry==FALSE){
            bcount++;
            //A[ii][jj] = (ELEM *) new (BSIZE*BSIZE*sizeof(ELEM));
            A[ii][jj] = (ELEM *) new ELEM[BSIZE*BSIZE];
            if (A[ii][jj]==NULL) {
               printf("Out of memory\n");
               exit(1);
            }
         }
         else A[ii][jj] = NULL;
      }
}


void generate_matrix_values ()
{
   int init_val, i, j, ii, jj;
   ELEM *p;

   init_val = 1325;

   for (ii = 0; ii < NB; ii++) 
     for (jj = 0; jj < NB; jj++)
     {
        p = A[ii][jj];
        if (p!=NULL)
           for (i = 0; i < BSIZE; i++) 
              for (j = 0; j < BSIZE; j++) {
	           init_val = (3125 * init_val) % 65536;
      	           (*p) = (ELEM)((init_val - 32768.0) / 16384.0);
                   p++;
              }
     }
}

void print_structure()
{
   ELEM *p;
   int sum = 0; 
   int ii, jj, i, j;

   printf ("Structure for matrix A\n");

   for (ii = 0; ii < NB; ii++) {
     for (jj = 0; jj < NB; jj++) {
        p = A[ii][jj];
        if (p!=NULL)
        {
           {
              for(i =0; i < BSIZE; i++)
              {
                 for(j =0; j < BSIZE; j++)
                 {
                    printf("%+lg \n", p[i * BSIZE + j]);
                 }
                 printf("\n");
              }
           }
        }
     }
   }
}

vector<ELEM> read_structure()
{
   ELEM *p;
   int sum = 0; 
   int ii, jj, i, j;
   vector<ELEM> results; 
   for (ii = 0; ii < NB; ii++) {
     for (jj = 0; jj < NB; jj++) {
        p = A[ii][jj];
        if (p!=NULL)
        {
           {
              for(i =0; i < BSIZE; i++)
              {
                 for(j =0; j < BSIZE; j++)
                 {
                    results.push_back(p[i * BSIZE + j]);
                 }
              }
           }
        }
     }
   }
   return results;
}

ELEM *allocate_clean_block()
{
  int i,j;
  ELEM *p, *q;

  //p=(ELEM*)malloc(BSIZE*BSIZE*sizeof(ELEM));
  p= new ELEM[BSIZE*BSIZE];
  q=p;
  if (p!=NULL){
     for (i = 0; i < BSIZE; i++) 
        for (j = 0; j < BSIZE; j++){(*p)=(ELEM)0.0; p++;}
	
  }
  else printf ("OUT OF MEMORY!!!!!!!!!!!!!!!\n");
  return (q);
}


/* ************************************************************ */
/* Utility routine to measure time                              */
/* ************************************************************ */

double myusecond()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  return ((double) tv.tv_sec *4000000) + tv.tv_usec;
}

double mysecond()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  return ((double) tv.tv_sec) + ((double)tv.tv_usec*1e-6);
}

double gtod_ref_time_sec = 0.0;

float get_time()
{
    double t, norm_sec;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    // If this is the first invocation of through dclock(), then initialize the
    // "reference time" global variable to the seconds field of the tv struct.
    if (gtod_ref_time_sec == 0.0)
        gtod_ref_time_sec = (double) tv.tv_sec;

    // Normalize the seconds field of the tv struct so that it is relative to the
    // "reference time" that was recorded during the first invocation of dclock().
    norm_sec = (double) tv.tv_sec - gtod_ref_time_sec;

    // Compute the number of seconds since the reference time.
    t = norm_sec + tv.tv_usec * 1.0e-6;

    return (float) t;
}


void sparselu_parallel()
{
   float t_start,t_end;
   float time;
   int ii, jj, kk;
   int bcount = 0;

   ofstream file;
   init_dot_file(file, "sparselu.dot");

   generate_matrix_structure(bcount);
   generate_matrix_values();
   printf("Init OK Matrix is: %d (%d %d) # of blocks: %d memory is %ld MB\n", (NB*BSIZE), NB, BSIZE, bcount, bcount*sizeof(ELEM)/1024/1024);
   // init XiTAO runtime 
   gotao_init_hw(4, -1, -1);
   int prev_diag = -1;
   //Timing Start
   t_start=get_time();
   for (kk=0; kk<NB; kk++) {

      auto lu0 = new LU0(A[kk][kk], BSIZE, TAO_WIDTH); 
      //cout << "Creating LU0: " << kk << " " << kk << endl;
      //lu0_objs.push_back(new LU0(A[kk][kk], BSIZE, TAO_WIDTH));

      //check if the task will get its input this from a previous task
      if(scoreboard[kk][kk]) {
        write_edge(file, get_tao_name(scoreboard[kk][kk]), kk, kk, get_tao_name(lu0), kk, kk);
        scoreboard[kk][kk]->make_edge(lu0);
      } else {
         gotao_push(lu0);
      } 
      scoreboard[kk][kk] = lu0;

      for (jj=kk+1; jj<NB; jj++)
         if (A[kk][jj] != NULL) {
            //fwd(A[kk][kk], A[kk][jj]);
            auto fwd =  new FWD(A[kk][kk], A[kk][jj], BSIZE, TAO_WIDTH);
            if(scoreboard[kk][kk]) {
              write_edge(file, get_tao_name(scoreboard[kk][kk]), kk, kk, get_tao_name(fwd), kk, jj);
              scoreboard[kk][kk]->make_edge(fwd);
            }

            if(scoreboard[kk][jj]) {
              write_edge(file, get_tao_name(scoreboard[kk][jj]), kk, jj, get_tao_name(fwd), kk, jj);
              scoreboard[kk][jj]->make_edge(fwd);
            }
           
           scoreboard[kk][jj] = fwd;
         }

      for (ii=kk+1; ii<NB; ii++) 
         if (A[ii][kk] != NULL) {
            //bdiv (A[kk][kk], A[ii][kk]);
            auto bdiv = new BDIV(A[kk][kk], A[ii][kk], BSIZE, TAO_WIDTH);

            if(scoreboard[kk][kk]) {
              write_edge(file, get_tao_name(scoreboard[kk][kk]), kk, kk, get_tao_name(bdiv), ii, kk);
              scoreboard[kk][kk]->make_edge(bdiv);
            }

            if(scoreboard[ii][kk]) {
              write_edge(file, get_tao_name(scoreboard[ii][kk]), ii, kk, get_tao_name(bdiv), ii, kk);
              scoreboard[ii][kk]->make_edge(bdiv);
            }

            scoreboard[ii][kk] = bdiv;
         }

      for (ii=kk+1; ii<NB; ii++) {
         if (A[ii][kk] != NULL) {
            for (jj=kk+1; jj<NB; jj++) {
               if (A[kk][jj] != NULL) {
                  if (A[ii][jj]==NULL)
                  {
                     A[ii][jj]=allocate_clean_block();
                  }

                  //bmod(A[ii][kk], A[kk][jj], A[ii][jj]);
                  auto bmod = new BMOD(A[ii][kk], A[kk][jj], A[ii][jj], BSIZE, TAO_WIDTH);

                  if(scoreboard[ii][kk]) {
                      write_edge(file, get_tao_name(scoreboard[ii][kk]), ii, kk, get_tao_name(bmod), ii, jj);
                      scoreboard[ii][kk]->make_edge(bmod);
                  }

                  if(scoreboard[kk][jj]) {
                      write_edge(file, get_tao_name(scoreboard[kk][jj]), kk, jj, get_tao_name(bmod), ii, jj);
                      scoreboard[kk][jj]->make_edge(bmod);
                  }

                  if(scoreboard[ii][jj]) {
                    write_edge(file, get_tao_name(scoreboard[ii][jj]), ii, jj, get_tao_name(bmod), ii, jj);
                    scoreboard[ii][jj]->make_edge(bmod);
                  }

                  scoreboard[ii][jj] = bmod;
               }
            }
         }
      }
   }
   //Timing Stop
   t_end=get_time();
   time = t_end-t_start;
   printf("Building DAG: %11.4f sec\n", time);
   close_dot_file(file);
   //Timing Start
   t_start=get_time();

   //gotao_push(lu0_objs[0]);
   //Start the TAODAG exeuction
   gotao_start();
   //Finalize and claim resources back
   gotao_fini();

   //Timing Stop
   t_end=get_time();

   time = t_end-t_start;
   printf("Matrix is: %d (%d %d) memory is %ld MB time to compute in parallel: %11.4f sec\n", 
         (NB*BSIZE), NB, BSIZE, (NB*BSIZE)*(NB*BSIZE)*sizeof(ELEM)/1024/1024, time);
   // print_structure();
}

void sparselu_serial() {
  float t_start,t_end;
  float time;
  int ii, jj, kk;
  int bcount = 0;
  generate_matrix_structure(bcount);
  generate_matrix_values();
  //print_structure();
  printf("Init OK Matrix is: %d (%d %d) # of blocks: %d memory is %ld MB\n", (NB*BSIZE), NB, BSIZE, bcount, bcount*sizeof(ELEM)/1024/1024);

  //Timing Start
  t_start=get_time();

  for (kk=0; kk<NB; kk++) {

     lu0(A[kk][kk], BSIZE);

     for (jj=kk+1; jj<NB; jj++)
        if (A[kk][jj] != NULL)
           fwd(A[kk][kk], A[kk][jj], BSIZE);

     for (ii=kk+1; ii<NB; ii++) 
        if (A[ii][kk] != NULL)
           bdiv(A[kk][kk], A[ii][kk], BSIZE);

     for (ii=kk+1; ii<NB; ii++) {
        if (A[ii][kk] != NULL) {
           for (jj=kk+1; jj<NB; jj++) {
              if (A[kk][jj] != NULL) {
                 if (A[ii][jj]==NULL)
                 {
                    A[ii][jj]=allocate_clean_block();
                 }

                 bmod(A[ii][kk], A[kk][jj], A[ii][jj], BSIZE);
              }
           }
        }
     }
  }

  //Timing Stop
  t_end=get_time();

  time = t_end-t_start;
  printf("Matrix is: %d (%d %d) memory is %ld MB time to compute in serial: %11.4f sec\n", 
        (NB*BSIZE), NB, BSIZE, (NB*BSIZE)*(NB*BSIZE)*sizeof(ELEM)/1024/1024, time);

}

/* ************************************************************ */
/* main               */
/* ************************************************************ */

int main(int argc, char** argv) {
  if(argc < 3) {
    printf("Usage: ./sparselu BLOCKS BLOCKSIZE\n");
    exit(0);
  }
  NB = atoi(argv[1]);
  BSIZE = atoi(argv[2]);
  A.resize(NB);
  scoreboard.resize(NB);
  for(int i = 0; i < NB; ++i) {
    A[i].resize(NB, NULL);
    scoreboard[i].resize(NB, NULL);
  }
  sparselu_parallel();
  auto res_parallel = read_structure();
  sparselu_serial();
  auto res_serial = read_structure();
  ELEM diff = 0.0;
  ELEM norm = 0.0;
  if(res_parallel.size() != res_serial.size()) {
    printf("Result vectors are not equally sized\n");
  }
  else {
    for(int i = 0; i < res_parallel.size(); i++){
      if(isnan(res_serial[i]) || isnan(res_parallel[i])) continue;
      if(!isfinite(res_serial[i]) || !isfinite(res_parallel[i])) continue;
      diff += (res_serial[i] - res_parallel[i]) * (res_serial[i] - res_parallel[i]);
      norm += res_serial[i] * res_parallel[i];
    }
    if(isnan(sqrt(diff/norm)) || !isfinite(sqrt(diff/norm))) printf("Error is too high\n");
    else printf("%-20s : %8.5e\n","Rel. L2 Error", sqrt(diff/norm));
  }
}

