#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>

#include <tbb/parallel_for.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>

#ifdef DO_LOI
extern "C"
{
#include "loi.h"
}
#endif

int numWorkers = 32;
int awidth;

#include "types.h"
#include "buildtree.h"
#include "kernel.h"
#include "traversal.h"


//! Get the current time in seconds
double getTime() {
  struct timeval tv;                                            // Time value
  gettimeofday(&tv, NULL);                                      // Get time of day in seconds and microseconds
  return double(tv.tv_sec+tv.tv_usec*1e-6);                     // Combine seconds and microseconds and return
}

int main(int argc, char ** argv) {                              // Main function
  const int numBodies = 1000000;                                 // Number of bodies
  const int numTargets = 10;                                    // Number of targets for checking answer
  const int ncrit = 20;                                          // Number of bodies per leaf cell
  theta = 0.2;                                                  // Multipole acceptance criterion
  nspawn = 1000;                                                // Threshold for spawning threads

  if(argc == 2) 
	awidth = numWorkers = atoi(argv[1]);

  else if(argc == 3){  
	numWorkers = atoi(argv[1]);
	awidth = atoi(argv[2]);
	}

  else awidth= numWorkers;

  tbb::task_scheduler_init init(numWorkers);                    // Number of worker threads

#if DO_LOI
    loi_init(); // calc TSC freq and init data structures
    printf(" TSC frequency has been measured to be: %g Hz\n", (double) TSCFREQ);
    int maxthr = numWorkers;
#endif


  //! Initialize dsitribution, source & target value of bodies
  printf("--- FMM Profiling ----------------\n");               // Start profiling
  double time = getTime();                                      // Start timer
  srand48(0);                                                   // Set seed for random number generator
  Body * bodies = new Body [numBodies];                         // Initialize bodies
  real_t average = 0;                                           // Average charge
  for (int b=0; b<numBodies; b++) {                             // Loop over bodies
    for (int d=0; d<2; d++) {                                   //  Loop over dimension
      bodies[b].X[d] = drand48() * 2 * M_PI - M_PI;             //   Initialize positions
    }                                                           //  End loop over dimension
    bodies[b].q = drand48() - .5;                               //  Initialize charge
    average += bodies[b].q;                                     //  Accumulate charge
    bodies[b].p = 0;                                            //  Clear potential
    for (int d=0; d<2; d++) bodies[b].F[d] = 0;                 //  Clear force
  }                                                             // End loop over bodies
  average /= numBodies;                                         // Average charge
  for (int b=0; b<numBodies; b++) {                             // Loop over bodies
    bodies[b].q -= average;                                     // Charge neutral
  }                                                             // End loop over bodies
  printf("%-20s : %lf s\n","Init bodies",getTime()-time);       // Stop timer

  // ! Get Xmin and Xmax of domain
  time = getTime();                                             // Start timer 
  real_t R0;                                                    // Radius of root cell
  real_t Xmin[2], Xmax[2], X0[2];                               // Min, max of domain, and center of root cell
  for (int d=0; d<2; d++) Xmin[d] = Xmax[d] = bodies[0].X[d];   // Initialize Xmin, Xmax
  for (int b=0; b<numBodies; b++) {                             // Loop over range of bodies
    for (int d=0; d<2; d++) Xmin[d] = fmin(bodies[b].X[d], Xmin[d]);//  Update Xmin
    for (int d=0; d<2; d++) Xmax[d] = fmax(bodies[b].X[d], Xmax[d]);//  Update Xmax
  }                                                             // End loop over range of bodies
  for (int d=0; d<2; d++) X0[d] = (Xmax[d] + Xmin[d]) / 2;      // Calculate center of domain
  R0 = 0;                                                       // Initialize localRadius
  for (int d=0; d<2; d++) {                                     // Loop over dimensions
    R0 = fmax(X0[d] - Xmin[d], R0);                             //  Calculate min distance from center
    R0 = fmax(Xmax[d] - X0[d], R0);                             //  Calculate max distance from center
  }                                                             // End loop over dimensions
  R0 *= 1.00001;                                                // Add some leeway to radius
  printf("%-20s : %lf s\n","Get bounds",getTime()-time);        // Stop timer 

  //! Build tree structure
  time = getTime();                                             // Start timer 
  Body * buffer = new Body [numBodies];                         // Buffer for bodies
  for (int b=0; b<numBodies; b++) buffer[b] = bodies[b];        // Copy bodies to buffer
  Cell * C0 = buildTree(bodies, buffer, 0, numBodies, X0, R0, ncrit);// Build tree recursively
  printf("%-20s : %lf s\n","Grow tree",getTime()-time);         // Stop timer 

  //! FMM evaluation
  time = getTime();                                             // Start timer 
  upwardPass(C0);                                               // Upward pass for P2M, M2M
  printf("%-20s : %lf s\n","Upward pass",getTime()-time);       // Stop timer 
  time = getTime();                                             // Start timer 
#if !defined(LIST) && defined(DO_LOI)
  phase_profile_start();
#endif
  Traversal traversal(C0, C0);                                  // Traversal for M2L, P2P
  traversal();
#if !defined(LIST) && defined(DO_LOI)
  phase_profile_stop(0);
#endif

#ifdef LIST
#ifdef DO_LOI 
  phase_profile_start();
#endif
#ifdef TAO
  gotao_init_hw(numWorkers,-1,-1);
#endif
  breadthFirst(C0);                                             // Traversal for M2L, P2P
#ifdef DO_LOI
  phase_profile_stop(0); 
#endif
#endif
  printf("%-20s : %lf s\n","Traverse",getTime()-time);          // Stop timer 
  time = getTime();                                             // Start timer 
  downwardPass(C0);                                             // Downward pass for L2L, L2P
  printf("%-20s : %lf s\n","Downward pass",getTime()-time);     // Stop timer 

  //! Downsize target bodies by even sampling 
  Body * jbodies = new Body [numBodies];                        // Source bodies
  for (int b=0; b<numBodies; b++) jbodies[b] = bodies[b];       // Save bodies in jbodies
  int stride = numBodies / numTargets;                          // Stride of sampling
  for (int b=0; b<numTargets; b++) {                            // Loop over target samples
    bodies[b] = bodies[b*stride];                               //  Sample targets
  }                                                             // End loop over target samples
  Body * bodies2 = new Body [numTargets];                       // Backup bodies
  for (int b=0; b<numTargets; b++) {                            // Loop over bodies
    bodies2[b] = bodies[b];                                     //  Save bodies in bodies2
  }                                                             // End loop over bodies
  for (int b=0; b<numTargets; b++) {                            // Loop over bodies
    bodies[b].p = 0;                                            //  Clear potential
    for (int d=0; d<2; d++) bodies[b].F[d] = 0;                 //  Clear force
  }                                                             // End loop over bodies
  time = getTime();                                             // Start timer 
  //direct(numTargets, bodies, numBodies, jbodies);               // Direc N-body
  printf("%-20s : %lf s\n","Direct N-Body",getTime()-time);     // Stop timer 

  //! Evaluate relaitve L2 norm error
  double dp2 = 0, p2 = 0, df2 = 0, f2 = 0;
  for (int b=0; b<numTargets; b++) {                            // Loop over bodies & bodies2
    dp2 += (bodies[b].p - bodies2[b].p) * (bodies[b].p - bodies2[b].p);// Difference of potential
    p2 += bodies2[b].p * bodies2[b].p;                          //  Value of potential
    df2 += (bodies[b].F[0] - bodies2[b].F[0]) * (bodies[b].F[0] - bodies2[b].F[0])// Difference of force
      + (bodies[b].F[0] - bodies2[b].F[0]) * (bodies[b].F[0] - bodies2[b].F[0]);// Difference of force
    f2 += bodies2[b].F[0] * bodies2[b].F[0] + bodies2[b].F[1] * bodies2[b].F[1];//  Value of force
  }                                                             // End loop over bodies & bodies2
  printf("--- FMM vs. direct ---------------\n");               // Print message
  printf("Rel. L2 Error (p)  : %e\n",sqrtf(dp2/p2));            // Print potential error
  printf("Rel. L2 Error (f)  : %e\n",sqrtf(df2/f2));            // Print force error
#ifdef TAO
  printf("TAO total steals: %d\n", tao_total_steals);
#endif
#ifdef DO_LOI
#ifdef LOI_TIMING
    loi_statistics(&fmm_kernels, maxthr);
#endif
#ifdef DO_KRD
    krd_save_traces();
#endif
#endif
  return 0;
}
