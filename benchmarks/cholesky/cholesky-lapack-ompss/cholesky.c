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

#define TYPE double
#include "blas.h"
#include "blas_init.h"
#include "timer.h"
#include "init.h"

static const integer ONE = 1;
static const integer MINUS_ONE = -1;
static const integer ZERO = 0;

static const double dzero = 0.0;
static const double done = 1.0;
static const double dminusone = -1.0;


//
// Kernels
//


#pragma omp task inout([BS]A) priority(1)
void dpotrf_tile(integer BS, double A[BS][BS]) {
	integer INFO;
	
	lapack_dpotrf("L", &BS, (double *) A, &BS, &INFO);
}


#pragma omp task input([BS]A, [BS]B) inout([BS]C)
void dgemm_tile(integer BS, double const A[BS][BS], double const B[BS][BS], double C[BS][BS]) {
	lapack_dgemm("N", "T", &BS, &BS, &BS, &dminusone, (double const *) A, &BS, (double const *) B, &BS, &done, (double *) C, &BS);
}


#pragma omp task input([BS]T) inout([BS]B)
void dtrsm_tile(integer BS, double const T[BS][BS], double B[BS][BS]) {
	lapack_dtrsm("R", "L", "T", "N", &BS, &BS, &done, (double const *) T, &BS, (double *) B, &BS);
}


#pragma omp task input([BS]A) inout([BS]C)
void dsyrk_tile(integer BS, double const A[BS][BS], double C[BS][BS]) {
	lapack_dsyrk("L", "N", &BS, &BS, &dminusone, (double const *) A, &BS, &done, (double *) C, &BS);
}

void cholesky(integer N, integer BS, double A[N/BS][N/BS][BS][BS]) {
	for (integer j = 0; j < N/BS; j++) {
		for (integer k= 0; k< j; k++) {
			for (integer i = j+1; i < N/BS; i++) {
				// A[i,j] = A[i,j] - A[i,k] * (A[j,k])^t
				dgemm_tile(BS, A[k][i], A[k][j], A[j][i]);
			}
		}
		
		for (integer i = 0; i < j; i++) {
			// A[j,j] = A[j,j] - A[j,i] * (A[j,i])^t
			dsyrk_tile(BS, A[i][j], A[j][j]);
		}
		
		// Cholesky Factorization of A[j,j]
		dpotrf_tile(BS, A[j][j]);
		
		for (integer i = j+1; i < N/BS; i++) {
			// A[i,j] <- A[i,j] = X * (A[j,j])^t
			dtrsm_tile(BS, A[j][j], A[j][i]);
		}
	}
}
//--------------------------------------------------------------------------------
static integer N_;

static inline double initializeElement(long I, long J, long i, long j) {
	// Symetric and positive definite
	if ((I+i) == (J+j)) {
		return ((double) N_) + (I+J+i*3.0+j*5.0)/(10.0*N_);
	} else {
		return (I+J+i*3.0+j*5.0)/(10.0*N_);
	}
}

static void init(integer N, integer BS, double A[N/BS][N/BS][BS][BS]) {
	force_blas_init();
	initializeUsingUserSpecification(N, N, BS, BS, A);
}

static void print_matrix(integer N, integer BS, double A[N/BS][N/BS][BS][BS])
{
  int i=0, j=0, k=0, l=0;
  for(i = 0; i < N/BS; i++){
	printf("\n");
	for(j = 0; j < N/BS; j++){
	        printf("\n[%d,%d]\n",i,j);
		for(k = 0; k < BS; k++){
			for(l = 0; l < BS; l++)
				printf("[%d,%d]: %e, ", k,l,A[i][j][k][l]);
				printf("\n");
				} 
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 7) {
		fprintf(stderr, "Usage: %s <N> <N> <block size> <block size> <index> <B|PW|SEQ>\n", argv[0]);
		exit (1);
	}
	integer N = atol(argv[1]);
	integer BS = atol(argv[3]);
	parseInitializationType(argv[6]);
	N_ = N;
	double (*A)[N/BS][BS][BS] = (double (*)[N/BS][BS][BS]) malloc (sizeof(double) * N * N);
	init(N, BS, A);
	#pragma omp taskwait
	START_TIME();
	cholesky(N, BS, A);
	#pragma omp taskwait
	STOP_TIME();

	if(N <= 32)
	  print_matrix(N,BS,A);

	if (getenv("MEASURE_TIME") == NULL) {
		printf("%f", (0.33*N*N*N+0.5*N*N+0.17*N)/GET_TIME() );
	} else {
		printf("%f", GET_TIME() / 1000000.0);
	}
	return 0;
}
