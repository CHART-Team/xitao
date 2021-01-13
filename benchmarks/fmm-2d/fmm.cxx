#include "build_tree.h"
#include "kernel.h"
#include "timer.h"
#include "traverse.h"
#include <iostream>
using namespace exafmm;

int main(int argc, char ** argv) {
  if(argc < 3) {
    std::cout << "Usage: ./bin numBodies ncrit" << std::endl;
    exit(0);
  }
  const int numBodies = atoi(argv[1]);                          // Number of bodies
  P = 10;                                                       // Order of expansions
  ncrit = atoi(argv[2]);                                        // Number of bodies per leaf cell
  theta = 0.4;                                                  // Multipole acceptance criterion
#ifdef USE_XITAO
  xitao_init(argc, argv);
#endif
  printf("--- %-16s ------------\n", "FMM Profiling");          // Start profiling
  //! Initialize bodie
  start("Initialize bodies");                                   // Start timer
  Bodies bodies(numBodies);                                     // Initialize bodies
  real_t average = 0;                                           // Average charge
  srand48(0);                                                   // Set seed for random number generator
  for (size_t b=0; b<bodies.size(); b++) {                      // Loop over bodies
    for (int d=0; d<2; d++) {                                   //  Loop over dimension
      bodies[b].X[d] = drand48() * 2 * M_PI - M_PI;             //   Initialize positions
    }                                                           //  End loop over dimension
    bodies[b].q = drand48() - .5;                               //  Initialize charge
    average += bodies[b].q;                                     //  Accumulate charge
    bodies[b].p = 0;                                            //  Clear potential
    for (int d=0; d<2; d++) bodies[b].F[d] = 0;                 //  Clear force
  }                                                             // End loop over bodies
  average /= bodies.size();                                     // Average charge
  for (size_t b=0; b<bodies.size(); b++) {                      // Loop over bodies
    bodies[b].q -= average;                                     // Charge neutral
  }                                                             // End loop over bodies
  stop("Initialize bodies");                                    // Stop timer

  //! Build tree
  start("Build tree");                                          // Start timer
  Cells cells = buildTree(bodies);                              // Build tree
  stop("Build tree");                                           // Stop timer
#if USE_XITAO
  //! FMM DAG building
  start("FMM DAG");                                             // Start timer
  upwardPass(cells);                                            // Upward pass for P2M, M2M
  horizontalPass(cells, cells);                                 // Horizontal pass for M2L, P2P
  downwardPass(cells);                                          // Downward pass for L2L, L2P
  stop("FMM DAG");                                              // Stop timer
  start("FMM Time");
  xitao_start();
  xitao_fini();
  stop("FMM Time");
#else
  start("FMM Time");
  //! FMM evaluation
  start("P2M & M2M");                                           // Start timer
  upwardPass(cells);                                            // Upward pass for P2M, M2M
  stop("P2M & M2M");                                            // Stop timer
  start("M2L & P2P");                                           // Start timer
  horizontalPass(cells, cells);                                 // Horizontal pass for M2L, P2P
  stop("M2L & P2P");                                            // Stop timer
  start("L2L & L2P");                                           // Start timer
  downwardPass(cells);                                          // Downward pass for L2L, L2P
  stop("L2L & L2P");                                            // Stop timer
  stop("FMM Time");
  
#endif  


  //! Direct N-Body
  start("Direct N-Body");                                       // Start timer
  const int numTargets = 10;                                    // Number of targets for checking answer
  Bodies jbodies = bodies;                                      // Save bodies in jbodies
  int stride = bodies.size() / numTargets;                      // Stride of sampling
  for (int b=0; b<numTargets; b++) {                            // Loop over target samples
    bodies[b] = bodies[b*stride];                               //  Sample targets
  }                                                             // End loop over target samples
  bodies.resize(numTargets);                                    // Resize bodies
  Bodies bodies2 = bodies;                                      // Backup bodies
  for (size_t b=0; b<bodies.size(); b++) {                      // Loop over bodies
    bodies[b].p = 0;                                            //  Clear potential
    for (int d=0; d<2; d++) bodies[b].F[d] = 0;                 //  Clear force
  }                                                             // End loop over bodies
  direct(bodies, jbodies);                                      // Direct N-Body
  stop("Direct N-Body");                                        // Stop timer

  //! Verify result
  double pDif = 0, pNrm = 0, FDif = 0, FNrm = 0;
  for (size_t b=0; b<bodies.size(); b++) {                      // Loop over bodies & bodies2
    pDif += (bodies[b].p - bodies2[b].p) * (bodies[b].p - bodies2[b].p);// Difference of potential
    pNrm += bodies2[b].p * bodies2[b].p;                        //  Value of potential
    FDif += (bodies[b].F[0] - bodies2[b].F[0]) * (bodies[b].F[0] - bodies2[b].F[0])// Difference of force
      + (bodies[b].F[0] - bodies2[b].F[0]) * (bodies[b].F[0] - bodies2[b].F[0]);// Difference of force
    FNrm += bodies2[b].F[0] * bodies2[b].F[0] + bodies2[b].F[1] * bodies2[b].F[1];//  Value of force
  }                                                             // End loop over bodies & bodies2
  printf("--- %-16s ------------\n", "FMM vs. direct");         // Print message
  printf("%-20s : %8.5e s\n","Rel. L2 Error (p)", sqrt(pDif/pNrm));// Print potential error
  printf("%-20s : %8.5e s\n","Rel. L2 Error (F)", sqrt(FDif/FNrm));// Print force error
#if USE_XITAO
  if(xitao::config::use_performance_modeling) {
//    xitao_ptt::print_table<FMM_TAO_1>("P2M", 0);
//    xitao_ptt::print_table<FMM_TAO_1>("M2M", 1);
    xitao_ptt::print_table<M2LListTAO>("M2L", 0);
    xitao_ptt::print_table<P2PListTAO>("P2P", 0);
//    xitao_ptt::print_table<FMM_TAO_1>("L2L", 2);
//    xitao_ptt::print_table<FMM_TAO_1>("L2P", 3);
  }
  std::cout << "Total number of steals: " <<  tao_total_steals << "\n";
#endif
  return 0;
}
