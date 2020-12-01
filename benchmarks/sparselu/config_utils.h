#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H
#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include <vector>
#include <stdio.h>
#include <stdlib.h> 
#include <math.h>
#include <sys/time.h>
#include <time.h>
//#include "sparselu_taos.h"
#include "sparselu.h"                                                      
#include "xitao.h"                                                                  
#include <assert.h>
#include <set>
#include <fstream>      
#include <string>
#include <sstream>



extern "C" {
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
}
using namespace xitao;
using namespace std;

typedef double ELEM;

int NB = 10;
int BSIZE = 512;
int TAO_WIDTH = 1;
const float sta_precision = 100.0f;
size_t STA_IND = 0;
int NTHREADS = 0;
vector<vector<ELEM*>> A;
vector<vector<float>> sta;
vector<vector<AssemblyTask*>> dependency_matrix;
#ifndef NO_STA
vector<vector<bool>> init_matrix;
#endif
// Enable to output dot file. Recommended to use with NB < 16 
//#define OUTPUT_DOT
#define FALSE (0)
#define TRUE (1)


inline void init_dot_file(ofstream& file, const char* name) 
{
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
inline void write_edge(ofstream& file, vector<string> const src, int indxsrc, int indysrc, vector<string> const dst, int indxdst, int indydst) 
{
#ifdef OUTPUT_DOT  
  stringstream st;
  st << src[0] <<"_" << indxsrc << "_" << indysrc << "";
  file << st.str() << " -> ";
  st.str("");
  st << dst[0] <<"_" << indxdst << "_" << indydst << ";" << endl;
  file << st.str();
//  file << src[0] <<"_" << indxsrc << "_" << indysrc << "[label=\"" << src[1] << "[" << indxsrc << "," << indxdst << "]\"" << ",color="<< src[2] << ",style=bold,fontstyle=bold]" << std::endl;
 // file << dst[0] <<"_" << indxdst << "_" << indydst << "[label=\"" << dst[1] << "[" << indxdst << "," << indydst << "]\"" << ",color="<< dst[2] << ",style=bold,fontstyle=bold]" << std::endl;
  file << src[0] <<"_" << indxsrc << "_" << indysrc << "[label=\"" << "[" << indxsrc << "," << indysrc << "]\"" << ",fillcolor="<< src[2] << ",style=filled]" << std::endl;
  file << dst[0] <<"_" << indxdst << "_" << indydst << "[label=\"" << "[" << indxdst << "," << indydst << "]\"" << ",fillcolor="<< dst[2] << ",style=filled]" << std::endl;

#endif
}
#if 1
float get_sta_val(int ii, int jj) {
  if(sta[ii][jj] < 0) sta[ii][jj] = ((STA_IND++) % (NTHREADS+1))/(float)NTHREADS;
  return sta[ii][jj];
} 
#elif 0
float get_sta_val(int ii, int jj) { 
  float sta_val = (float) (ii * NB + jj) / (float) (NB * NB);
  return trunc(sta_val * sta_precision) / sta_precision;
}
#else
float get_sta_val(int ii, int jj) { 
  float sta_val = (float) (ii) / NB;
  return trunc(sta_val * sta_precision) / sta_precision;
}
#endif
#if 0
int get_sta_int_val(int ii, int jj) { 
  return int(get_sta_val(ii, jj) * sta_precision);  
}  
#else
int get_sta_int_val(int ii, int jj) { 
  return sta[ii][jj] * NTHREADS;
}
#endif
int is_null_entry(int ii, int jj) {
  int null_entry = FALSE;
  if ((ii<jj) && (ii%3 !=0)) null_entry =TRUE;
  if ((ii>jj) && (jj%3 !=0)) null_entry =TRUE;
  if (ii%2==1) null_entry=TRUE;
  if (jj%2==1) null_entry=TRUE;
  if (ii==jj) null_entry=FALSE;
  return null_entry;
}

int count_matrix_blocks() { 
  int bcount = 0; 
  for(int i = 0; i < NB; ++i) 
    for(int j = 0; j < NB; ++j) 
      if(is_null_entry(i, j) == FALSE) 
        bcount++;
  return bcount;  
} 

void generate_matrix_structure (int& bcount)
{
   int null_entry;
   for (int ii=0; ii<NB; ii++)
      for (int jj=0; jj<NB; jj++){
         null_entry= is_null_entry(ii, jj);
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
           for (i = 0; i < BSIZE; i++) { 
              for (j = 0; j < BSIZE; j++) {
	           init_val = (3125 * init_val) % 65536;
      	           (*p) = (ELEM)((init_val - 32768.0) / 16384.0);
                   p++;
              }
              init_val = 1325;
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


#endif
