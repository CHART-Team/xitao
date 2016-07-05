#ifndef buildtree_h
#define buildtree_h

//! Build cells of tree adaptively using a top-down approach based on recursion (uses task based thread parallelism)
Cell * buildTree(Body * bodies, Body * buffer, int begin, int end,
		 real_t X[2], real_t R, int ncrit, int level=0, bool direction=false) {
  if (begin == end) return NULL;                              // If no bodies left, return null pointer
  //! Create a tree cell
  Cell * cell = new Cell();                                   // Allocate memory for single cell
  cell->BODY = bodies + begin;                                // Pointer of first body in cell
  if(direction) cell->BODY = buffer + begin;                  // If bodies is in buffer, point to it
  cell->NBODY = end - begin;                                  // Number of bodies in cell
  cell->NNODE = 1;                                            // Initialize counter for decendant cells
  for (int d=0; d<2; d++) cell->X[d] = X[d];                  // Center position of cell
  cell->R = R / (1 << level);                                 // Cell radius
  for (int i=0; i<4; i++) cell->CHILD[i] = NULL;              // Initialize pointers to children
  //! If cell is a leaf
  if (end - begin <= ncrit) {                                 // If number of bodies is less than threshold
    if (direction) {                                          //  If direction of data is from bodies to buffer
      for (int i=begin; i<end; i++) {                         //   Loop over bodies in cell
	for (int d=0; d<2; d++) buffer[i].X[d] = bodies[i].X[d];//  Copy bodies coordinates to buffer
	buffer[i].q = bodies[i].q;                            //    Copy bodies source to buffer
      }                                                       //   End loop over bodies in cell
    }                                                         //  End if for direction of data
    return cell;                                              //  Return cell pointer
  }                                                           // End if for number of bodies
  //! Count number of bodies in each quadrant 
  int size[4] = {0,0,0,0};
  real_t x[2];                                                // Coordinates of bodies 
  for (int i=begin; i<end; i++) {                             // Loop over bodies in cell
    for (int d=0; d<2; d++) x[d] = bodies[i].X[d];            //  Position of body
    int quadrant = (x[0] > X[0]) + ((x[1] > X[1]) << 1);      //  Which quadrant body belongs to
    size[quadrant]++;                                         //  Increment body count in quadrant
  }                                                           // End loop over bodies in cell
  //! Exclusive scan to get offsets
  int offset = begin;                                         // Offset of first quadrant
  int offsets[4], counter[4];                                 // Offsets and counter for each quadrant
  for (int i=0; i<4; i++) {                                   // Loop over elements
    offsets[i] = offset;                                      //  Set value
    offset += size[i];                                        //  Increment offset
  }                                                           // End loop over elements
  //! Sort bodies by quadrant
  for (int i=0; i<4; i++) counter[i] = offsets[i];            // Copy offsets yo counter
  for (int i=begin; i<end; i++) {                             // Loop over bodies
    for (int d=0; d<2; d++) x[d] = bodies[i].X[d];            //  Position of body
    int quadrant = (x[0] > X[0]) + ((x[1] > X[1]) << 1);      //  Which quadrant body belongs to`
    for (int d=0; d<2; d++) buffer[counter[quadrant]].X[d] = bodies[i].X[d];// Permute bodies coordinates out-of-place according to quadrant
    buffer[counter[quadrant]].q = bodies[i].q;                //  Permute bodies sources out-of-place according to quadrant  
    counter[quadrant]++;                                      //  Increment body count in quadrant
  }                                                           // End loop over bodies
  //! Loop over children and recurse
  real_t Xchild[2];                                           // Coordinates of children
  for (int i=0; i<4; i++) {                                   // Loop over children
    for (int d=0; d<2; d++) Xchild[d] = X[d];                 //  Initialize center position of child cell
    real_t r = R / (1 << (level + 1));                        //  Radius of cells for child's level
    for (int d=0; d<2; d++) {                                 //  Loop over dimensions
      Xchild[d] += r * (((i & 1 << d) >> d) * 2 - 1);         //   Shift center position to that of child cell
    }                                                         //  End loop over dimensions
    cell->CHILD[i] = buildTree(buffer, bodies,                //  Recursive call for each child
			       offsets[i], offsets[i] + size[i],//  Range of bodies is calcuated from quadrant offset
			       Xchild, R, ncrit, level+1, !direction);//  Alternate copy direction bodies <-> buffer
  }                                                           // End loop over children
  //! Accumulate number of decendant cells
  for (int i=0; i<4; i++) {                                   // Loop over children
    if (cell->CHILD[i]) {                                     //  If child exists
      cell->NNODE += cell->CHILD[i]->NNODE;                   //   Increment child cell counter
    }                                                         //  End if for child
  }                                                           // End loop over chlidren
  return cell;                                                // Return quadtree cell
}

#endif
