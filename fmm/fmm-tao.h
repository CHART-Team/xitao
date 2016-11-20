#include "tao.h"
#include <vector>

class fmm_st : public AssemblyTask
{
        public:

        fmm_st(int width) : AssemblyTask(width), nx(0), w(width)
        {
                cv = new std::vector<Cell*>[width];    
        }

        int insert(Cell *C)
        {
                cv[nx].push_back(C);
                ny = (ny+1) % w;
		if(!ny)
                    nx = (nx+1) % w;
        }

        int cleanup(){
                delete[] cv;
        }

        // this assembly can work totally asynchronously
        int execute(int threadid)
        {
        int tid = threadid - leader;
	//std::cout << "Leader " << leader << ", Worker id " << tid << " started " << std::endl;
        for(int c=0; c<cv[tid].size(); c++) {                     // Loop over cells
          Cell * C = cv[tid][c];                                   //  Current cell
          Evaluate evaluate(C);                                    //  Instantiate functor
          evaluate();
          }
        }

        int w;
        std::vector<Cell*> * cv;                // internal list of cells, one per virtual core
        int nx;                                 // we do round robin distribution here
	int ny;
        GENERIC_LOCK(io); 

};

