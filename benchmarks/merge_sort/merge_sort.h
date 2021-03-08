#include <iostream>
#include <chrono>
#include <omp.h>
#include "xitao.h"
using namespace std;

// declare the class
class MergeSortTAO;

// max size of input array
const uint32_t MAX_ARRAY_SIZE = 100;

// place holder for a 2D array of taos
MergeSortTAO* merge_taos[MAX_ARRAY_SIZE * MAX_ARRAY_SIZE];

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
    
    uint32_t i; // row index of the tao
    uint32_t j; // column index of the tao
    
    // the tao construction. resource hint 1
    MergeSortTAO(uint32_t *_A, uint32_t _n, uint32_t _i, uint32_t _j): A(_A) ,n(_n), i(_i), j(_j), AssemblyTask(1){ }    
    
    // the work function
    void execute(int nthread)
    {	
	if(n > 1)
	    merge(A, left->A, left->n, right->A, right->n);
    }
    
    void cleanup(){  }
};

// build the DAG by reversing the recursion tree 
MergeSortTAO* buildDAG(uint32_t *A, uint32_t n, uint32_t i = 0, uint32_t j = 0) {

    // create a tao    
    merge_taos[i * MAX_ARRAY_SIZE + j] = new MergeSortTAO(A, n, i, j);    
    
    if(n <= 1) // leaf tao
    // {
	// std::cout << "leaf tao: i=" << i << " j=" << j << " n=" << n << " A=" << A[0] << std::endl;
	// push the tao
	xitao_push(merge_taos[i * MAX_ARRAY_SIZE + j]);
    // }
    else
    {
	uint32_t left_n = n / 2;
	uint32_t right_n = n - left_n;
	// std::cout << "i=" << i << " j=" << j << " n=" << n << " left_n=" << left_n << " right_n=" << right_n << " A=[";
	// for(int k = 0; k < n-1; k++)
	//     std::cout << A[k] << ",";
	// std::cout << A[n-1] << "]" << std::endl;
	
	// build the left dag
	merge_taos[i * MAX_ARRAY_SIZE + j]->left = buildDAG(A, left_n, i + 1, j);
	// create edge to current tao
	merge_taos[i * MAX_ARRAY_SIZE + j]->left->make_edge(merge_taos[i * MAX_ARRAY_SIZE + j]);
	// build the right dag
	merge_taos[i * MAX_ARRAY_SIZE + j]->right = buildDAG(A + left_n, right_n, i + 1, j + left_n);
	// create edge to current tao
	merge_taos[i * MAX_ARRAY_SIZE + j]->right->make_edge(merge_taos[i * MAX_ARRAY_SIZE + j]);    
    }
    
    return merge_taos[i * MAX_ARRAY_SIZE + j];
}
