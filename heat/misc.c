/*
 * misc.c
 *
 * Helper functions for
 * - printing test parameters 
 * - rading parameters file
 * - initialization
 * - finalization,
 * - writing out a picture
 * - timing execution time
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys/time.h>
#include <malloc.h>

#include "heat.h"

/*
 * Initialize the iterative solver
 * - allocate memory for matrices
 * - set boundary conditions according to configuration
 */
int initialize( algoparam_t *param, int no_padding )
{
    int i, j;
    double dist;

    // total number of points (including border)
    const int np = param->resolution+2;
    
    // Padding required for proper alignment.
    if ( no_padding )
	param->padding = 0;
    else{
	param->padding = param->resolution/2-1;
	//fprintf( stderr, "Padding data (%d)\n", param->padding );
    }
    
    // Padded row/column size
    const int psize = np + 2*param->padding;
  
    //
    // allocate memory
    //
    if ( no_padding ) {
	(param->u)     = (double*)calloc( sizeof(double),np*np );
	(param->uhelp) = (double*)calloc( sizeof(double),np*np );
    }
    else {
	// Aligned and padded memory allocation is only required for u and uhelp
	const size_t alignment = sizeof(double)*psize*psize;
	//fprintf( stderr, "Aligning memory (%d *sizeof(double/**/))\n", alignment/sizeof(double) );
	
	posix_memalign( (void**) &(param->u), alignment, sizeof(double)*psize*psize );
	posix_memalign( (void**) &(param->uhelp), alignment, sizeof(double)*psize*psize );
    }
    (param->uvis)  = (double*)calloc( sizeof(double),
				      (param->visres+2) *
				      (param->visres+2) );

    if( !(param->u) || !(param->uhelp) || !(param->uvis) )
    {
	fprintf(stderr, "Error: Cannot allocate memory\n");
	return 0;
    }
    
    if ( !no_padding ) {
	memset( &param->u[param->padding*psize+param->padding], 0, sizeof(double)*np*np  );
	memset( &param->uhelp[param->padding*psize+param->padding], 0, sizeof(double)*np*np );
    }

    for( i=0; i<param->numsrcs; i++ )
    {
	/* top row */
	for( j=0; j<np; j++ )
	{
	    dist = sqrt( pow((double)j/(double)(np-1) - 
			     param->heatsrcs[i].posx, 2)+
			 pow(param->heatsrcs[i].posy, 2));
	  
	    if( dist <= param->heatsrcs[i].range )
	    {
                // Mind the padding
		(param->u)[((psize+1)*param->padding)+j] +=
		    (param->heatsrcs[i].range-dist) /
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
	    }
	}
      
	/* bottom row */
	for( j=0; j<np; j++ )
	{
	    dist = sqrt( pow((double)j/(double)(np-1) - 
			     param->heatsrcs[i].posx, 2)+
			 pow(1-param->heatsrcs[i].posy, 2));
	  
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[psize*(param->padding+np-1)+param->padding+j]+=
		    (param->heatsrcs[i].range-dist) / 
		    param->heatsrcs[i].range * 
		    param->heatsrcs[i].temp;
	    }
	}
      
	/* leftmost column */
	for( j=1; j<np-1; j++ )
	{
	    dist = sqrt( pow(param->heatsrcs[i].posx, 2)+
			 pow((double)j/(double)(np-1) - 
			     param->heatsrcs[i].posy, 2)); 
	  
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[ (j+param->padding)*psize+param->padding ]+=
		    (param->heatsrcs[i].range-dist) / 
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
	    }
	}
      
	/* rightmost column */
	for( j=1; j<np-1; j++ )
	{
	    dist = sqrt( pow(1-param->heatsrcs[i].posx, 2)+
			 pow((double)j/(double)(np-1) - 
			     param->heatsrcs[i].posy, 2)); 
	  
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[ (j+param->padding)*psize+param->padding+np-1 ]+=
		    (param->heatsrcs[i].range-dist) /
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
	    }
	}
    }

    // Copy u into uhelp
    double *putmp, *pu;
    pu = param->u;
    putmp = param->uhelp;
    for( int j=0; j<psize; j++ )
	for( int i=0; i<psize; i++ )
	    *putmp++ = *pu++;

    // exchange parameters
    param->ftmp = param->u;

    return 1;
}

/*
 * free used memory
 */
int finalize( algoparam_t *param )
{
    if( param->u ) {
	free(param->u);
	param->u = 0;
    }

    if( param->uhelp ) {
	free(param->uhelp);
	param->uhelp = 0;
    }

    if( param->uvis ) {
	free(param->uvis);
	param->uvis = 0;
    }

    return 1;
}


/*
 * write the given temperature u matrix to rgb values
 * and write the resulting image to file f
 */
void write_image( FILE * f, double *u, unsigned padding,
		  unsigned sizex, unsigned sizey ) 
{
    // RGB table
    unsigned char r[1024], g[1024], b[1024];
    int i, j, k;
  
    double min, max;

    j=1023;

    // prepare RGB table
    for( i=0; i<256; i++ )
    {
	r[j]=255; g[j]=i; b[j]=0;
	j--;
    }
    for( i=0; i<256; i++ )
    {
	r[j]=255-i; g[j]=255; b[j]=0;
	j--;
    }
    for( i=0; i<256; i++ )
    {
	r[j]=0; g[j]=255; b[j]=i;
	j--;
    }
    for( i=0; i<256; i++ )
    {
	r[j]=0; g[j]=255-i; b[j]=255;
	j--;
    }

    min=DBL_MAX;
    max=-DBL_MAX;

    // find minimum and maximum 
    for( i=0; i<sizey; i++ )
    {
	for( j=0; j<sizex; j++ )
	{
	    if( u[i*sizex+j]>max )
		max=u[i*sizex+j];
	    if( u[i*sizex+j]<min )
		min=u[i*sizex+j];
	}
    }
  

    fprintf(f, "P3\n");
    fprintf(f, "%u %u\n", sizex, sizey);
    fprintf(f, "%u\n", 255);

    for( i=0; i<sizey; i++ )
    {
	for( j=0; j<sizex; j++ )
	{
	    //k=(int)(1024.0*(u[i*sizex+j]-min)/(max-min));
	    // gmiranda: 1024 is out of bounds
	    k=(int)(1023.0*(u[i*sizex+j]-min)/(max-min));
	    fprintf(f, "%d %d %d  ", r[k], g[k], b[k]);
	}
	fprintf(f, "\n");
    }
}


int coarsen( double *uold, unsigned oldx, unsigned oldy, unsigned padding,
	     double *unew, unsigned newx, unsigned newy )
{
    int i, j;

    int stepx;
    int stepy;
    int stopx = newx;
    int stopy = newy;

    if (oldx>newx)
	// FIXME: padding not considered here
	stepx=oldx/newx;
    else {
	stepx=1;
	stopx=oldx;
    }
    if (oldy>newy)
	// FIXME: padding not considered here
	stepy=oldy/newy;
    else {
	stepy=1;
	stopy=oldy;
    }

    // NOTE: this only takes the top-left corner,
    // and doesnt' do any real coarsening
    for( i=0; i<stopy-1; i++ )
    {
	for( j=0; j<stopx-1; j++ )
        {
	    //unew[i*(newx+2*padding)+j]=uold[i*(oldx+2*padding)*stepy+j*stepx];
	    unew[i*newx+j]=uold[(i+padding)*(oldx+2*padding)*stepy+(j+padding)*stepx];
        }
    }

  return 1;
}

#define BUFSIZE 100
int read_input( FILE *infile, algoparam_t *param )
{
  int i, n;
  char buf[BUFSIZE];

  fgets(buf, BUFSIZE, infile);
  n = sscanf( buf, "%u", &(param->maxiter) );
  if( n!=1)
    return 0;

  fgets(buf, BUFSIZE, infile);
  n = sscanf( buf, "%u", &(param->resolution) );
  if( n!=1 )
    return 0;

  param->visres = param->resolution;

  fgets(buf, BUFSIZE, infile);
  n = sscanf(buf, "%d", &(param->algorithm) );
  if( n!=1 )
    return 0;

  fgets(buf, BUFSIZE, infile);
  n = sscanf(buf, "%u", &(param->numsrcs) );
  if( n!=1 )
    return 0;

  //(param->heatsrcs) = 
  //  (heatsrc_t*) malloc( sizeof(heatsrc_t) * (param->numsrcs) );
  posix_memalign( (void **) &param->heatsrcs, sizeof(heatsrc_t) * (param->numsrcs), sizeof(heatsrc_t) * (param->numsrcs) );
  
  for( i=0; i<param->numsrcs; i++ )
    {
      fgets(buf, BUFSIZE, infile);
      n = sscanf( buf, "%f %f %f %f",
		  &(param->heatsrcs[i].posx),
		  &(param->heatsrcs[i].posy),
		  &(param->heatsrcs[i].range),
		  &(param->heatsrcs[i].temp) );

      if( n!=4 )
	return 0;
    }

  return 1;
}


void print_params( algoparam_t *param )
{
  int i;

  fprintf(stdout, "Iterations        : %u\n", param->maxiter);
  fprintf(stdout, "Resolution        : %u\n", param->resolution);
  fprintf(stdout, "Algorithm         : %d (%s)\n",
	  param->algorithm,
	  (param->algorithm == 0) ? "Jacobi":(param->algorithm ==1) ? "Gauss-Seidel":"Red-Black" );
  fprintf(stdout, "Num. Heat sources : %u\n", param->numsrcs);

  for( i=0; i<param->numsrcs; i++ )
    {
      fprintf(stdout, "  %2d: (%2.2f, %2.2f) %2.2f %2.2f \n",
	     i+1,
	     param->heatsrcs[i].posx,
	     param->heatsrcs[i].posy,
	     param->heatsrcs[i].range,
	     param->heatsrcs[i].temp );
    }
}


//
// timing.c 
// 
double wtime()
{
  struct timeval tv;
  gettimeofday(&tv, 0);

  return tv.tv_sec+1e-6*tv.tv_usec;
}

