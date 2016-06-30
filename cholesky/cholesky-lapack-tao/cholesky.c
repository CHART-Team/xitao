// This file will provide just the bindings, so that we can use 
// the VLAs defined in the original code from the C++ TAO code

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


extern integer N_;

static inline double initializeElement(long I, long J, long i, long j) {
	// Symetric and positive definite
	if ((I+i) == (J+j)) {
		return ((double) N_) + (I+J+i*3.0+j*5.0)/(10.0*N_);
	} else {
		return (I+J+i*3.0+j*5.0)/(10.0*N_);
	}
}

//
// Kernels and generic wrappers
//

void dpotrf_tile(integer BS, double A[BS][BS]) {
	integer INFO;
	
	lapack_dpotrf("L", &BS, (double *) A, &BS, &INFO);
}

void dpotrf_tile_g(integer BS, void *A)
{
	dpotrf_tile(BS, A);
}


void dgemm_tile(integer BS, double const A[BS][BS], double const B[BS][BS], double C[BS][BS]) {
	lapack_dgemm("N", "T", &BS, &BS, &BS, &dminusone, (double const *) A, &BS, (double const *) B, &BS, &done, (double *) C, &BS);
}

void dgemm_tile_g(integer BS, void *A, void *B, void *C)
{
	dgemm_tile(BS, A, B, C);
}


void dtrsm_tile(integer BS, double const T[BS][BS], double B[BS][BS]) {
	lapack_dtrsm("R", "L", "T", "N", &BS, &BS, &done, (double const *) T, &BS, (double *) B, &BS);
}

void dtrsm_tile_g(integer BS, void *T, void *B)
{
	dtrsm_tile(BS, T, B);
}


void dsyrk_tile(integer BS, double const A[BS][BS], double C[BS][BS]) {
	lapack_dsyrk("L", "N", &BS, &BS, &dminusone, (double const *) A, &BS, &done, (double *) C, &BS);
}

void dsyrk_tile_g(integer BS, void *A, void *C)
{
	dsyrk_tile(BS, A, C);
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

void print_matrix_g(integer N, integer BS, double *A)
{
	print_matrix(N,BS,A);
}
