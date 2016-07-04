#ifndef traversal_h
#define traversal_h

int images;                                                     //!< Number of periodic image sublevels
real_t theta;                                                   //!< Multipole acceptance criterion
real_t Xperiodic[2];                                            //!< Periodic coordinate offset

void dualTreeTraversal(Cell * Ci, Cell * Cj);

//! Split cell and call dualTreeTraversal() recursively for child
void splitCell(Cell * Ci, Cell * Cj) {
  if (Cj->NNODE == 1) {                                         // If Cj is leaf
    for (int i=0; i<4; i++) {                                   //  Loop over Ci's children
      if (Ci->CHILD[i]) dualTreeTraversal(Ci->CHILD[i], Cj);    //   Traverse a single pair of cells
    }                                                           //  End loop over Ci's children
  } else if (Ci->NNODE == 1) {                                  // Else if Ci is leaf
    for (int i=0; i<4; i++) {                                   //  Loop over Cj's children
      if (Cj->CHILD[i]) dualTreeTraversal(Ci, Cj->CHILD[i]);    //   Traverse a single pair of cells
    }                                                           //  End loop over Cj's children
  } else if (Ci->R >= Cj->R) {                                  // Else if Ci is larger than Cj
    for (int i=0; i<4; i++) {                                   //  Loop over Ci's children
      if (Ci->CHILD[i]) dualTreeTraversal(Ci->CHILD[i], Cj);    //   Traverse a single pair of cells
    }                                                           //  End loop over Ci's children
  } else {                                                      // Else if Cj is larger than Ci
    for (int i=0; i<4; i++) {                                   //  Loop over Cj's children
      if (Cj->CHILD[i]) dualTreeTraversal(Ci, Cj->CHILD[i]);    //   Traverse a single pair of cells
    }                                                           //  End loop over Cj's children
  }                                                             // End if for leafs and Ci Cj size
}

//! Dual tree traversal for a single pair of cells
void dualTreeTraversal(Cell * Ci, Cell * Cj) {
  real_t dX[2];                                                 // Distance vector
  for (int d=0; d<2; d++) dX[d] = Ci->X[d] - Cj->X[d] - Xperiodic[d];// Distance vector from source to target
  real_t R2 = (dX[0] * dX[0] + dX[1] * dX[1]) * theta * theta;  // Scalar distance squared
  if (R2 > (Ci->R + Cj->R) * (Ci->R + Cj->R)) {                 // If distance is far enough
    M2L(Ci, Cj, Xperiodic);                                     //  Use approximate kernels
  } else if (Ci->NNODE == 1 && Cj->NNODE == 1) {                // Else if both cells are bodies
    P2P(Ci, Cj, Xperiodic);                                     //   Use exact kernel
  } else {                                                      // Else if cells are close but not bodies
    splitCell(Ci, Cj);                                          //  Split cell and call function recursively for child
  }                                                             // End if for multipole acceptance
}


//! Tree traversal of periodic cells
void traversePeriodic(Cell * Ci0, Cell * Cj0, real_t cycle) {
  for (int d=0; d<2; d++) Xperiodic[d] = 0;                     // Periodic coordinate offset
  Cell * Cp = new Cell();                                       // Last cell is periodic parent cell
  Cell * Cj = new Cell();                                       // Last cell is periodic parent cell
  *Cp = *Cj = *Cj0;                                             // Copy values from source root
  Cp->CHILD[0] = Cj;                                            // Child cells for periodic center cell
  for (int i=1; i<4; i++) Cp->CHILD[i] = NULL;                  // Define only one child
  for (int level=0; level<images-1; level++) {                  // Loop over sublevels of tree
    for (int ix=-1; ix<=1; ix++) {                              //  Loop over x periodic direction
      for (int iy=-1; iy<=1; iy++) {                            //   Loop over y periodic direction
	if (ix != 0 || iy != 0) {                               //    If periodic cell is not at center
	  for (int cx=-1; cx<=1; cx++) {                        //     Loop over x periodic direction (child)
	    for (int cy=-1; cy<=1; cy++) {                      //      Loop over y periodic direction (child)
	      Xperiodic[0] = (ix * 3 + cx) * cycle;             //       Coordinate offset for x periodic direction
	      Xperiodic[1] = (iy * 3 + cy) * cycle;             //       Coordinate offset for y periodic direction
	      M2L(Ci0, Cp, Xperiodic);                          //       Perform M2L kernel
	    }                                                   //      End loop over y periodic direction (child)
	  }                                                     //     End loop over x periodic direction (child)
	}                                                       //    Endif for periodic center cell
      }                                                         //   End loop over y periodic direction
    }                                                           //  End loop over x periodic direction
    complex_t M[P];                                             //  Multipole expansions
    for (int n=0; n<P; n++) {                                   //  Loop over order of expansions
      M[n] = Cp->M[n];                                          //   Save multipoles of periodic parent
      Cp->M[n] = 0;                                             //   Reset multipoles of periodic parent
    }                                                           //  End loop over order of expansions
    for (int ix=-1; ix<=1; ix++) {                              //  Loop over x periodic direction
      for (int iy=-1; iy<=1; iy++) {                            //   Loop over y periodic direction
	if( ix != 0 || iy != 0) {                               //    If periodic cell is not at center
	  Cj->X[0] = Cp->X[0] + ix * cycle;                     //     Set new x coordinate for periodic image
	  Cj->X[1] = Cp->X[1] + iy * cycle;                     //     Set new y cooridnate for periodic image
	  for (int n=0; n<P; n++) Cj->M[n] = M[n];              //     Copy multipoles to new periodic image
	  M2M(Cp);                                              //     Evaluate periodic M2M kernels for this sublevel
	}                                                       //    Endif for periodic center cell
      }                                                         //   End loop over y periodic direction
    }                                                           //  End loop over x periodic direction
    cycle *= 3;                                                 //  Increase center cell size three times
  }                                                             // End loop over sublevels of tree
}

//! Recursive call for upward pass 
void upwardPass(Cell * C) {
  for (int i=0; i<4; i++) {                                     // Loop over child cells
    if (C->CHILD[i]) upwardPass(C->CHILD[i]);                   //  Recursive call with new task
  }                                                             // End loop over child cells
  for (int n=0; n<P; n++) C->M[n] = 0;                          // Initialize multipole expansion coefficients
  for (int n=0; n<P; n++) C->L[n] = 0;                          // Initialize local expansion coefficients
  if (C->NNODE == 1) P2M(C);                                    // P2M kernel
  M2M(C);                                                       // M2M kernel
}

//! Evaluate P2P and M2L using dual tree traversal
void traversal(Cell * Ci0, Cell * Cj0, real_t cycle) {
  if (images == 0) {                                            // If non-periodic boundary condition
    for (int d=0; d<2; d++) Xperiodic[d] = 0;                   //  No periodic shift
    dualTreeTraversal(Ci0, Cj0);                                //  Traverse the tree
  } else {                                                      // If periodic boundary condition
    for (int ix=-1; ix<=1; ix++) {                              //  Loop over x periodic direction
      for (int iy=-1; iy<=1; iy++) {                            //   Loop over y periodic direction
	Xperiodic[0] = ix * cycle;                              //    Coordinate shift for x periodic direction
	Xperiodic[1] = iy * cycle;                              //    Coordinate shift for y periodic direction
	dualTreeTraversal(Ci0, Cj0);                            //    Traverse the tree for this periodic image
      }                                                         //   End loop over y periodic direction
    }                                                           //  End loop over x periodic direction
    traversePeriodic(Ci0, Cj0, cycle);                          //  Traverse tree for periodic images
  }                                                             // End if for periodic boundary condition
}                                                               // End if for empty cell vectors

//! Recursive call for downward pass
void downwardPass(Cell * C) {
  L2L(C);                                                       // L2L kernel
  if (C->NNODE == 1) L2P(C);                                    // L2P kernel
  for (int i=0; i<4; i++) {                                     // Loop over child cells
    if (C->CHILD[i]) downwardPass(C->CHILD[i]);                 //  Recursive call with new task
  }                                                             // End loop over child cells
}
  
//! Direct summation
void direct(int ni, Body * ibodies, int nj, Body * jbodies, real_t cycle) {
  Cell * Ci = new Cell();                                       // Allocate single target cell
  Cell * Cj = new Cell();                                       // Allocate single source cell
  Ci->BODY = ibodies;                                           // Pointer of first target body
  Ci->NBODY = ni;                                               // Number of target bodies
  Cj->BODY = jbodies;                                           // Pointer of first source body
  Cj->NBODY = nj;                                               // Number of source bodies
  int prange = 0;                                               // Range of periodic images
  for (int i=0; i<images; i++) {                                // Loop over periodic image sublevels
    prange += int(powf(3.,i));                                  //  Accumulate range of periodic images
  }                                                             // End loop over perioidc image sublevels
  for (int ix=-prange; ix<=prange; ix++) {                      // Loop over x periodic direction
    for (int iy=-prange; iy<=prange; iy++) {                    //  Loop over y periodic direction
      Xperiodic[0] = ix * cycle;                                //   Coordinate shift for x periodic direction
      Xperiodic[1] = iy * cycle;                                //   Coordinate shift for y periodic direction
      P2P(Ci, Cj, Xperiodic);                                   //   Evaluate P2P kernel
    }                                                           //  End loop over y periodic direction
  }                                                             // End loop over x periodic direction
}
#endif
