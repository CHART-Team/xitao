#include <stdio.h>
#include <stdlib.h> 
#include <math.h>

#include <sys/time.h>
#include <time.h>
#include "sparselu.h"

ELEM *A[NB][NB];

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
            A[ii][jj] = (ELEM *)malloc(BSIZE*BSIZE*sizeof(ELEM));
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

void  print_structure()
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

ELEM *allocate_clean_block()
{
  int i,j;
  ELEM *p, *q;

  p=(ELEM*)malloc(BSIZE*BSIZE*sizeof(ELEM));
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

/* ************************************************************ */
/* main 							*/
/* ************************************************************ */

int main(int argc, char* argv[])
{
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

      lu0(A[kk][kk]);

      for (jj=kk+1; jj<NB; jj++)
         if (A[kk][jj] != NULL)
            fwd(A[kk][kk], A[kk][jj]);

      for (ii=kk+1; ii<NB; ii++) 
         if (A[ii][kk] != NULL)
            bdiv(A[kk][kk], A[ii][kk]);

      for (ii=kk+1; ii<NB; ii++) {
         if (A[ii][kk] != NULL) {
            for (jj=kk+1; jj<NB; jj++) {
               if (A[kk][jj] != NULL) {
                  if (A[ii][jj]==NULL)
                  {
                     A[ii][jj]=allocate_clean_block();
                  }

                  bmod(A[ii][kk], A[kk][jj], A[ii][jj]);
               }
            }
         }
      }
   }

   //Timing Stop
   t_end=get_time();

   time = t_end-t_start;
   printf("Matrix is: %d (%d %d) memory is %ld MB time to compute = %11.4f sec\n", 
         (NB*BSIZE), NB, BSIZE, (NB*BSIZE)*(NB*BSIZE)*sizeof(ELEM)/1024/1024, time);
//   print_structure();
}

