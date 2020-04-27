#include <iostream>
#include "xitao.h"
// the printing TAO (contains work and data)
class printTAO : public AssemblyTask {
  // the data
  const char* str;
  // constructor, execute and cleanup functions are public
  // as they are called by the runtime 
public:
  // constructer fills in the data. 
  // Passes a resource hint of width 1 to parent 
  printTAO(const char* _str) : AssemblyTask(1), str(_str) { }
  // execute the work
  void execute(int nthread) {
    // due to moldability feature, the runtime could decide 
    // that this work needs more than one thread. 
    // so, we get the relative thread id
    int tid = nthread - leader; 
    // make sure only one thread prints
    if(tid == 0)
      //do the work
      std::cout << str;
  }
  // the cleanup function is called by the runtime after
  // execution to claim the resources. 
  // so you could deallocate created memory, close the files, etc., here
  void cleanup () { }
};

/*! @example 
 @brief A a XiTAO Hello world! program*/
int main(int argc, char** argv) {
  // init the runtime
  gotao_init();
  // allocate the Hello TAO
  printTAO* printHello = new printTAO("Hello");
  // allocate the World TAO
  printTAO* printWorld = new printTAO(" world!\n");
  // make an edge, so "Hello" is printed before "world!"
  printHello->make_edge(printWorld);
  // push the head TAO for the DAG
  gotao_push(printHello);
  // execute the pending TAO-DAGs
  gotao_start();
  // wait for all pending executions to finish
  gotao_fini();
  // return
  return 0;
}