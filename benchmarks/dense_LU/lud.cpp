// Most of the code is extracted and adapted from the rodinia benchmark for dense LU //
#include"lud.h"
#include "xitao.h"
using namespace xitao;

// Generate well-conditioned matrix internally  by Ke Wang 2013/08/07 22:20:06 
int create_matrix(float **mp, int size)
{
  float *m;
  int i,j;
  float lamda = -0.001;
  float coe[2*size-1];
  float coe_i =0.0;

  for (i=0; i < size; i++)
    {
      coe_i = 10*exp(lamda*i); 
      j=size-1+i;     
      coe[j]=coe_i;
      j=size-1-i;     
      coe[j]=coe_i;
    }

  m = (float*) malloc(sizeof(float)*size*size);
  if ( m == NULL) {
      return 0;
  }

  for (i=0; i < size; i++) {
      for (j=0; j < size; j++) {
	m[i*size+j]=coe[size-1-i+j];
      }
  }

  *mp = m;

  return 1;
}

void copy_matrix(float *src, float **dst, int dim)
{
    int size = dim*dim*sizeof(float);
    float *temp = (float *) malloc (size);
    std::memcpy(temp, src, size);
    *dst = temp;
}

// c = a * b , where a,b and c are matrices of order "size"
void matrix_multiply(float *a, float *b, float *c, int size)
{
    int i;
    int j;
    int k;

    for(i = 0; i < size; i++)
	for(j = 0; j < size; j++)
	    for(k = 0; k < size; k++)
		c[i*size + k] = a[i*size + j] * b[j*size + k];    
}

// Check the correction of the factorization by computing:
// ||m - l*u || / ||m|| , where m is the input matrix 
int lud_verify(float *m, float *lu, int matrix_dim)
{
    int i,j,k;
    int flag = 1;
    float diff = 0.0;
    float norm = 0.0;
	
    float *tmp = (float*)malloc(matrix_dim*matrix_dim*sizeof(float)); 

    // compute tmp = l*u
    for(i = 0; i < matrix_dim; i ++)
	for(j = 0; j< matrix_dim; j++)
	{
	    float sum = 0;
	    float l,u;
	    for (k = 0; k <= MIN(i,j); k++)
	    {
		if (i == k)
		    l = 1;
		else
		    l = lu[i*matrix_dim + k];
		u = lu[k*matrix_dim + j];
		sum += l*u;
	    }
	    tmp[i*matrix_dim + j] = sum;
	}

    // compute the sum term of the norms
    for (i = 0; i < matrix_dim; i++)
    {
	for (j = 0; j < matrix_dim; j++)
	{
	    diff += (m[i*matrix_dim + j] - tmp[i*matrix_dim + j]) * (m[i*matrix_dim + j] - tmp[i*matrix_dim + j]);
	    norm += m[i*matrix_dim + j]*m[i*matrix_dim + j];	    
	}
    }
    
    printf("Relative Error = %13.8e\n", sqrt(diff)/sqrt(norm) );
    
    if(sqrt(diff)/sqrt(norm) > 1e-5)
	flag = 0;
    
    free(tmp);

    return flag;
}



int parallel_lud(float *a, int size)
{
    if(size <= 0)
    {
	cout << "Error: input matrix size must be >= 1" << endl;
	return 0;
    }
    
    //int num_diagonal_blocks = (size % BS)? (size / BS) + 1 : size / BS;
    AssemblyTask **dep_matrix;
    int offset, chunk_idx, size_inter, chunks_in_inter_row, chunks_per_inter;
    int ii,jj,kk;

    // init XiTAO runtime 
    gotao_init();
    
    // create dependency matrix
    dep_matrix = (AssemblyTask**) malloc(sizeof(AssemblyTask*)*size*size);
    for(int i = 0; i < size; i++)
	for(int j = 0; j < size; j++)
	    DM(i,j) = NULL;
    
    // loop over iterations of LU factorization
    for(offset = 0, kk = 0; offset < size - BS ; offset += BS, kk++)
    {
        // lu factorization of left-top corner block diagonal matrix 
	lud_diagonal *diagonal_block = new lud_diagonal(a,size,offset);
	if(DM(kk,kk)) // add dependency from the previous diagonal block
	{
	    DM(kk,kk)->make_edge(diagonal_block);
	}
	else //this is the first block, push it 
	{
	    gotao_push(diagonal_block);
	    cout << "Pushed the head of the DAG" << endl;	    
	}	
	// add current block to the dependency matrix
	DM(kk,kk) = diagonal_block;
	
	// preparation for perimeter blocks calculations 
        size_inter = size - offset -  BS;
        chunks_in_inter_row  = size_inter/BS;
	
        // calculate perimeter block matrices
        for(chunk_idx = 0, ii = kk+1; chunk_idx < chunks_in_inter_row; chunk_idx++, ii++)
        {
            // processing top perimeter
	    top_perimeter *top_block = new top_perimeter(a, size, offset, chunk_idx, chunks_per_inter, chunks_in_inter_row);
	    // add edge from diagonal block to top perimeter
	    if(DM(kk,kk))
		DM(kk,kk)->make_edge(top_block);
	    
	    // add edges from internal block of previous iteration to diagonal block
	    if(DM(kk,ii))
		DM(kk,ii)->make_edge(top_block);	    
	    // add current block to the dependency matrix
	    DM(kk,ii) = top_block;	    
	    
            // processing left perimeter
	    left_perimeter *left_block = new left_perimeter(a, size, offset, chunk_idx, chunks_per_inter, chunks_in_inter_row);	    
	    // add edge from diagonal block to left perimeter
	    if(DM(kk,kk))
		DM(kk,kk)->make_edge(left_block);	    
	    // add edges from internal block of previous iteration to diagonal block
	    if(DM(ii,kk))
		DM(ii,kk)->make_edge(left_block);
	    // add current block to the dependency matrix	    
	    DM(ii,kk) = left_block;	    	    
        }
	
        // update interior block matrices
	chunks_per_inter = chunks_in_inter_row*chunks_in_inter_row;
	
	// prepare deb_matrix indices
	ii = kk+1;
	jj = kk+1;
	// loop over inernal chunks row by row, j is the fast changing index.		
        for(chunk_idx =0; chunk_idx < chunks_per_inter; chunk_idx++)
        {                    
	    update_inter_block *internal_block = new update_inter_block(a, size, offset, chunk_idx, chunks_per_inter, chunks_in_inter_row);
	    
	    // add edge from top
	    if(DM(kk,jj))
		DM(kk,jj)->make_edge(internal_block);
	    
	    // add edge from left
	    if(DM(ii,kk))
		DM(ii,kk)->make_edge(internal_block);
	    
	    // add edge from previous iteration
	    if(DM(ii,jj))
		DM(ii,jj)->make_edge(internal_block);
	    // add current block to the dependency matrix	    
	    DM(ii,jj) = internal_block;
	    
	    // adjust ii and jj
	    if(jj == (kk + chunks_in_inter_row))
	    {
		ii++;
		jj = kk + 1;
	    }
	    else
	    {
		jj++;
	    }	    
        }       
    }
    
    // last block
    lud_diagonal *diagonal_block = new lud_diagonal(a,size,offset);
    // add current block to the dependency matrix    
    if(DM(kk,kk))
    {
	DM(kk,kk)->make_edge(diagonal_block);
    }
    // add current block to the dependency matrix    
    DM(kk,kk) = diagonal_block;    
          
    //stat executing the DAG
    cout << "Start execution" << endl;    
    gotao_start();

    // finalize XiTAO runtime 
    gotao_fini();    
    cout << "Reached the end of the code!" << endl;
    
    // clean up
    free(dep_matrix);
    
    return 1;    
}

int main(int argc, char *argv[])
{    
    cout << "start" << endl;
    float *a, *b;    
    int matrix_dim = 512; // matrix order 
    int ret = 0;

    // create matrix A
    ret = create_matrix(&a, matrix_dim);
    if(!ret)
	cout << "Error creating matrix A" << endl;

    // copy A to B
    copy_matrix(a, &b, matrix_dim);
 
    // compute L and U
    ret = 0;
    ret = parallel_lud(a, matrix_dim);
    if(!ret)
	cout << "Error computing L and U" << endl;
        
    // verify
    ret = 0;    
    ret = lud_verify(b, a, matrix_dim);
    
    if(ret == 1)
    	cout << "success" << endl;
    else
    	cout << "fail" << endl;

    // clean up
    free(a);
    free(b);
    
    return 0;
}
