#ifndef types_h
#define types_h
#include <complex>
#include <vector>

// Basic type definitions
typedef float real_t;                                           //!< Floating point type is single precision
typedef std::complex<real_t> complex_t;                         //!< Complex type

// Multipole/local expansion coefficients
const int P = 6;                                                //!< Order of expansions

//! Structure of bodies
struct Body {
  real_t X[2];                                                  //!< Position
  real_t q;                                                     //!< Charge
  real_t p;                                                     //!< Potential
  real_t F[2];                                                  //!< Force
};

//! Structure of cells
struct Cell {
  int NNODE;                                                    //!< Number of child cells
  int NBODY;                                                    //!< Number of descendant bodies
  Cell * CHILD[4];                                              //!< Pointer of child cells
  Body * BODY;                                                  //!< Pointer of first body
  real_t X[2];                                                  //!< Cell center
  real_t R;                                                     //!< Cell radius
  complex_t M[P];                                               //!< Multipole expansion coefficients
  complex_t L[P];                                               //!< Local expansion coefficients
#ifdef LIST
  std::vector<Cell*> M2L;                                       //!< M2L interaction list
  std::vector<Cell*> P2P;                                       //!< P2P interaction list
#endif
};

#endif
