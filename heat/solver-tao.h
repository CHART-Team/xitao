// solver.h -- jacobi solver as a TAO_PAR_FOR_2D_BASE class

#include "heat.h"
#include "tao.h"
#include "tao_parfor2D.h"

#ifdef DO_LOI
#include "loi.h"
#endif

// chunk sizes for KRD
#define KRDBLOCKX 128  // 128KB
#define KRDBLOCKY 128

#define JACOBI2D 0
#define COPY2D 1

#define min(a,b) ( ((a) < (b)) ? (a) : (b) )


class jacobi2D : public TAO_PAR_FOR_2D_BASE
{
    public:
            jacobi2D(void *a, void*c, int rows, int cols, int offx, int offy, int chunkx, int chunky, 
                         gotao_schedule_2D sched, int ichunkx, int ichunky, int width, float place=GOTAO_NO_AFFINITY,
                         int nthread=0) 
                         : TAO_PAR_FOR_2D_BASE(a,c,rows,cols,offx,offy,chunkx,chunky,
                                         sched,ichunkx,ichunky,width,place) {}

                int ndx(int a, int b){ return a*gotao_parfor2D_cols + b; }

                int compute_for2D(int offx, int offy, int chunkx, int chunky)
                {

                    double *in = (double *) gotao_parfor2D_in;
                    double *out = (double *) gotao_parfor2D_out;
                    double diff; double sum=0.0;

                    // global rows and cols
                    int grows = gotao_parfor2D_rows;
                    int gcols = gotao_parfor2D_cols;


                    int xstart = (offx == 0)? 1 : offx;
                    int ystart = (offy == 0)? 1 : offy;
                    int xstop = ((offx + chunkx) >= grows)? grows - 1: offx + chunkx;
                    int ystop = ((offy + chunky) >= gcols)? gcols - 1: offy + chunky;

#if DO_LOI
    kernel_profile_start();
#endif
                    for (int i=xstart; i<xstop; i++) 
                        for (int j=ystart; j<ystop; j++) {
                        out[ndx(i,j)]= 0.25 * (in[ndx(i,j-1)]+  // left
                                               in[ndx(i,j+1)]+  // right
                                               in[ndx(i-1,j)]+  // top
                                               in[ndx(i+1,j)]); // bottom
                               
// for similarity with the OmpSs version, we do not check the residual
//                        diff = out[ndx(i,j)] - in[ndx(i,j)];
//                        sum += diff * diff; 
                        }
#if DO_LOI
    kernel_profile_stop(JACOBI2D);
#if DO_KRD
    for(int x = xstart; x < xstop; x += KRDBLOCKX)
      for(int y = ystart; y < ystop; y += KRDBLOCKY)
      {
      int krdblockx = ((x + KRDBLOCKX - 1) < xstop)? KRDBLOCKX : xstop - x;
      int krdblocky = ((y + KRDBLOCKY - 1) < ystop)? KRDBLOCKY : ystop - y;
      kernel_trace1(JACOBI2D, &in[ndx(x,y)], KREAD(krdblockx*krdblocky)*sizeof(double));
      kernel_trace1(JACOBI2D, &out[ndx(x,y)], KREAD(krdblockx*krdblocky)*sizeof(double));
      }
#endif
#endif

//                     std::cout << "compute offx " << offx << " offy " << offy 
//                          << " chunkx " << chunkx << " chunky " << chunky 
//                          << " xstart " << xstart << " xstop " << xstop 
//                          << " ystart " << ystart << " ystop " << ystop
//                          << " affinity "  << get_affinity()
//                          << " local residual is " << sum << std::endl;
                }

};

class copy2D : public TAO_PAR_FOR_2D_BASE
{
    public:
            copy2D(void *a, void*c, int rows, int cols, int offx, int offy, int chunkx, int chunky, 
                         gotao_schedule_2D sched, int ichunkx, int ichunky, int width, float place=GOTAO_NO_AFFINITY,
                         int nthread=0) 
                         : TAO_PAR_FOR_2D_BASE(a,c,rows,cols,offx,offy,chunkx,chunky,
                                         sched,ichunkx,ichunky,width,place) {}

                  int ndx(int a, int b){ return a*gotao_parfor2D_cols + b; }

                  int compute_for2D(int offx, int offy, int chunkx, int chunky)
                  {
                    double *in = (double *) gotao_parfor2D_in;
                    double *out = (double *) gotao_parfor2D_out;
                    double diff; double sum=0.0;

                    // global rows and cols
                    int grows = gotao_parfor2D_rows;
                    int gcols = gotao_parfor2D_cols;

                    int xstart = offx;
                    int ystart = offy;

                    int xstop = ((offx + chunkx) >= grows)? grows: offx + chunkx;
                    int ystop = ((offy + chunky) >= gcols)? gcols: offy + chunky;
#if DO_LOI
    kernel_profile_start();
#endif

                    for (int i=xstart; i<xstop; i++) 
                        for (int j=ystart; j<ystop; j++) 
                           out[ndx(i,j)]= in[ndx(i,j)];

//                    std::cout << "copy: offx " << offx << " offy " << offy 
//                          << " chunkx " << chunkx << " chunky " << chunky 
//                          << " xstart " << xstart << " xstop " << xstop 
//                          << " ystart " << ystart << " ystop " << ystop
//                          << " affinity "  << get_affinity()  
//                          << std::endl;
#if DO_LOI
    kernel_profile_stop(COPY2D);
#if DO_KRD
    for(int x = xstart; x < xstop; x += KRDBLOCKX)
      for(int y = ystart; y < ystop; y += KRDBLOCKY)
      {
      int krdblockx = ((x + KRDBLOCKX - 1) < xstop)? KRDBLOCKX : xstop - x;
      int krdblocky = ((y + KRDBLOCKY - 1) < ystop)? KRDBLOCKY : ystop - y;
      kernel_trace1(JACOBI2D, &in[ndx(x,y)], KREAD(krdblockx*krdblocky)*sizeof(double));
      kernel_trace1(JACOBI2D, &out[ndx(x,y)], KREAD(krdblockx*krdblocky)*sizeof(double));
      }
#endif
#endif
                   }

};

/*
 * Blocked Jacobi solver: one iteration step
 */
/*double relax_jacobi (unsigned sizey, double (*u)[sizey], double (*utmp)[sizey], unsigned sizex)
{
    double diff, sum=0.0;
    int nbx, bx, nby, by;
  
    nbx = 4;
    bx = sizex/nbx;
    nby = 4;
    by = sizey/nby;
    for (int ii=0; ii<nbx; ii++)
        for (int jj=0; jj<nby; jj++) 
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++) 
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                utmp[i][j]= 0.25 * (u[ i][j-1 ]+  // left
                         u[ i][(j+1) ]+  // right
                             u[(i-1)][ j]+  // top
                             u[ (i+1)][ j ]); // bottom
                diff = utmp[i][j] - u[i][j];
                sum += diff * diff; 
            }

    return sum;
}
*/
/*
 * Blocked Red-Black solver: one iteration step
 */
/*
double relax_redblack (unsigned sizey, double (*u)[sizey], unsigned sizex )
{
    double unew, diff, sum=0.0;
    int nbx, bx, nby, by;
    int lsw;

    nbx = 4;
    bx = sizex/nbx;
    nby = 4;
    by = sizey/nby;
    // Computing "Red" blocks
    for (int ii=0; ii<nbx; ii++) {
        lsw = ii%2;
        for (int jj=lsw; jj<nby; jj=jj+2) 
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++) 
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                      u[ i][(j+1) ]+  // right
                      u[ (i-1)][ j ]+  // top
                      u[ (i+1)][ j ]); // bottom
                diff = unew - u[i][j];
                sum += diff * diff; 
                u[i][j]=unew;
            }
    }

    // Computing "Black" blocks
    for (int ii=0; ii<nbx; ii++) {
        lsw = (ii+1)%2;
        for (int jj=lsw; jj<nby; jj=jj+2) 
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++) 
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                      u[ i][ (j+1) ]+  // right
                      u[ (i-1)][ j     ]+  // top
                      u[ (i+1)][ j     ]); // bottom
                diff = unew - u[i][ j];
                sum += diff * diff; 
                u[i][j]=unew;
            }
    }

    return sum;
}
*/
/*
 * Blocked Gauss-Seidel solver: one iteration step
 */
/*
double relax_gauss (unsigned padding, unsigned sizey, double (*u)[sizey], unsigned sizex )
{
    double unew, diff, sum=0.0;
    int nbx, bx, nby, by;

    nbx = 8;
    bx = sizex/nbx;
    nby = 8;
    by = sizey/nby;
    for (int ii=0; ii<nbx; ii++)
        for (int jj=0; jj<nby; jj++){
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++)
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                      u[ i][(j+1) ]+  // right
                      u[ (i-1)][ j     ]+  // top
                      u[ (i+1)][ j     ]); // bottom
                diff = unew - u[i][ j];
                sum += diff * diff; 
                u[i][j]=unew;
                }
        }

    return sum;
}
*/
