// evaluate.h

#pragma once

struct Evaluate {                                               // Functor for kernel evaluation
  Cell * C;                                                     // Cell to evaluate
  Evaluate(Cell * _C) : C(_C) {};                               // Constructor
  void operator() () const {                                          // Functor
    for (int i=0; i<C->M2L.size(); i++) M2L(C,C->M2L[i]);       //  M2L kernel
    if (C->NNODE == 1)                                          //  If leaf cell
      for (int i=0; i<C->P2P.size(); i++) P2P(C,C->P2P[i]);     //   P2P kernel
  }                                                             // End functor
};
