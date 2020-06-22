// Most of the code is extracted and adapted from the rodinia benchmark for dense LU //
// TODO: all "pragma omp simd" forloops can be adapted to make the task moldable.

#include <iostream>
#include <math.h>       /* exp */
#include <cstring>  // memcpy
#include "tao.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
}


#define BS 32 // the block size (BSxBS)

#define AA(_i,_j) a[offset*size+_i*size+_j+offset]
#define BB(_i,_j) a[_i*size+_j]
#define MIN(i,j) ((i)<(j) ? (i) : (j))
#define DM(_i,_j) dep_matrix[_i*size+_j]


using namespace std;

// Compute LU for a diagonal block
class lud_diagonal : public AssemblyTask 
{
public:       
    lud_diagonal(float* _a, int _size, int _offset) : a(_a), size(_size), offset(_offset), AssemblyTask(1)
    {
    }

    int size;
    int offset;
    float *a;
    void cleanup()
    {      
    }

    void execute(int threadid)
    {
	int tid = threadid - leader; 
	if(tid == 0)
	{
	    int i, j, k;
	    for (i = 0; i < BS; i++)
	    {	    
		for (j = i; j < BS; j++)
		{
		    for (k = 0; k < i ; k++)
		    {
			AA(i,j) = AA(i,j) - AA(i,k) * AA(k,j);
		    }
		}
		
		float temp = 1.f/AA(i,i);
		for (j = i+1; j < BS; j++)
		{
		    for (k = 0; k < i ; k++)
		    {
			AA(j,i) = AA(j,i) - AA(j,k) * AA(k,i);
		    }
		    AA(j,i) = AA(j,i)*temp;
		}
	    }	
	}
    }
};

// Update U blocks for the current raw block
class top_perimeter : public AssemblyTask 
{
public:       
top_perimeter(float* _a, int _size, int _offset, int _chunk_idx, int _chunks_per_inter, int _chunks_in_inter_row) :
    a( _a), size(_size),  offset(_offset),  chunk_idx(_chunk_idx), chunks_per_inter(_chunks_per_inter),
    chunks_in_inter_row(_chunks_in_inter_row), AssemblyTask(1)
    {
    }

    float* a;
    int size;
    int offset;
    int chunk_idx;
    int chunks_per_inter;
    int chunks_in_inter_row;
    
    void cleanup()
    {      
    }

    void execute(int threadid)
    {
	int tid = threadid - leader; 
	if(tid == 0)
	{
            int i, j, k, i_global, j_global, i_here, j_here;
            float sum;           
            float temp[BS*BS] __attribute__ ((aligned (64)));

            for (i = 0; i < BS; i++) {
                //#pragma omp simd
                for (j =0; j < BS; j++){
                    temp[i*BS + j] = a[size*(i + offset) + offset + j ];
                }
            }
            i_global = offset;
            j_global = offset;
            
            // processing top perimeter
            //
            j_global += BS * (chunk_idx+1);
            for (j = 0; j < BS; j++) {
                for (i = 0; i < BS; i++) {
                    sum = 0.f;
                    for (k=0; k < i; k++) {
                        sum += temp[BS*i +k] * BB((i_global+k),(j_global+j));
                    }
                    i_here = i_global + i;
                    j_here = j_global + j;
                    BB(i_here, j_here) = BB(i_here,j_here) - sum;
                }
            }
	    
	}
    }	
};

// Update L blocks for the current column block
class left_perimeter : public AssemblyTask 
{
public:       
left_perimeter(float* _a, int _size, int _offset, int _chunk_idx, int _chunks_per_inter, int _chunks_in_inter_row) :
    a( _a), size(_size),  offset(_offset),  chunk_idx(_chunk_idx), chunks_per_inter(_chunks_per_inter),
    chunks_in_inter_row(_chunks_in_inter_row), AssemblyTask(1)
    {
    }

    float* a;
    int size;
    int offset;
    int chunk_idx;
    int chunks_per_inter;
    int chunks_in_inter_row;
    
    void cleanup()
    {      
    }

    void execute(int threadid)
    {
	int tid = threadid - leader; 
	if(tid == 0)
	{
            int i, j, k, i_global, j_global, i_here, j_here;
            float sum;           
            float temp[BS*BS] __attribute__ ((aligned (64)));

            for (i = 0; i < BS; i++) {
                //#pragma omp simd
                for (j =0; j < BS; j++){
                    temp[i*BS + j] = a[size*(i + offset) + offset + j ];
                }
            }
            i_global = offset;
            j_global = offset;
            
            // processing left perimeter
            //
            j_global = offset;
            i_global += BS * (chunk_idx + 1);
            for (i = 0; i < BS; i++) {
                for (j = 0; j < BS; j++) {
                    sum = 0.f;
                    for (k=0; k < j; k++) {
                        sum += BB((i_global+i),(j_global+k)) * temp[BS*k + j];
                    }
                    i_here = i_global + i;
                    j_here = j_global + j;
                    a[size*i_here + j_here] = ( a[size*i_here+j_here] - sum ) / a[size*(offset+j) + offset+j];
                }
            }
	    
	}
    }	
};

// Update blocks of the trailing matrix
class update_inter_block : public AssemblyTask 
{
public:       
update_inter_block(float* _a, int _size, int _offset, int _chunk_idx, int _chunks_per_inter, int _chunks_in_inter_row) :
    a( _a), size(_size),  offset(_offset),  chunk_idx(_chunk_idx), chunks_per_inter(_chunks_per_inter),
    chunks_in_inter_row(_chunks_in_inter_row), AssemblyTask(1)
    {
    }

    float* a;
    int size;
    int offset;
    int chunk_idx;
    int chunks_per_inter;
    int chunks_in_inter_row;
    
    void cleanup()
    {      
    }

    void execute(int threadid)
    {
	int tid = threadid - leader; 
	if(tid == 0)
	{
            int i, j, k, i_global, j_global;
            float temp_top[BS*BS] __attribute__ ((aligned (64)));
            float temp_left[BS*BS] __attribute__ ((aligned (64)));
            float sum[BS] __attribute__ ((aligned (64))) = {0.f};
            
            i_global = offset + BS * (1 +  chunk_idx/chunks_in_inter_row);
            j_global = offset + BS * (1 + chunk_idx%chunks_in_inter_row);

            for (i = 0; i < BS; i++) {
//#pragma omp simd
                for (j =0; j < BS; j++){
                    temp_top[i*BS + j]  = a[size*(i + offset) + j + j_global ];
                    temp_left[i*BS + j] = a[size*(i + i_global) + offset + j];
                }
            }

            for (i = 0; i < BS; i++)
            {
                for (k=0; k < BS; k++) {
//#pragma omp simd 
                    for (j = 0; j < BS; j++) {
                        sum[j] += temp_left[BS*i + k] * temp_top[BS*k + j];
                    }
                }
//#pragma omp simd 
                for (j = 0; j < BS; j++) {
                    BB((i+i_global),(j+j_global)) -= sum[j];
                    sum[j] = 0.f;
                }
            }
       	    
	}
    }	
};


