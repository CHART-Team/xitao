#ifndef traversal_h
#define traversal_h

real_t theta;                                                   //!< Multipole acceptance criterion

void traversal(Cell * Ci, Cell * Cj);

//! Split cell and call dualTreeTraversal() recursively for child
void splitCell(Cell * Ci, Cell * Cj) {
  if (Cj->NNODE == 1) {                                         // If Cj is leaf
    for (int i=0; i<4; i++) {                                   //  Loop over Ci's children
      if (Ci->CHILD[i]) traversal(Ci->CHILD[i], Cj);            //   Traverse a single pair of cells
    }                                                           //  End loop over Ci's children
  } else if (Ci->NNODE == 1) {                                  // Else if Ci is leaf
    for (int i=0; i<4; i++) {                                   //  Loop over Cj's children
      if (Cj->CHILD[i]) traversal(Ci, Cj->CHILD[i]);            //   Traverse a single pair of cells
    }                                                           //  End loop over Cj's children
  } else if (Ci->R >= Cj->R) {                                  // Else if Ci is larger than Cj
    for (int i=0; i<4; i++) {                                   //  Loop over Ci's children
      if (Ci->CHILD[i]) traversal(Ci->CHILD[i], Cj);            //   Traverse a single pair of cells
    }                                                           //  End loop over Ci's children
  } else {                                                      // Else if Cj is larger than Ci
    for (int i=0; i<4; i++) {                                   //  Loop over Cj's children
      if (Cj->CHILD[i]) traversal(Ci, Cj->CHILD[i]);            //   Traverse a single pair of cells
    }                                                           //  End loop over Cj's children
  }                                                             // End if for leafs and Ci Cj size
}

//! Dual tree traversal for a single pair of cells
void traversal(Cell * Ci, Cell * Cj) {
  real_t dX[2];                                                 // Distance vector
  for (int d=0; d<2; d++) dX[d] = Ci->X[d] - Cj->X[d];          // Distance vector from source to target
  real_t R2 = (dX[0] * dX[0] + dX[1] * dX[1]) * theta * theta;  // Scalar distance squared
  if (R2 > (Ci->R + Cj->R) * (Ci->R + Cj->R)) {                 // If distance is far enough
#ifdef LIST
    Ci->M2L.push_back(Cj);                                      //  Queue M2L kernels
#else
    M2L(Ci, Cj);                                                //  Use approximate kernels
#endif
  } else if (Ci->NNODE == 1 && Cj->NNODE == 1) {                // Else if both cells are bodies
#ifdef LIST
    Ci->P2P.push_back(Cj);                                      //  Queue P2P kernels
#else
    P2P(Ci, Cj);                                                //  Use exact kernel
#endif
  } else {                                                      // Else if cells are close but not bodies
    splitCell(Ci, Cj);                                          //  Split cell and call function recursively for child
  }                                                             // End if for multipole acceptance
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

//! Recursive call for downward pass
void downwardPass(Cell * C) {
#ifdef LIST
  for (int i=0; i<C->M2L.size(); i++) M2L(C,C->M2L[i]);         // M2L kernel
#endif
  L2L(C);                                                       // L2L kernel
  if (C->NNODE == 1) {
#ifdef LIST
    for (int i=0; i<C->P2P.size(); i++) P2P(C,C->P2P[i]);       // P2P kernel
#endif
    L2P(C);                                                     // L2P kernel
  }
  for (int i=0; i<4; i++) {                                     // Loop over child cells
    if (C->CHILD[i]) downwardPass(C->CHILD[i]);                 //  Recursive call with new task
  }                                                             // End loop over child cells
}
  
//! Direct summation
void direct(int ni, Body * ibodies, int nj, Body * jbodies) {
  Cell * Ci = new Cell();                                       // Allocate single target cell
  Cell * Cj = new Cell();                                       // Allocate single source cell
  Ci->BODY = ibodies;                                           // Pointer of first target body
  Ci->NBODY = ni;                                               // Number of target bodies
  Cj->BODY = jbodies;                                           // Pointer of first source body
  Cj->NBODY = nj;                                               // Number of source bodies
  P2P(Ci, Cj);                                                  // Evaluate P2P kernel
}
#endif
