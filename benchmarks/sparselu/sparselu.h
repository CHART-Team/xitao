/* BSD 3-Clause License

* Copyright (c) 2019-2021, contributors
* All rights reserved.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.

* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.

* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SPARSELU
#define SPARSELU

#define FALSE (0)
#define TRUE (1)

typedef double ELEM;

/* ************************************************************ */
/* sparselu kernels                              */
/* ************************************************************ */

void lu0(ELEM *diag, int BSIZE)
{
   int i, j, k;
                                                                                   
   for (k=0; k<BSIZE; k++)
      for (i=k+1; i<BSIZE; i++) {
         diag[i*BSIZE+k] = diag[i*BSIZE+k] / diag[k*BSIZE+k];
         for (j=k+1; j<BSIZE; j++)
            diag[i*BSIZE+j] = diag[i*BSIZE+j] - diag[i*BSIZE+k] * diag[k*BSIZE+j];
      }
}

void fwd(ELEM *diag, ELEM *col, int BSIZE)
{
   int i, j, k;
                                                                              
   for (k=0; k<BSIZE; k++) 
      for (i=k+1; i<BSIZE; i++)
         for (j=0; j<BSIZE; j++)
            col[i*BSIZE+j] = col[i*BSIZE+j] - diag[i*BSIZE+k]*col[k*BSIZE+j];
}

void bdiv(ELEM *diag, ELEM *row, int BSIZE)
{
   int i, j, k;
                                                                              
   for (i=0; i<BSIZE; i++)
      for (k=0; k<BSIZE; k++) {
         row[i*BSIZE+k] = row[i*BSIZE+k] / diag[k*BSIZE+k];
         for (j=k+1; j<BSIZE; j++)
            row[i*BSIZE+j] = row[i*BSIZE+j] - row[i*BSIZE+k]*diag[k*BSIZE+j];
      }
}

void bmod(ELEM *row, ELEM *col, ELEM *inner, int BSIZE)
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
#endif
