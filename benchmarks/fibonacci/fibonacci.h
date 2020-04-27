#include <iostream>
#include <chrono>
#include <omp.h>
#include "xitao.h"
using namespace std;

// enable a known trick to avoid redundant recursion for evaluated cases
//#define MEMOIZE
// the maximum number of Fibonacci terms that can fit in unsigned 64 bit
const uint32_t MAX_FIB = 92;

// a global variable to manage the granularity of TAO creation (coarsening level)
uint32_t grain_size;

// declare the class
class FibTAO;
// init the memoization array of TAOs 
FibTAO* fib_taos[MAX_FIB + 1];

// basic Fibonacci implementation
size_t fib(uint32_t num) {
	// return 0 for 0 and negative terms (undefined)
	if(num <= 0) return 0; 
	// return 1 for the term 1
	else if(num == 1) return 1;
	// recursively find the result
	return fib(num - 1) + fib(num - 2);
}

// the Fibonacci TAO (Every TAO class must inherit from AssemblyTask)
class FibTAO : public AssemblyTask {
public:
	// the n - 1 tao
	FibTAO* prev1;		
	// the n - 2 tao																	
	FibTAO* prev2;
	// the term number																				
	uint32_t term;		
	// the Fib value for the TAO																					
 	size_t val;							
 	// the tao construction. resource hint 1															
 	FibTAO(int _term): term(_term), AssemblyTask(1) { }		
 	// the work function
 	void execute(int nthread) {	
 		// calculate locally if at required granularity
 		if(term <= grain_size) val = fib(term);
 		// if this is not a terminal term													
 		else if(term > 1) 										
 			// calculate the value  												
 			val = prev1->val + prev2->val;
 	}
 	void cleanup(){  }
};

// build the DAG by reversing the recursion tree 
FibTAO* buildDAG(uint32_t term) {
	// gaurd against negative terms
	if(term <  0) term = 0;
	// if this is terminal term
	if(term <= 1) { 
		// create the terminal tao
		fib_taos[term] = new FibTAO(term);
		// push the tao
		gotao_push(fib_taos[term]);
		// return the tao
		return fib_taos[term];
	} 
#ifdef MEMOIZE
	// if this TAO has already been created (avoid redundant calculation)
	if(fib_taos[term]) return fib_taos[term];
#endif	
	// construct the tao			
	fib_taos[term] = new FibTAO(term);
	// create TAOs as long as you are above the grain size
	if(term > grain_size) { 
		// build DAG of n - 1 term
		fib_taos[term]->prev1 = buildDAG(term - 1);
		// make edge to current
		fib_taos[term]->prev1->make_edge(fib_taos[term]);
		// build DAG of n - 1 term
		fib_taos[term]->prev2 = buildDAG(term - 2);
		// make edge to current
		fib_taos[term]->prev2->make_edge(fib_taos[term]);
	} else { // you have reached a terminal TAO 
		// push the TAO to fire the DAG execution
		gotao_push(fib_taos[term]);
	}
	// return the current tao (the head of the DAG)
	return fib_taos[term];
}
