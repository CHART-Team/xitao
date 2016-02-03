#include "heat-ompss.h"
#include <stdlib.h>

#define min(a,b) ( ((a) < (b)) ? (a) : (b) )

/*
 * Blocked Jacobi solver: one iteration step
 */
void relax_jacobi (unsigned padding, unsigned sizey, double (*u)[sizey+padding*2], double (*utmp)[sizey+padding*2], unsigned sizex, int check, double *residual )
{
    int nbx, bx, nby, by;
    int inf_i, sup_i, inf_j, sup_j;
  
    nbx = 4;
    bx = sizex/nbx;
    nby = 4;
    by = sizey/nby;
    
    double *sum;
    double (*local_sum)[nby];
    posix_memalign( (void**)&local_sum, sizeof( double) * nbx *nby, sizeof( double) * nbx *nby );
    
    if ( check ) {
        posix_memalign( (void**)&sum, sizeof( double ), sizeof( double ) );
        *sum=0.0;
    }

    for (int ii=padding+1; ii<sizex-1+padding; ii+=bx) {
        for (int jj=padding+1; jj<sizey-1+padding; jj+=by) {
            inf_i = ii; inf_j=jj; 
            sup_i = min( ii+bx, sizex-1 + padding );
            sup_j = min( jj+by, sizey-1 + padding );
            
            #pragma omp task in( u[inf_i-1;1][inf_j:sup_j-1],\
                u[sup_i;1][inf_j:sup_j-1], u[inf_i:sup_i-1][inf_j-1;1],\
                u[inf_i:sup_i-1][sup_j;1] )\
                in( u[inf_i:sup_i-1][inf_j:sup_j-1] )\
                out( utmp[inf_i:sup_i-1][inf_j:sup_j-1],\
                local_sum[(ii-padding)/bx][(jj-padding)/by] ) label(jacobi_iter)
            {
                if ( check ) local_sum[(ii-padding)/bx][(jj-padding)/by]=0.0f;
            
                for (int i=inf_i; i<sup_i; i++) {
                    for (int j=inf_j; j<sup_j; j++) {
                        double diff;
                        utmp[i][j]= 0.25 * (u[ i][j-1 ]+  // left
                                                 u[ i][(j+1) ]+  // right
                                                 u[(i-1)][ j]+  // top
                                                 u[ (i+1)][ j ]); // bottom
                        if ( check ) {
                            diff = utmp[i][j] - u[i][j];
                            local_sum [(ii-padding)/bx][(jj-padding)/by] += diff * diff;
                        }
                    }
                }
                
            }
            if ( check ) {
                #pragma omp task in (local_sum [(ii-padding)/bx][(jj-padding)/by]) inout (*sum) label(sum_acc)
                {
                    *sum += local_sum [(ii-padding)/bx][(jj-padding)/by];
                }
            }
        }
    }
    
    if ( check ) {
        #pragma omp task in (*sum) out (*residual) label(residual_set)
        {
            *residual = *sum;
            // Release local variables
            free( local_sum );
            free( sum );
        }
    }
    else {
        // Free local_sum as it will not be released if check is false
        free( local_sum );
    }

}

/*
 * Blocked Red-Black solver: one iteration step
 */
void relax_redblack (unsigned padding, unsigned sizey, double (*u)[sizey+padding*2], unsigned sizex, int check, double *residual  )
{
    int nbx, bx, nby, by;
    int inf_i, sup_i, inf_j, sup_j;
    int lsw;

    nbx = 4;
    bx = sizex/nbx;
    nby = 4;
    by = sizey/nby;
    
    double *sum;
    double (*local_sum)[nby];
    posix_memalign( (void**)&local_sum, sizeof( double) * nbx *nby, sizeof( double) * nbx *nby );
    
    if ( check ) {
        posix_memalign( (void**)&sum, sizeof( double ), sizeof( double ) );
        *sum=0.0;
    }

    
    // Computing "Red" blocks
    for (int ii=0; ii<nbx; ii++) {
        lsw = ii%2;
        for (int jj=lsw; jj<nby; jj=jj+2) {
            inf_i = padding+1+ii*bx;
            inf_j = padding+1+jj*by;
            sup_i = min(padding+1+(ii+1)*bx, padding+sizex-1);
            sup_j = min(padding+1+(jj+1)*by, padding+sizey-1);
            #pragma omp task in( u[inf_i-1;1][inf_j:sup_j-1],\
                u[sup_i;1][inf_j:sup_j-1], u[inf_i:sup_i-1][inf_j-1;1],\
                u[inf_i:sup_i-1][sup_j;1] )\
                inout( u[inf_i:sup_i-1][inf_j:sup_j-1] )\
                out( local_sum[ii][jj] ) label(red_iter)
            {
                if( check ) local_sum[ii][jj] = 0.0f;
                for (int i=inf_i; i < sup_i; i++) 
                    for (int j=inf_j; j < sup_j; j++) {
                        double unew, diff;
                        unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                                          u[ i][(j+1) ]+  // right
                                          u[ (i-1)][ j ]+  // top
                                          u[ (i+1)][ j ]); // bottom
                        if ( check ) {
                            diff = unew - u[i][j];
                            local_sum[ii][jj] += diff * diff;
                        }
                        u[i][j]=unew;
                    }
            }
            if ( check ) {
                #pragma omp task in (local_sum [ii][jj]) inout (*sum) 
                {
                    *sum += local_sum [ii][jj];
                }
            }
        }
    }

    // Computing "Black" blocks
    for (int ii=0; ii<nbx; ii++) {
        lsw = (ii+1)%2;
        for (int jj=lsw; jj<nby; jj=jj+2) {
            inf_i = padding+1+ii*bx;
            inf_j = padding+1+jj*by;
            sup_i = min(padding+1+(ii+1)*bx, padding+sizex-1);
            sup_j = min(padding+1+(jj+1)*by, padding+sizey-1);
            #pragma omp task in( u[inf_i-1;1][inf_j:sup_j-1],\
                u[sup_i;1][inf_j:sup_j-1], u[inf_i:sup_i-1][inf_j-1;1],\
                u[inf_i:sup_i-1][sup_j;1] )\
                inout( u[inf_i:sup_i-1][inf_j:sup_j-1] )\
                out( local_sum[ii][jj] ) label(black_iter)
            {
                if( check ) local_sum[ii][jj] = 0.0f;
                for (int i=inf_i; i < sup_i; i++) 
                    for (int j=inf_j; j < sup_j; j++) {
                        double unew, diff;
                        unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                                          u[ i][ (j+1) ]+  // right
                                          u[ (i-1)][ j     ]+  // top
                                          u[ (i+1)][ j     ]); // bottom
                        if ( check ) {
                            diff = unew - u[i][j];
                            local_sum[ii][jj] += diff * diff;
                        }
                        u[i][j]=unew;
                    }
            }
            if ( check ) {
                #pragma omp task in (local_sum [ii][jj]) inout (*sum) label(sum_acc)
                {
                    *sum += local_sum [ii][jj];
                }
            }
        }
    }

    if ( check ) {
        #pragma omp task in (*sum) out (*residual) label(residual_set)
        {
            *residual = *sum;
            // Release local variables
            free( local_sum );
            free( sum );
        }
    }
    else {
        // Free local_sum as it will not be released if check is false
        free( local_sum );
    }
}

/*
 * Blocked Gauss-Seidel solver: one iteration step
 */
void relax_gauss (unsigned padding, unsigned sizey, double (*u)[sizey+padding*2], unsigned sizex, int check, double *residual )
{
    // gmiranda: Do not use variables in the stack for tasks since they might be destroyed
    double *sum;
    
    int nbx, bx, nby, by;
    int inf_i, sup_i, inf_j, sup_j;

    nbx = 8;
    bx = sizex/nbx;
    nby = 8;
    by = sizey/nby;
    

    // gmiranda: same, stack vars are not allowed!
    double (*local_sum)[nby];
    posix_memalign( (void**)&local_sum, sizeof( double) * nbx *nby, sizeof( double) * nbx *nby );

    if ( check ) {
        posix_memalign( (void**)&sum, sizeof( double ), sizeof( double ) );
        //#pragma omp task out (*sum) label(init_sum)
        *sum=0.0;
    }
   

    for (int ii=padding+1; ii<sizex-1+padding; ii+=bx){
        for (int jj=padding+1; jj<sizey-1+padding; jj+=by) {
		inf_i = ii; inf_j=jj; 
	 	sup_i = (ii+bx)<sizex-1 + padding ? ii+bx : sizex-1 + padding; 	
	 	sup_j = (jj+bx)<sizey-1 + padding ? jj+by : sizey-1 + padding;
// north, south, west, east
#pragma omp task in( u[inf_i-1;1][inf_j:sup_j-1],\
    u[sup_i;1][inf_j:sup_j-1],\
    u[inf_i:sup_i-1][inf_j-1;1],\
    u[inf_i:sup_i-1][sup_j;1] )\
    inout( u[inf_i:sup_i-1][inf_j:sup_j-1] )\
    out( local_sum[(ii-padding)/bx][(jj-padding)/by] ) label(gauss_iteration)
{
            if ( check )
                local_sum[(ii-padding)/bx][(jj-padding)/by]=0.0f;
	
            for (int i=inf_i; i<sup_i; i++)
                for (int j=inf_j; j<sup_j; j++) {
                    double unew, diff;
                    unew= 0.25 * (    u[i][(j-1)]+  // left
                          u[i][(j+1)]+  // right
                          u[(i-1)][j]+  // top
                          u[(i+1)][j]); // bottom
                    if ( check ) {
                        diff = unew - u[i][j];
                        local_sum [(ii-padding)/bx][(jj-padding)/by] += diff * diff;
                    }
                    u[i][j]=unew;
                }
}
        if ( check ) {
            #pragma omp task in (local_sum [(ii-padding)/bx][(jj-padding)/by]) inout (*sum) label(sum_acc)
            {
                *sum += local_sum [(ii-padding)/bx][(jj-padding)/by];
            }
        }
    }
    }
    if ( check ) {
        #pragma omp task in (*sum) out (*residual) label(residual_set)
        {
            *residual = *sum;
            // Release local variables
            free( local_sum );
            free( sum );
        }
    }
    else {
        // Free local_sum as it will not be released if check is false
        free( local_sum );
    }
}
