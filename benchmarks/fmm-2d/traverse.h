#ifndef traverse_lazy_h
#define traverse_lazy_h
#include "exafmm.h"
#if USE_XITAO
#include "hilbert.h"
#include "xitao.h"
#endif
namespace exafmm {
#ifdef USE_XITAO
   
  double hilbert_sta(real_t X[2]) { 
    const int locs = 2;
    constexpr float dx = 2 * M_PI / (1 << locs); 
    int x = floor((X[0] + M_PI) / dx);
    int y = floor((X[1] + M_PI) / dx);
    int key = xy2d(locs, x, y);
    double sta = double(key) / (1 << (2 * locs));
    return sta;
  } 

  class FMM_TAO_1 : public AssemblyTask {
    typedef std::function<void(Cell*)> fmm_func_type;
    Cell* cell; 
    fmm_func_type fmm_func;
  public:
    FMM_TAO_1(Cell*& _cell, fmm_func_type _fmm_func, size_t _workload_hint ) : 
                   AssemblyTask(1), cell(_cell), fmm_func(_fmm_func) {
      workload_hint = _workload_hint;
      set_sta(hilbert_sta(_cell->X));
    }
    void execute(int nthread) { 
      if(nthread == leader) { 
        fmm_func(cell);
      }
    }

    void cleanup() {} 
  };

  class FMM_TAO_2 : public AssemblyTask {
    typedef std::function<void(Cell*, Cell*)> fmm_func_type;
    Cell* cell_1; 
    Cell* cell_2; 
    fmm_func_type fmm_func;
  public:
    FMM_TAO_2(Cell*& _cell_1, Cell*& _cell_2, fmm_func_type _fmm_func, size_t _workload_hint ) : 
                   AssemblyTask(1), cell_1(_cell_1), cell_2(_cell_2), fmm_func(_fmm_func) { 
      workload_hint = _workload_hint;
    }

    void execute(int nthread) { 
      if(nthread == leader) { 
        fmm_func(cell_1, cell_2);
      }
    }
    
    void cleanup() {} 
  };

  class P2PListTAO : public AssemblyTask {
    Cell* cell_i; 
  public:
    P2PListTAO(Cell* _cell_i) : AssemblyTask(1), cell_i (_cell_i) {
      //criticality = 1;
      set_sta(hilbert_sta(cell_i->X));
    }

    void execute(int nthread) { 
	assert(width > 0);
	int tid = nthread - leader;
	int size = cell_i->NBODY / width; 
	int start = tid * size;
	int end   = (tid == width - 1)? cell_i->NBODY : start + size;
    //  if(nthread == leader) { 
        for (size_t j=0; j<cell_i->listP2P.size(); j++) {      //  Loop over P2P list
         // assert(start < end);
          P2P(cell_i,cell_i->listP2P[j], start, end);                      //   P2P kernel
   //       P2P(cell_i,cell_i->listP2P[j]);                      //   P2P kernel
        }   
      //}
    }
    
    void cleanup() {} 
  };

  class M2LListTAO : public AssemblyTask {
    Cell* cell_i; 
  public:
    M2LListTAO(Cell* _cell_i) : AssemblyTask(1), cell_i (_cell_i) { 
      set_sta(hilbert_sta(cell_i->X));
    }

    void execute(int nthread) { 
      if(nthread == leader) { 
        for (size_t j=0; j<cell_i->listM2L.size(); j++) {      //  Loop over P2P list
          M2L(cell_i,cell_i->listM2L[j]);                      //   M2L kernel
        }   
      }
    }
    
    void cleanup() {} 
  };
  std::vector<FMM_TAO_1*> M2M_map;
  std::vector<M2LListTAO*> M2L_map;
  std::vector<P2PListTAO*> P2P_map;

  FMM_TAO_1* upwardPass_xitao(Cell * Ci) {
    std::vector<FMM_TAO_1*> childTAOs;
    for (Cell * Cj=Ci->CHILD; Cj!=Ci->CHILD+Ci->NCHILD; Cj++) { // Loop over child cells
      childTAOs.push_back(upwardPass_xitao(Cj));                //  Recursive call for child cell
    }                                                           // End loop over child cells
    Ci->M.resize(P, 0.0);                                       // Allocate and initialize multipole coefs
    Ci->L.resize(P, 0.0);                                       // Allocate and initialize local coefs
    FMM_TAO_1* M2M_tao = new FMM_TAO_1(Ci, M2M, 1);
    if (Ci->NCHILD == 0) { 
      FMM_TAO_1* P2M_tao = new FMM_TAO_1(Ci, P2M, 0);
      P2M_tao->make_edge(M2M_tao);
      xitao_push(P2M_tao, rand() % xitao_nthreads);
    } else {
       for(auto&& child : childTAOs)
         child->make_edge(M2M_tao);
    }
    M2M_map[Ci->INDEX] = M2M_tao;
    return M2M_tao;
  }

  void upwardPass(Cells & cells) {
    M2M_map.resize(cells.size(), NULL);
    M2L_map.resize(cells.size(), NULL);
    P2P_map.resize(cells.size(), NULL);
    auto val = upwardPass_xitao(&cells[0]);
    if(val == NULL) {
      cout << "Empty upward pass DAG" << endl;
    }
  }

  //! Recursive call to dual tree traversal for list construction
  void getList(Cell * Ci, Cell * Cj) {
    for (int d=0; d<2; d++) dX[d] = Ci->X[d] - Cj->X[d];        // Distance vector from source to target
    real_t R2 = norm(dX) * theta * theta;                       // Scalar distance squared
    if (R2 > (Ci->R + Cj->R) * (Ci->R + Cj->R)) {               // If distance is far enough
      Ci->listM2L.push_back(Cj);                                //  Add to M2L list
    } else if (Ci->NCHILD == 0 && Cj->NCHILD == 0) {            // Else if both cells are leafs
      Ci->listP2P.push_back(Cj);                                //  Add to P2P list
    } else if (Cj->NCHILD == 0 || (Ci->R >= Cj->R && Ci->NCHILD != 0)) {// Else if Cj is leaf or Ci is larger
      for (Cell * ci=Ci->CHILD; ci!=Ci->CHILD+Ci->NCHILD; ci++) {// Loop over Ci's children
        getList(ci, Cj);                                        //   Recursive call to target child cells
      }                                                         //  End loop over Ci's children
    } else {                                                    // Else if Ci is leaf or Cj is larger
      for (Cell * cj=Cj->CHILD; cj!=Cj->CHILD+Cj->NCHILD; cj++) {//  Loop over Cj's children
        getList(Ci, cj);                                        //   Recursive call to source child cells
      }                                                         //  End loop over Cj's children
    }                                                           // End if for leafs and Ci Cj size
  }

  void buildHorizonalDAG(Cells & cells) {
    for (size_t i=0; i<cells.size(); i++) {                     // Loop over cells
      if(cells[i].listM2L.size() > 0) {
        M2LListTAO* M2L_tao = new M2LListTAO(&cells[i]);                        //   M2L kernel
        M2L_map[cells[i].INDEX] = M2L_tao;
         for (size_t j=0; j<cells[i].listM2L.size(); j++) {        //  Loop over M2L list
           M2M_map[cells[i].listM2L[j]->INDEX]->make_edge(M2L_tao);
         }  
      }
      if(cells[i].listP2P.size() > 0) {
        P2PListTAO* P2P_tao = new P2PListTAO(&cells[i]);                        //   P2P kernel
        P2P_map[cells[i].INDEX] = P2P_tao;
        xitao_push(P2P_tao, rand() % xitao_nthreads);
      }
    } 
  }

  //! Horizontal pass interface
  void horizontalPass(Cells & icells, Cells & jcells) {
    getList(&icells[0], &jcells[0]);                            // Pass root cell to recursive call
    buildHorizonalDAG(icells);
  }
  //! Recursive call to pre-order traversal for downward pass
  void downwardPass(Cell * Cj, FMM_TAO_1* parent) {
    FMM_TAO_1* L2L_tao = new FMM_TAO_1(Cj, L2L, 2);
    if(M2L_map[Cj->INDEX] != NULL){
      M2L_map[Cj->INDEX]->make_edge(L2L_tao);      
    } 
    for (Cell * Ci=Cj->CHILD; Ci!=Cj->CHILD+Cj->NCHILD; Ci++) { // Loop over child cells
      if(M2L_map[Ci->INDEX] != NULL){
	M2L_map[Ci->INDEX]->make_edge(L2L_tao);                             // L2L kernel
      }
    }
    if(parent) { 
      parent->make_edge(L2L_tao);
    } else {
      xitao_push(L2L_tao, rand() % xitao_nthreads);
    }
    if (Cj->NCHILD == 0) {
      FMM_TAO_1* L2P_tao = new FMM_TAO_1(Cj, L2P, 3);
      P2P_map[Cj->INDEX]->make_edge(L2P_tao);
      L2L_tao->make_edge(L2P_tao);
    }                               // L2P kernel
    for (Cell * Ci=Cj->CHILD; Ci!=Cj->CHILD+Cj->NCHILD; Ci++) { // Loop over child cells
      downwardPass(Ci, L2L_tao);                                 //  Recursive call for child cell
    }                                                           // End loop over child cells
  }

  //! Downward pass interface
  void downwardPass(Cells & cells) {
    downwardPass(&cells[0], NULL);                                    // Pass root cell to recursive call
  }
#else
  //! Recursive call to post-order tree traversal for upward pass
  void upwardPass(Cell * Ci) {
    for (Cell * Cj=Ci->CHILD; Cj!=Ci->CHILD+Ci->NCHILD; Cj++) { // Loop over child cells
#pragma omp task untied if(Cj->NBODY > 100)                     //  Start OpenMP task if large enough task
      upwardPass(Cj);                                           //  Recursive call for child cell
    }                                                           // End loop over child cells
#pragma omp taskwait                                            // Synchronize OpenMP tasks
    Ci->M.resize(P, 0.0);                                       // Allocate and initialize multipole coefs
    Ci->L.resize(P, 0.0);                                       // Allocate and initialize local coefs
    if (Ci->NCHILD == 0) P2M(Ci);                               // P2M kernel
    M2M(Ci);                                                    // M2M kernel
  }

//! Upward pass interface
  void upwardPass(Cells & cells) {
#pragma omp parallel                                            // Start OpenMP
#pragma omp single nowait                                       // Start OpenMP single region with nowait
    upwardPass(&cells[0]);                                      // Pass root cell to recursive call
  }
  //! Recursive call to dual tree traversal for list construction
  void getList(Cell * Ci, Cell * Cj) {
    for (int d=0; d<2; d++) dX[d] = Ci->X[d] - Cj->X[d];        // Distance vector from source to target
    real_t R2 = norm(dX) * theta * theta;                       // Scalar distance squared
    if (R2 > (Ci->R + Cj->R) * (Ci->R + Cj->R)) {               // If distance is far enough
      Ci->listM2L.push_back(Cj);                                //  Add to M2L list
    } else if (Ci->NCHILD == 0 && Cj->NCHILD == 0) {            // Else if both cells are leafs
      Ci->listP2P.push_back(Cj);                                //  Add to P2P list
    } else if (Cj->NCHILD == 0 || (Ci->R >= Cj->R && Ci->NCHILD != 0)) {// Else if Cj is leaf or Ci is larger
      for (Cell * ci=Ci->CHILD; ci!=Ci->CHILD+Ci->NCHILD; ci++) {// Loop over Ci's children
        getList(ci, Cj);                                        //   Recursive call to target child cells
      }                                                         //  End loop over Ci's children
    } else {                                                    // Else if Ci is leaf or Cj is larger
      for (Cell * cj=Cj->CHILD; cj!=Cj->CHILD+Cj->NCHILD; cj++) {//  Loop over Cj's children
        getList(Ci, cj);                                        //   Recursive call to source child cells
      }                                                         //  End loop over Cj's children
    }                                                           // End if for leafs and Ci Cj size
  }

  //! Evaluate M2L, P2P kernels
  void evaluate(Cells & cells) {
#pragma omp parallel for schedule(dynamic)
    for (size_t i=0; i<cells.size(); i++) {                     // Loop over cells
      for (size_t j=0; j<cells[i].listM2L.size(); j++) {        //  Loop over M2L list
        M2L(&cells[i],cells[i].listM2L[j]);                     //   M2L kernel
      }                                                         //  End loop over M2L list
      for (size_t j=0; j<cells[i].listP2P.size(); j++) {        //  Loop over P2P list
        P2P(&cells[i],cells[i].listP2P[j]);                     //   P2P kernel
      }                                                         //  End loop over P2P list
    }                                                           // End loop over cells
  }

  //! Horizontal pass interface
  void horizontalPass(Cells & icells, Cells & jcells) {
    getList(&icells[0], &jcells[0]);                            // Pass root cell to recursive call
    evaluate(icells);                                           // Evaluate M2L & P2P kernels
  }

  //! Recursive call to pre-order traversal for downward pass
  void downwardPass(Cell * Cj) {
    L2L(Cj);                                                    // L2L kernel
    if (Cj->NCHILD == 0) L2P(Cj);                               // L2P kernel
    for (Cell * Ci=Cj->CHILD; Ci!=Cj->CHILD+Cj->NCHILD; Ci++) { // Loop over child cells
#pragma omp task untied if(Ci->NBODY > 100)                     //  Start OpenMP task if large enough task
      downwardPass(Ci);                                         //  Recursive call for child cell
    }                                                           // End loop over child cells
#pragma omp taskwait                                            // Synchronize OpenMP tasks
  }

  //! Downward pass interface
  void downwardPass(Cells & cells) {
#pragma omp parallel                                            // Start OpenMP
#pragma omp single nowait                                       // Start OpenMP single region with nowait
    downwardPass(&cells[0]);                                    // Pass root cell to recursive call
  }
#endif
  //! Direct summation
  void direct(Bodies & bodies, Bodies & jbodies) {
    Cells cells(2);                                             // Define a pair of cells to pass to P2P kernel
    Cell * Ci = &cells[0];                                      // Allocate single target cell
    Cell * Cj = &cells[1];                                      // Allocate single source cell
    Ci->BODY = &bodies[0];                                      // Pointer of first target body
    Ci->NBODY = bodies.size();                                  // Number of target bodies
    Cj->BODY = &jbodies[0];                                     // Pointer of first source body
    Cj->NBODY = jbodies.size();                                 // Number of source bodies
    P2P(Ci, Cj);                                                // Evaluate P2P kernel
  }
}

#endif
