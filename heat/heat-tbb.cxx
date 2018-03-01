// heat-tbb.cxx: TBB version of heat
// The version implements only jacobi

/*
 * Iterative solver for heat distribution
 */

#include <tbb/flow_graph.h>
#include "tbb/task_scheduler_init.h"

using namespace std; // for task_scheduler_init
using namespace tbb::flow;
using namespace tbb;

#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "heat.h"

#ifdef DO_LOI
#include "loi.h"
#endif

// Use LOI instrumentation: 
#ifdef DO_LOI

#define JACOBI2D 0
#define COPY2D 1
#define KRDBLOCKX 128
#define KRDBLOCKY 128

#define min(a,b) ( ((a) < (b)) ? (a) : (b) )

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
        "Usage: %s <input file> [result file]\n\n", s);
}

int ndx(int a, int b, int c){ return a*c + b; }

void jacobi2D_tbb(void *i, void *o, int rows, int cols, 
                  int offx, int offy, int chunkx, int chunky)
{
                    double *in  = (double *) i;
                    double *out = (double *) o;

                    // global rows and cols
                   // std::cout << "Jacobi 2D: " << offx << " " << offy << std::endl;

                    int xstart = (offx == 0)? 1 : offx;
                    int ystart = (offy == 0)? 1 : offy;
                    int xstop = ((offx + chunkx) >= rows)? rows - 1: offx + chunkx;
                    int ystop = ((offy + chunky) >= cols)? cols - 1: offy + chunky;

#if DO_LOI
    kernel_profile_start();
#endif
                    for (int i=xstart; i<xstop; i++) 
                        for (int j=ystart; j<ystop; j++) {
                        out[ndx(i,j,cols)]= 0.25 * (in[ndx(i,j-1,cols)]+  // left
                                               in[ndx(i,j+1,cols)]+  // right
                                               in[ndx(i-1,j,cols)]+  // top
                                               in[ndx(i+1,j,cols)]); // bottom
                               
                        }
#if DO_LOI
    kernel_profile_stop(JACOBI2D);
#if DO_KRD
    for(int x = xstart; x < xstop; x += KRDBLOCKX)
      for(int y = ystart; y < ystop; y += KRDBLOCKY)
      {
      int krdblockx = ((x + KRDBLOCKX - 1) < xstop)? KRDBLOCKX : xstop - x;
      int krdblocky = ((y + KRDBLOCKY - 1) < ystop)? KRDBLOCKY : ystop - y;
      kernel_trace1(JACOBI2D, &in[ndx(x,y,cols)], KREAD(krdblockx*krdblocky)*sizeof(double));
      kernel_trace1(JACOBI2D, &out[ndx(x,y,cols)], KWRITE(krdblockx*krdblocky)*sizeof(double));
      }
#endif
#endif

}

void copy2D_tbb(void *i,  void *o,  int rows,   int cols, 
                int offx, int offy, int chunkx, int chunky)
{
                    double *in  = (double *) i;
                    double *out = (double *) o;

                    //std::cout << "Copy 2D: " << offx << " " << offy << std::endl;

                    int xstart = (offx == 0)? 1 : offx;
                    int ystart = (offy == 0)? 1 : offy;
                    int xstop = ((offx + chunkx) >= rows)? rows - 1: offx + chunkx;
                    int ystop = ((offy + chunky) >= cols)? cols - 1: offy + chunky;

#if DO_LOI
    kernel_profile_start();
#endif
                    for (int i=xstart; i<xstop; i++) 
                        for (int j=ystart; j<ystop; j++) 
                            out[ndx(i,j,cols)]= in[ndx(i,j,cols)];

#if DO_LOI
    kernel_profile_stop(COPY2D);
#if DO_KRD
                    for(int x = xstart; x < xstop; x += KRDBLOCKX)
                      for(int y = ystart; y < ystop; y += KRDBLOCKY)
                      {
                      int krdblockx = ((x + KRDBLOCKX - 1) < xstop)? KRDBLOCKX : xstop - x;
                      int krdblocky = ((y + KRDBLOCKY - 1) < ystop)? KRDBLOCKY : ystop - y;
                      kernel_trace1(COPY2D, &in[ndx(x,y,cols)], KREAD(krdblockx*krdblocky)*sizeof(double));
                      kernel_trace1(COPY2D, &out[ndx(x,y,cols)], KWRITE(krdblockx*krdblocky)*sizeof(double));
                      }
#endif
#endif

}



int main( int argc, char *argv[] )
{
    int thread_base;
    unsigned iter;
    FILE *infile, *resfile;
    const char *resfilename;
    int exdecomp, eydecomp;

    // algorithmic parameters
    algoparam_t param;
    int np;

    double runtime, flop;
    double residual=0.0;

    // check arguments
    if( argc < 2 )
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

    // check result file
    resfilename= (argc>=3) ? argv[2]:"heat.ppm";

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

    int tbb_nthreads;
    if(getenv("TBB_NTHREADS"))
        tbb_nthreads = atoi(getenv("TBB_NTHREADS"));
    else 
        tbb_nthreads = task_scheduler_init::automatic;
    task_scheduler_init init(tbb_nthreads);

    if( !initialize(&param, 1) )
    {
        fprintf(stderr, "Error in Solver initialization.\n\n");
        usage(argv[0]);
            return 1;
    }

    // full size (param.resolution are only the inner points)
    np = param.resolution + 2;

    // now we generate the whole DAG
    // There is no nondeterminism here, so we can do that in full here
    // But need to take care with memory size. The number of elements and pointers
    // to be stored is iter*x_decop*y_decomp -> 16*iter or 64*iter depending on
    // the decomposition. For 1000 iterations this means 16000 or 64000 items.
   
    // The TBB version requires only parameters for the external decomposition


    if(getenv("EXDECOMP")) 
      exdecomp = atoi(getenv("EXDECOMP"));
    else
      exdecomp = EXDECOMP;

    if(getenv("EYDECOMP")) 
      eydecomp = atoi(getenv("EYDECOMP"));
    else
      eydecomp = EYDECOMP;

    std::cout << "TBB configuration:\n\t" << 
                        "\tExternal X Decomposition = "   << exdecomp <<
                        "\n\tExternal Y Decomposition = " << eydecomp <<
                          std::endl;
    
    // TBB flow_graph nodes
    continue_node<continue_msg> *stc[param.maxiter][exdecomp][eydecomp];
    continue_node<continue_msg> *cpb[param.maxiter][exdecomp][eydecomp];
    graph g;

#define ceildiv(a,b) ((a + b -1)/(b))

    iter = 0;

    // create initial stencils (prologue)
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          stc[iter][x][y] = new continue_node<continue_msg>(g, [=]
                      (const continue_msg&) { 
                         jacobi2D_tbb( param.u, param.uhelp, np, np,
                             x*ceildiv(np, exdecomp), y*ceildiv(np, eydecomp), 
                             ceildiv(np, exdecomp), ceildiv(np, eydecomp)); 
                             });

       }

    // from this point just creat "iter" copies of the loop
    while(iter < param.maxiter-1)
    {

    // create copies
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          cpb[iter][x][y] = new continue_node<continue_msg>(g, [=] 
                      (const continue_msg&) { 
                         copy2D_tbb( param.uhelp, param.u, np, np,
                             x*ceildiv(np, exdecomp), y*ceildiv(np, eydecomp), 
                             ceildiv(np, exdecomp), ceildiv(np, eydecomp)); 
                             });

          make_edge(*stc[iter][x][y], *cpb[iter][x][y]);

          if((x-1)>=0)       make_edge(*stc[iter][x-1][y], *cpb[iter][x][y]);
          if((x+1)<exdecomp) make_edge(*stc[iter][x+1][y], *cpb[iter][x][y]);
          if((y-1)>=0)       make_edge(*stc[iter][x][y-1], *cpb[iter][x][y]);
          if((y+1)<eydecomp) make_edge(*stc[iter][x][y+1], *cpb[iter][x][y]);

       }

    iter++;

    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          stc[iter][x][y] = new continue_node<continue_msg>(g, [=] 
                      (const continue_msg&) { 
                         jacobi2D_tbb( param.u, param.uhelp, np, np,
                             x*ceildiv(np, exdecomp), y*ceildiv(np, eydecomp), 
                             ceildiv(np, exdecomp), ceildiv(np, eydecomp)); 
                             });

          make_edge(*cpb[iter-1][x][y], *stc[iter][x][y]);

          if((x-1)>=0)       make_edge(*cpb[iter-1][x-1][y], *stc[iter][x][y]);
          if((x+1)<exdecomp) make_edge(*cpb[iter-1][x+1][y], *stc[iter][x][y]);
          if((y-1)>=0)       make_edge(*cpb[iter-1][x][y-1], *stc[iter][x][y]);
          if((y+1)<eydecomp) make_edge(*cpb[iter-1][x][y+1], *stc[iter][x][y]);
       }
    }

    // the epilogue
    for(int x = 0; x < exdecomp; x++)
       for(int y = 0; y < eydecomp; y++) 
       {
          cpb[iter][x][y] = new continue_node<continue_msg>(g, [=] 
                      (const continue_msg&) { 
                         copy2D_tbb( param.uhelp, param.u, np, np,
                             x*ceildiv(np, exdecomp), y*ceildiv(np, eydecomp), 
                             ceildiv(np, exdecomp), ceildiv(np, eydecomp)); 
                             });

          make_edge(*stc[iter][x][y], *cpb[iter][x][y]);

          if((x-1)>=0)       make_edge(*stc[iter][x-1][y], *cpb[iter][x][y]);
          if((x+1)<exdecomp) make_edge(*stc[iter][x+1][y], *cpb[iter][x][y]);
          if((y-1)>=0)       make_edge(*stc[iter][x][y-1], *cpb[iter][x][y]);
          if((y+1)<eydecomp) make_edge(*stc[iter][x][y+1], *cpb[iter][x][y]);

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

   // in theory this should start the computation
   for(int x = 0; x < exdecomp; x++)
     for(int y = 0; y < eydecomp; y++) 
        stc[0][x][y]->try_put(continue_msg());

   g.wait_for_all();

   // wait for all threads to synchronize
#ifdef DO_LOI
    phase_profile_stop(0); 
#endif
   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "\n";


    // Flop count after iter iterations
    flop = iter * 11.0 * param.resolution * param.resolution;
    // stopping time
//    runtime = wtime() - runtime;

#ifdef DO_LOI
#ifdef LOI_TIMING
    loi_statistics(&heat_kernels, tbb_nthreads);
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

    finalize( &param );

    return 0;
}
