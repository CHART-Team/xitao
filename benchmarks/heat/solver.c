#include "heat.h"

#define min(a,b) ( ((a) < (b)) ? (a) : (b) )

/*
 * Blocked Jacobi solver: one iteration step
 */
double relax_jacobi (unsigned sizey, double (*u)[sizey], double (*utmp)[sizey], unsigned sizex)
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
//                printf("utmp(%d,%d) = %g\n", i, j, utmp[i][j]);
//	            diff = utmp[i][j] - u[i][j];
//	            sum += diff * diff; 
	        }

    return sum;
}

/*
 * Blocked Red-Black solver: one iteration step
 */
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

/*
 * Blocked Gauss-Seidel solver: one iteration step
 */
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

