// solver.h -- jacobi solver as a TAO_PAR_FOR_2D_BASE class

#include "heat.h"
#include "tao.h"
#include "tao_parfor2D.h"

#define min(a,b) ( ((a) < (b)) ? (a) : (b) )


class jacobi2D : public TAO_PAR_FOR_2D_BASE
{
    public:
            jacobi2D(void *a, void*c, int rows, int cols, int offx, int offy, int chunkx, int chunky, 
                         gotao_schedule_2D sched, int ichunkx, int ichunky, int width, int nthread=0) 
                         : TAO_PAR_FOR_2D_BASE(a,c,rows,offx,offy,chunkx,chunky,
                                         cols,sched,ichunkx,ichunky,width) {}

                int compute_for2D(int offx, int offy, int chunkx, int chunky)
                {

                    double *u = (double *) gotao_parfor2D_in;
                    double *utmp = (double *) gotao_parfor2D_out;
                    double diff; double sum=0.0;

                    // global rows and cols
                    int rows = gotao_parfor2D_rows;
                    int cols = gotao_parfor2D_cols;

#define n(a,b) (a*gotao_parfor2D_cols + b)

                    int xstart = (offx == 0)? 1 : offx;
                    int ystart = (offy == 0)? 1 : offy;
                    int xstop = (offx + chunkx >= rows) rows - 1: offx + chunkx;
                    int ystop = (offy + chunky >= cols) cols - 1: offy + chunky;

                    for (int i=xstart; i<xstop; i++) 
                        for (int j=ystart; j<ystop; j++) {
                        utmp[n(i,j)]= 0.25 * (u[n(i,j-1)]+  // left
                                              u[n(i,j+1)]+  // right
                                              u[n(i-1,j)]+  // top
                                              u[n(i+1,j)]); // bottom
                 //       diff = utmp[n(i,j)] - u[n(i,j)];
                 //       sum += diff * diff; 
                        }

                    // the local residual is now in "sum"
                    // but if we reduce it each time, we will lose lots of parallelism, 
                    // so initially I will just ignore the "diff" and "sum" variables
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
