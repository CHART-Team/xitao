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

// heat-tao.cxx: TAO version of heat
// The initial version implements only jacobi

/*
 * Iterative solver for heat distribution
 */

#include "xitao.h"
#include "solver-tao.h"
#include <stdio.h>
#include <stdlib.h>
#include "heat.h"

using namespace xitao;

#ifdef DO_LOI
#include "loi.h"
#endif
#ifndef TAO_STA
#error "./heat-tao requires TAO_STA"
#endif 

// Use LOI instrumentation: 
#ifdef DO_LOI

/* this structure describes the relationship between phases and kernels in the application */ 
struct loi_kernel_info heat_kernels = {
        2,              // 2 kernels in total
        1,              // 1 phase
        {"Jacobi_Core", "Copy_Core"}, // Name of the two kernels
        {"Heat"},                     // Name of the phase
        {(1<<JACOBI2D | 1<<COPY2D)},  // both kernels belong to the same phase [0]
        };

#endif

void usage( char *s )
{
    fprintf(stderr, 
        "Usage: %s <input file> <use workload hint> [result file]\n\n", s);
}

int main( int argc, char *argv[] )
{
    int thread_base; int nthreads; 
    unsigned iter;
    FILE *infile, *resfile;
    const char *resfilename;
    int awidth, exdecomp, eydecomp, ixdecomp, iydecomp;

    // algorithmic parameters
    algoparam_t param;
    int np;

    double runtime, flop;
    double residual=0.0;

    // check arguments
    if( argc < 3 )
    {
    usage( argv[0] );
    return 1;
    }

    // check input file
    if( !(infile=fopen(argv[1], "r"))  ) 
    {
    fprintf(stderr, 
        "\nError: Cannot open \"%s\" for reading.\n\n", argv[1]);
      
    usage(argv[0]);
    return 1;
    }

    use_workload_hint = (argc>=3) ? atoi(argv[2]) : 0;
    // check result file
    resfilename= (argc>=4) ? argv[3]:"heat.ppm";

    if( !(resfile=fopen(resfilename, "w")) )
    {
    fprintf(stderr, 
        "\nError: Cannot open \"%s\" for writing.\n\n", 
        resfilename);
    usage(argv[0]);
    return 1;
    }

    // check input
    if( !read_input(infile, &param) )
    {
    fprintf(stderr, "\nError: Error parsing input file.\n\n");
    usage(argv[0]);
    return 1;
    }
    print_params(&param);

   if(getenv("XITAO_MAXTHREADS"))
        nthreads = atoi(getenv("XITAO_MAXTHREADS"));
   else 
        nthreads = XITAO_MAXTHREADS;

   if(getenv("GOTAO_THREAD_BASE"))
        thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
   else
        thread_base = GOTAO_THREAD_BASE;


    if( !initialize(&param, 1) )
    {
        fprintf(stderr, "Error in Solver initialization.\n\n");
        usage(argv[0]);
            return 1;
    }

    // full size (param.resolution are only the inner points)
    np = param.resolution + 2;

    // now we generate the whole DAG
    // There is no indeterminism here, so we can do that in full here
    // But need to take care with memory size. The number of elements and pointers
    // to be stored is iter*x_decop*y_decomp -> 16*iter or 64*iter depending on
    // the decomposition. For 1000 iterations this means 16000 or 64000 items.
   
    // define some default values

    //copyback cpb[param.maxiter][X_DECOMP][Y_DECOMP];

    if(getenv("AWIDTH")) 
	awidth = atoi(getenv("AWIDTH"));
    else
 	awidth = AWIDTH;

    if(getenv("EXDECOMP")) 
	exdecomp = atoi(getenv("EXDECOMP"));
    else
 	exdecomp = EXDECOMP;

    if(getenv("EYDECOMP")) 
	eydecomp = atoi(getenv("EYDECOMP"));
    else
 	eydecomp = EYDECOMP;

    if(getenv("IXDECOMP")) 
	ixdecomp = atoi(getenv("IXDECOMP"));
    else
 	ixdecomp = IXDECOMP;

    if(getenv("IYDECOMP")) 
	iydecomp = atoi(getenv("IYDECOMP"));
    else
 	iydecomp = IYDECOMP;

    std::cout << "TAO configuration:\n\tAssembly width = " << awidth << 
			           "\n\tExternal X Decomposition = " << exdecomp <<
			           "\n\tExternal Y Decomposition = " << eydecomp <<
			           "\n\tInternal X Decomposition = " << ixdecomp <<
			           "\n\tInternal Y Decomposition = " << iydecomp << std::endl;
    
    // When using stas, the runtime needs to be initialized before we start creating objects
    xitao_init(argc, argv);

    // first a single iteration of a stencil
    jacobi2D *stc[param.maxiter][exdecomp][eydecomp];
    copy2D   *cpb[param.maxiter][exdecomp][eydecomp];
    copy2D   *init1[exdecomp][eydecomp];  // param->u
    copy2D   *init2[exdecomp][eydecomp];  // param->uhelp

    // when specifying chunk sizes and offsets I use "ceil"-style division [a/b -> (a + b -1)/b] to 
    // avoid generating minichunks when the division (a/b) generates a remainder
    // Of course, the question now is: why not specify the parameters in terms of rational (a/b) partitions?
   
    // the first task will be to initialize the memory in param.u using tasks, so that a NUMA aware organization is derived
    // automatically from the virtual topology stas


#define ceildiv(a,b) ((a + b -1)/(b))

    iter = 0;
    // allocate a large array to both param.u and param.uhelp but do not initialize yet
    // we will use the first touch policy to make sure that the initialization is done close to where data is touched first time
    //
    // the below way of doing things is wasteful, but I for the while I don't want
    // to create another copy2D with two outputs so I keep it like this

    // create initial copy (this is intended to make this code NUMA-aware)
    // The initialized data is stored in param.ftmp
    //
#ifdef NUMA_ALLOC
    param.u = (double*) malloc( sizeof(double)*np*np );
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          init1[x][y] = new copy2D(
                             param.ftmp, 
                             param.u,
                             np, np,
                             x*ceildiv(np, exdecomp), // x*((np + exdecomp -1) / exdecomp),
                             y*ceildiv(np, eydecomp), //y*((np + eydecomp -1) / eydecomp),
                             ceildiv(np, exdecomp), //(np + exdecomp - 1) / exdecomp,
                             ceildiv(np, eydecomp), //(np + eydecomp - 1) / eydecomp, 
                             gotao_sched_2D_static,
                             ceildiv(np, ixdecomp*exdecomp), // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                             ceildiv(np, iydecomp*eydecomp), //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                             awidth);
          init1[x][y]->set_sta((float) (x * eydecomp + y) / (float) (exdecomp*eydecomp));
          if(use_workload_hint != 0) init1[x][y]->workload_hint = x * eydecomp + y;
          xitao_push(init1[x][y]); // insert into affinity queue
       }

    param.uhelp = (double*) malloc( sizeof(double)*np*np );

    // why is this being done? it seems that the next jacoby 2D just overwrites this copy?
    // ok -- we need it because the jacobi kernel updates the output, it does not copy it.
    // but hasnt uhelp not already been copied during the initialization?
    // ok -- yes, but not in a NUMA aware way. The following code ensures the NUMA-aware update
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          init2[x][y] = new copy2D(
                             param.u, 
                             param.uhelp,
                             np, np,
                             x*ceildiv(np, exdecomp), // x*((np + exdecomp -1) / exdecomp),
                             y*ceildiv(np, eydecomp), //y*((np + eydecomp -1) / eydecomp),
                             ceildiv(np, exdecomp), //(np + exdecomp - 1) / exdecomp,
                             ceildiv(np, eydecomp), //(np + eydecomp - 1) / eydecomp, 
                             gotao_sched_2D_static,
                             ceildiv(np, ixdecomp*exdecomp), // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                             ceildiv(np, iydecomp*eydecomp), //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                             awidth);

          init2[x][y]->clone_sta(init1[x][y]);
          init1[x][y]->make_edge(init2[x][y]);
       }
    // when not using NUMA allocation, we do not need to run any of this code since both u and uhelp are already initialized
#endif

    // create initial stencils (prologue)
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          stc[iter][x][y] = new jacobi2D(
                             param.u, 
                             param.uhelp,
                             np, np,
                             x*ceildiv(np, exdecomp), // x*((np + exdecomp -1) / exdecomp),
                             y*ceildiv(np, eydecomp), //y*((np + eydecomp -1) / eydecomp),
                             ceildiv(np, exdecomp), //(np + exdecomp - 1) / exdecomp,
                             ceildiv(np, eydecomp), //(np + eydecomp - 1) / eydecomp, 
                             gotao_sched_2D_static,
                             ceildiv(np, ixdecomp*exdecomp), // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                             ceildiv(np, iydecomp*eydecomp), //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                             awidth);

#ifdef NUMA_ALLOC
          stc[iter][x][y]->clone_sta(init2[x][y]);
          init2[x][y]->make_edge(stc[iter][x][y]);

          if((x-1)>=0)       init2[x-1][y]->make_edge(stc[iter][x][y]);
          if((x+1)<exdecomp) init2[x+1][y]->make_edge(stc[iter][x][y]);
          if((y-1)>=0)       init2[x][y-1]->make_edge(stc[iter][x][y]);
          if((y+1)<eydecomp) init2[x][y+1]->make_edge(stc[iter][x][y]);
#else
#ifdef TOPOPLACE
          stc[iter][x][y]->set_sta((float) (x * eydecomp + y) / (float) (exdecomp*eydecomp));
          if(use_workload_hint != 0) stc[iter][x][y]->workload_hint = x * eydecomp + y;
#else // no topo
          stc[iter][x][y]->set_sta(0.0);
#endif  // TOPOPLACE
          xitao_push(stc[iter][x][y]);
#endif  // NUMA_ALLOC


       }

    // from this point just creat "iter" copies of the loop
    while(iter < param.maxiter-1)
    {

    // create copies
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          cpb[iter][x][y] = new copy2D(
                             param.uhelp, 
                             param.u,
                             np, np,
                             x*ceildiv(np, exdecomp), 
                             y*ceildiv(np, eydecomp), 
                             ceildiv(np, exdecomp), 
                             ceildiv(np, eydecomp), 
                             gotao_sched_2D_static,
                             ceildiv(np, ixdecomp*exdecomp), 
                             ceildiv(np, iydecomp*eydecomp), 
                             awidth);

// this should ensure that we do not overwrite data which has not yet been fully processed
// necessary because we do not do renaming
          
          if(use_workload_hint != 0) cpb[iter][x][y]->workload_hint = x * eydecomp + y;
          stc[iter][x][y]->make_edge(cpb[iter][x][y]);
          cpb[iter][x][y]->clone_sta(stc[iter][x][y]);
          cpb[iter][x][y]->criticality = 0; 
          stc[iter][x][y]->criticality = 0; 
          if((x-1)>=0)       stc[iter][x-1][y]->make_edge(cpb[iter][x][y]);
          if((x+1)<exdecomp) stc[iter][x+1][y]->make_edge(cpb[iter][x][y]);
          if((y-1)>=0)       stc[iter][x][y-1]->make_edge(cpb[iter][x][y]);
          if((y+1)<eydecomp) stc[iter][x][y+1]->make_edge(cpb[iter][x][y]);

       }

    iter++;

    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          stc[iter][x][y] = new jacobi2D(
                             param.u, 
                             param.uhelp,
                             np, np,
                             x*ceildiv(np, exdecomp), // x*((np + exdecomp -1) / exdecomp),
                             y*ceildiv(np, eydecomp), //y*((np + eydecomp -1) / eydecomp),
                             ceildiv(np, exdecomp), //(np + exdecomp - 1) / exdecomp,
                             ceildiv(np, eydecomp), //(np + eydecomp - 1) / eydecomp, 
                             gotao_sched_2D_static,
                             ceildiv(np, ixdecomp*exdecomp), // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                             ceildiv(np, iydecomp*eydecomp), //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                             awidth);

          if(use_workload_hint != 0) stc[iter][x][y]->workload_hint = x * eydecomp + y;
          cpb[iter-1][x][y]->make_edge(stc[iter][x][y]);
          stc[iter][x][y]->clone_sta(cpb[iter-1][x][y]);
          cpb[iter-1][x][y]->criticality = 0; 
          stc[iter][x][y]->criticality = 0; 

          if((x-1)>=0)       cpb[iter-1][x-1][y]->make_edge(stc[iter][x][y]);
          if((x+1)<exdecomp) cpb[iter-1][x+1][y]->make_edge(stc[iter][x][y]);
          if((y-1)>=0)       cpb[iter-1][x][y-1]->make_edge(stc[iter][x][y]);
          if((y+1)<eydecomp) cpb[iter-1][x][y+1]->make_edge(stc[iter][x][y]);
       }
    }

    // the epilogue
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          cpb[iter][x][y] = new copy2D(
                             param.uhelp, 
                             param.u,
                             np, np,
                             x*ceildiv(np, exdecomp), 
                             y*ceildiv(np, eydecomp), 
                             ceildiv(np, exdecomp), 
                             ceildiv(np, eydecomp), 
                             gotao_sched_2D_static,
                             ceildiv(np, ixdecomp*exdecomp), 
                             ceildiv(np, iydecomp*eydecomp), 
                             awidth);

// this should ensure that we do not overwrite data which has not yet been fully processed
// necessary because we do not do renaming
          if(use_workload_hint != 0) cpb[iter][x][y]->workload_hint = x * eydecomp + y;
          stc[iter][x][y]->make_edge(cpb[iter][x][y]);
          cpb[iter][x][y]->clone_sta(stc[iter][x][y]);

          if((x-1)>=0)       stc[iter][x-1][y]->make_edge(cpb[iter][x][y]);
          if((x+1)<exdecomp) stc[iter][x+1][y]->make_edge(cpb[iter][x][y]);
          if((y-1)>=0)       stc[iter][x][y-1]->make_edge(cpb[iter][x][y]);
          if((y+1)<eydecomp) stc[iter][x][y+1]->make_edge(cpb[iter][x][y]);

       }

   std::chrono::time_point<std::chrono::system_clock> start, end;


// LOI instrumentation
#if DO_LOI
    loi_init(); // calc TSC freq and init data structures
    printf(" TSC frequency has been measured to be: %g Hz\n", (double) TSCFREQ);
    int maxthr = nthreads;
#endif

#ifdef DO_LOI 
   phase_profile_start();
#endif
   start = std::chrono::system_clock::now();
   xitao_start();

   // here the computation takes sta

   xitao_fini();
   // wait for all threads to synchronize
#ifdef DO_LOI
    phase_profile_stop(0); 
#endif
   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";

   /*
    iter = 0;
    while(1) {
    switch( param.algorithm ) {
        case 0: // JACOBI
                residual = relax_jacobi(np, (double (*)[np])param.u, (double (*)[np])param.uhelp,  np);
            // Copy uhelp into u
            for (int i=0; i<np; i++)
                    for (int j=0; j<np; j++){
                    param.u[ i*np+j ] = param.uhelp[ i*np+j ];
                    }
            break;
        case 1: // GAUSS
            residual = relax_gauss(param.padding, np, (double (*)[np])param.u,  np);
            break;
        case 2: // RED-BLACK
            residual = relax_redblack(np, (double (*)[np])param.u,  np);
            break;
        }

        iter++;

        // solution good enough ?
        if (((iter%1000)==0) && (residual < 0.00005)) break;

        // max. iteration reached ? (no limit with maxiter=0)
        if (param.maxiter>0 && iter>=param.maxiter) break;
    }
*/

   // copy back from the last entry
//    for (int i=0; i<np; i++)
//         for (int j=0; j<np; j++)
//              param.u[ i*np+j ] = param.uhelp[ i*np+j ];

    // Flop count after iter iterations
    flop = iter * 11.0 * param.resolution * param.resolution;
    // stopping time
//    runtime = wtime() - runtime;
//
//    fprintf(stdout, "Time: %04.3f ", runtime);
//    fprintf(stdout, "(%3.3f GFlop => %6.2f MFlop/s)\n", 
//        flop/1000000000.0,
//        flop/runtime/1000000);
//    fprintf(stdout, "Convergence to residual=%f: %d iterations\n", residual, iter);

#ifdef DO_LOI
#ifdef LOI_TIMING
    loi_statistics(&heat_kernels, maxthr);
#endif
#ifdef DO_KRD
    krd_save_traces();
#endif
#endif

    if(getenv("TAO_NOPLOT")) return 0;

    // for plot...
    coarsen( param.u, np, np, param.padding,
         param.uvis, param.visres+2, param.visres+2 );
  
    write_image( resfile, param.uvis, param.padding,
         param.visres+2, 
         param.visres+2 );

    if(xitao::config::use_performance_modeling){ 
      xitao_ptt::print_table<copy2D>("copy2D");
      xitao_ptt::print_table<jacobi2D>("jacobi2D");
    }


    finalize( &param );

    return 0;
}
