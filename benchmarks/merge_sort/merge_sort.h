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

#include <iostream>
#include <chrono>
#include <omp.h>
#include "xitao.h"
using namespace std;

// declare the class
class MergeSortTAO;
void mergesort_seq(uint32_t *A, uint32_t n);
uint32_t leaf;
uint32_t N;
uint32_t use_sta = 0;
uint32_t use_workload_hint = 0;
// basic merge sort implementation
void merge(uint32_t* A, uint32_t* left_A, uint32_t left_n, uint32_t* right_A, uint32_t right_n)
{
    uint32_t n = left_n + right_n; // length of the input array A
    uint32_t *temp = new uint32_t[n]; // temp array to perform the merge
    uint32_t left_i = 0; // index for the left array
    uint32_t right_i = 0; // index for the right array
    uint32_t i = 0; // index for the temp array 

    // while having entries in both arrays, pick the smallest
    while ((left_i < left_n) and (right_i < right_n))
    {
	// left is smallest
	if(left_A[left_i] <= right_A[right_i])
	{
	    temp[i] = left_A[left_i];
	    left_i++;
	    i++;
	}
	// right is smallest
	else
	{
	    temp[i] = right_A[right_i];
	    right_i++;
	    i++;
	}
    }
    // we reached the end of one of the two arrays
    // copy the rest of the left array if we didn't reach its end
    while (left_i < left_n)
    {
	temp[i] = left_A[left_i];
	left_i++; 
	i++;
    }
    // copy the rest of the right array if we didn't reach its end
    while(right_i < right_n)
    {
	temp[i] = right_A[right_i];
	right_i++;
	i++;
    }
    // update the input array
    for(i = 0; i < n; i++)
	A[i] = temp[i];
    // clean up
    delete temp;
}

// the Merge Sort TAO (Every TAO class must inherit from AssemblyTask)
class MergeSortTAO : public AssemblyTask {
public:
    // the left tao
    MergeSortTAO* left; 
    // the right tao
    MergeSortTAO* right; 
    // the length of the array
    uint32_t n;
    // the array of the TAO
    uint32_t *A;
    
    // the tao construction. resource hint 1
    MergeSortTAO(uint32_t *_A, uint32_t _n): A(_A) ,n(_n), left(NULL), right(NULL), AssemblyTask(1){ 
      if(use_workload_hint != 0) workload_hint = n;
    }    
    
    // the work function
    void execute(int nthread)
    {
      if(nthread != leader) return;
      if(left != NULL) {
        merge(A, left->A, left->n, right->A, right->n);
      } else if(n > 1) {
        mergesort_seq(A, n);
      }
    }
    
    void cleanup() { 
      if(left != NULL) delete left;
      if(right != NULL) delete right;
    }
};

// build the DAG by reversing the recursion tree 
MergeSortTAO* buildDAG(uint32_t *A, uint32_t n, uint32_t offset) {

    // create a tao    
    MergeSortTAO* current = new MergeSortTAO(A, n);    
    if(use_sta != 0)
      current->set_sta((float)offset / N);
    if(n <= leaf) {
	xitao_push(current);
    } 
    else
    {
	uint32_t left_n = n / 2;
	uint32_t right_n = n - left_n;
	// build the left dag
	current->left = buildDAG(A, left_n, offset);
	// create edge to current tao
	current->left->make_edge(current);
	// build the right dag
	current->right = buildDAG(A + left_n, right_n, offset + left_n);
	// create edge to current tao
	current->right->make_edge(current);    
    }
    
    return current;
}

void mergesort_seq(uint32_t *A, uint32_t n) {
  if(n <= 1) return;
  uint32_t left_n = n / 2;
  uint32_t right_n = n - left_n;
  mergesort_seq(A, left_n);
  mergesort_seq(A + left_n, right_n);
  merge(A, A, left_n, A + left_n, right_n); 
}

void mergesort_par(uint32_t *A, uint32_t n) {
  if(n <= 1) return;
  uint32_t left_n = n / 2;
  uint32_t right_n = n - left_n;
#pragma omp task untied if (n > leaf)
  mergesort_par(A, left_n);
#pragma omp task untied if (n > leaf)
  mergesort_par(A + left_n, right_n);
#pragma omp taskwait
  merge(A, A, left_n, A + left_n, right_n); 
}

