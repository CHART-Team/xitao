#include <iostream>
#include <chrono>
#include <omp.h>
#include "xitao.h"
using namespace std; 
const int MAX_FIB = 90;
// basic Fibonacci implementation
size_t fib(int num) {
	// return 0 for 0 and negative terms (undefined)
	if(num <= 0) return 0; 
	// return 1 for the term 1
	else if(num == 1) return 1;
	// recursively find the result
	return fib(num - 1) + fib(num - 2);
}

// the Fibonacci TAO 
class fibTAO : public AssemblyTask {
public:
	// the n - 1 tao
	fibTAO* prev1;		
	// the n - 2 tao																	
	fibTAO* prev2;
	// the term number																				
	int term;		
	// the Fib value for the TAO																					
 	size_t val;							
 	// the tao construction. resource hint 1															
 	fibTAO(int _term): term(_term), AssemblyTask(1) { }		
 	// the work function
 	void execute(int nthread) {	
 		// if this is not a terminal term													
 		if(term > 1) 										
 			// calculate the value  												
 			val = prev1->val + prev2->val;					
 		// assign terminal case 1							
 		else if(term == 1) val = 1;				
 		// assign terminal case 0											
 		else val = 0;						
 	}
 	void cleanup(){ }
};

fibTAO* fib_taos[MAX_FIB + 1];

// build the DAG by reversing the recursion tree 
fibTAO* buildDAG(int term) {
	// gaurd against negative term
	if(term <= 0) term = 0;
	// if this is terminal term
	if(term <= 1) { 
		// create the terminal tao
		fib_taos[term] = new fibTAO(term);
		// push the tao
		gotao_push(fib_taos[term]);
		// return the tao
		return fib_taos[term];
	} 
	// if this TAO has already been created (avoid redundant calculation)
	if(fib_taos[term]) return fib_taos[term];	
	// construct the tao			
	fib_taos[term] = new fibTAO(term);
	// build DAG of n - 1 term
	fib_taos[term]->prev1 = buildDAG(term - 1);
	// make edge to current
	fib_taos[term]->prev1->make_edge(fib_taos[term]);
	// build DAG of n - 1 term
	fib_taos[term]->prev2 = buildDAG(term - 2);
	// make edge to current
	fib_taos[term]->prev2->make_edge(fib_taos[term]);
	// return the current tao (the head of the DAG)
	return fib_taos[term];
}


int main(int argc, char** argv) {
	// accept the term number as argument
	if(argc < 2) {
		// prompt for input
		std::cout << "input Fibonacci term" << std::endl;
		// return error
		return -1;
	}
	// fetch the number
	int num = atoi(argv[1]);
	// error check
	if(num > MAX_FIB or num < 0) {
		// print error
		std::cout << "Bounds error! acceptable range: 0 <= term <= " << MAX_FIB << std::endl;
		// return error
		return -1;	
	}	 
	// initialize the XiTAO runtime system 
	gotao_init();						
	// recursively build the DAG														
 	fibTAO* parent = buildDAG(num);	
 	// fire the XiTAO runtime system 										
 	gotao_start();					
 	// end the XiTAO runtime system												
 	gotao_fini();
 	// print out the result
 	std::cout << "The " << num <<"th Fibonacci term: " << parent->val << std::endl;
	// return success
	return 0;
}
