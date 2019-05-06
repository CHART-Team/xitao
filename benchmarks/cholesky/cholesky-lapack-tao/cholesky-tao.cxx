#ifndef integer
#error This benchmark requires integer to be defined to the equivalent Fortran integer type
#endif

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define TYPE double;

extern "C" { 
#include "timer.h"
#include "blas.h"
#include "blas_init.h"
} 

// interfaces to the original functions
extern "C" {
void dpotrf_tile_g(integer, void *);
void dgemm_tile_g(integer, void *, void *, void *);
void dtrsm_tile_g(integer, void *, void *);
void dsyrk_tile_g(integer, void *, void *);
void print_matrix_g(integer N, integer BS, double *A);
}

#include "tao.h"

static const integer ONE = 1;
static const integer MINUS_ONE = -1;
static const integer ZERO = 0;

static const double dzero = 0.0;
static const double done = 1.0;
static const double dminusone = -1.0;



//
// Kernels
//


class TAO_dgemm_part : public AssemblyTask
{
// compute ::
//		#pragma omp parallel for
//		for (integer i = j+1; i < N/BS; i++) {
//			#pragma omp task untied
//			for (integer k= 0; k< j; k++) {
//				// A[i,j] = A[i,j] - A[i,k] * (A[j,k])^t
//				dgemm_tile(BS, A[k][i], A[k][j], A[j][i]);
//			}
//		}
public: 
	TAO_dgemm_part(double *_A, integer _N, integer _BS, integer _j, 
		       int _width, int nthread=0)
		: AssemblyTask(_width, nthread)
		{
			A = _A;
			j = _j;
			N = _N;
			BS = _BS;
			width = _width;

			steps = (N/BS + width - 1)/ width;
		}

         int cleanup(){ 
                    //delete dow;
                    //delete array;
                    }

	int j, N, BS;
	double *A;
	int steps; 
	int width;

         int execute(int threadid)
         {
#define ndx(A,i,j) (A + i*BS*N + j*BS*BS)
                int tid = threadid - leader;
                for(int i = tid; i > N/BS; i+=width){
//		  int i = j + 1 + tid*steps + s;
		  if(i < j+1) continue;
		  for (integer k= 0; k< j; k++) {
//			// A[i,j] = A[i,j] - A[i,k] * (A[j,k])^t
			dgemm_tile_g(BS, ndx(A,k,i), ndx(A,k,j), ndx(A,j,i));
			}
		}
         }
};

// this assembly is to be executed by a single thread
class TAO_syrk_potrf_part : public AssemblyTask
{
// compute ::
//		#pragma omp task untied
//		for (integer i = 0; i < j; i++) {
//			// A[j,j] = A[j,j] - A[j,i] * (A[j,i])^t
//			dsyrk_tile(BS, A[i][j], A[j][j]);
//		}
//		
//		#pragma omp taskwait
//		
//		// Cholesky Factorization of A[j,j]
//		#pragma omp task untied
// 		dpotrf_tile(BS, A[j][j]);
public: 
	TAO_syrk_potrf_part (double *_A, integer _N, integer _BS, integer _j, 
		             int width=1, int nthread=0) 
		: AssemblyTask(width, nthread)
		{
			A = _A;
			j = _j;
			N = _N;
			BS = _BS;
		}

         int cleanup(){ 
                    }

	int j, N, BS;
	double *A;
	int steps;

         int execute(int threadid)
         {
		// perform j-1 syrk() + 1x potrf()
		int i;
                int tid = threadid - leader;
		if (tid) return 0; // only the first thread executes all these calls
                for(int i = 0; i < j; i++)
	          dsyrk_tile_g(BS, ndx(A,i,j), ndx(A,j,j));
 		dpotrf_tile_g(BS, ndx(A,j,j));
         }
};

class TAO_trsm_part : public AssemblyTask
{
// compute ::
//		#pragma omp parallel for
//		for (integer i = j+1; i < N/BS; i++) {
//			// A[i,j] <- A[i,j] = X * (A[j,j])^t
//			#pragma omp task untied
//			dtrsm_tile(BS, A[j][j], A[j][i]);
//		}
public: 
	TAO_trsm_part(double *_A, integer _N, integer _BS, integer _j, 
		       int    _width, int nthread=0)
		: AssemblyTask(_width, nthread)
		{
			A = _A;
			j = _j;
			N = _N;
			BS = _BS;
			width = _width;

			steps = (N/BS + width - 1)/ width;
		}

         int cleanup(){ 
                    //delete dow;
                    //delete array;
                    }

	int j, N, BS, start;
	double *A;
	int steps;
	int width;

         int execute(int threadid)
         {
                int tid = threadid - leader;
                for(int i = tid; i < N/BS; i += width){
		  if(i < j+1) continue;  // check if out of bounds
		  dtrsm_tile_g(BS, ndx(A,j,j), ndx(A,j,i));
		}
         }
};


static int dgemm_width; 
static int trsm_width;

// construction of the TAO DAG
void cholesky(integer N, integer BS, double *A) {
	TAO_dgemm_part *dgemm_p = nullptr;
	TAO_syrk_potrf_part *sp_p = nullptr;
	TAO_trsm_part *trsm_p = nullptr;

	for (integer j = 0; j < N/BS; j++) {
		dgemm_p = new TAO_dgemm_part(A, N, BS, j, dgemm_width);
		if(trsm_p) trsm_p->make_edge(dgemm_p);
		else gotao_push(dgemm_p);
		
		sp_p = new TAO_syrk_potrf_part(A, N, BS, j); // with = 1
		dgemm_p->make_edge(sp_p);

		trsm_p = new TAO_trsm_part(A, N, BS, j, trsm_width);
		sp_p->make_edge(trsm_p);
	}
}

integer N_;

extern "C" { 

void initializeUsingUserSpecification_g(long VN, long HN, long VBS, long HBS, void *data);

static void init(integer N, integer BS, void * A) {
	force_blas_init();
	initializeUsingUserSpecification_g(N, N, BS, BS, A);

}

}

extern "C" 
{ 
void parseInitializationType(char const *initializazionTypeString);
} 

int main(int argc, char *argv[]) {
	// TAO variables
        int thread_base; 
	int nthreads;
 
	if (argc != 7) {
		fprintf(stderr, "Usage: %s <N> <N> <block size> <block size> <index> <B|PW|SEQ>\n", argv[0]);
		exit (1);
	}

	integer N = atol(argv[1]);
	integer BS = atol(argv[3]);

        if(getenv("GOTAO_NTHREADS"))
             nthreads = atoi(getenv("GOTAO_NTHREADS"));
        else 
             nthreads = GOTAO_NTHREADS;
     
        if(getenv("GOTAO_THREAD_BASE"))
             thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
        else
             thread_base = GOTAO_THREAD_BASE;

        if(getenv("DGEMM_WIDTH"))
             dgemm_width = atoi(getenv("DGEMM_WIDTH"));
        else
             dgemm_width = DGEMM_WIDTH;

        if(getenv("TRSM_WIDTH"))
             trsm_width = atoi(getenv("TRSM_WIDTH"));
        else
             trsm_width = TRSM_WIDTH;

	std::cout << "CHOLESKY TAO Configuration parameters " << std::endl;
	std::cout << "nthreads = "     << nthreads            << std::endl;
	std::cout << "thread_base = "  << thread_base         << std::endl;
	std::cout << "dgemm_width = "  << dgemm_width         << std::endl;
	std::cout << "trsm_width = "   << trsm_width          << std::endl;

	parseInitializationType(argv[6]);

	// initialize TAO environment
        gotao_init();

	N_ = N;
//	double (*A)[N/BS][BS][BS] = (double (*)[N/BS][BS][BS]) malloc (sizeof(double) * N * N);
	double *A = (double *) malloc (sizeof(double) * N * N);

	init(N, BS, A);
	cholesky(N, BS, A); // creates the dependency graph

	if(N<=32)
		print_matrix_g(N,BS,A);

	START_TIME();
	gotao_start();	 // execute the application
	gotao_fini();    // eliminate the runtime
	STOP_TIME();

	if(N<=32)
		print_matrix_g(N,BS,A);

	// make sure that result is correct
	if (getenv("MEASURE_TIME") == NULL) {
		printf("%f", (0.33*N*N*N+0.5*N*N+0.17*N)/GET_TIME() );
	} else {
		printf("%f", GET_TIME() / 1000000.0);
	}
	return 0;
}
