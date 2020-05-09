#define NB 32
#define BSIZE 4
#define FALSE (0)
#define TRUE (1)

typedef double ELEM;

/* ************************************************************ */
/* sparselu kernels                              */
/* ************************************************************ */

void lu0(ELEM *diag)
{
   int i, j, k;
                                                                                   
   for (k=0; k<BSIZE; k++)
      for (i=k+1; i<BSIZE; i++) {
         diag[i*BSIZE+k] = diag[i*BSIZE+k] / diag[k*BSIZE+k];
         for (j=k+1; j<BSIZE; j++)
            diag[i*BSIZE+j] = diag[i*BSIZE+j] - diag[i*BSIZE+k] * diag[k*BSIZE+j];
      }
}

void fwd(ELEM *diag, ELEM *col)
{
   int i, j, k;
                                                                              
   for (k=0; k<BSIZE; k++) 
      for (i=k+1; i<BSIZE; i++)
         for (j=0; j<BSIZE; j++)
            col[i*BSIZE+j] = col[i*BSIZE+j] - diag[i*BSIZE+k]*col[k*BSIZE+j];
}

void bdiv(ELEM *diag, ELEM *row)
{
   int i, j, k;
                                                                              
   for (i=0; i<BSIZE; i++)
      for (k=0; k<BSIZE; k++) {
         row[i*BSIZE+k] = row[i*BSIZE+k] / diag[k*BSIZE+k];
         for (j=k+1; j<BSIZE; j++)
            row[i*BSIZE+j] = row[i*BSIZE+j] - row[i*BSIZE+k]*diag[k*BSIZE+j];
      }
}

void bmod(ELEM *row, ELEM *col, ELEM *inner)
{
   int i, j, k;
                                                                                 
   for (i=0; i<BSIZE; i++){
         for (k=0; k<BSIZE; k++) {
      for (j=0; j<BSIZE; j++){
            inner[i*BSIZE+j] = inner[i*BSIZE+j] - row[i*BSIZE+k]*col[k*BSIZE+j];
         }
      }
   }
}

