#include "conn_xitao.h"
#if defined(CRIT_PERF_SCHED)
float CON_TAO3::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
float CON_TAO4::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
#endif
// TAO 1
int CON_TAO1::execute(int threadid){

//sample fill_cpu
//void fill_cpu(int N, float ALPHA, float *X, int INCX)
//{
//    int i;
//    for(i = 0; i < N; ++i) X[i*INCX] = ALPHA;
//}
//
	int N = l->outputs * l->batch;
        float ALPHA=0;
        float *X = l->output;
        int INCX=1;
        int tid = threadid- leader;
//      std::cout <<  "Thread " << tid <<" entered in TAO1" << "\n";
        int block = N / width;
	int start;
	int end;
/*	start= tid*block;
	if(tid == width-1)
		end= ((tid+1)*block)+(N % width); // add remainder
	else
		end= ((tid+1)*block);         
        for(int i = start; (i < N)  && (i < end); i++)
                X[i*INCX] = ALPHA;
*/
	// NEW DYNAMIC
	int m_s, m_e;
	while(1){
                 int blockid = next++;
                 if(blockid >= blocks) return 0;
                m_s = blockid*blocksize;
                m_e = (blockid==blocks-1)? N : (blockid+1)*blocksize;
		for(int i = m_s; i< m_e; i++)
                	X[i*INCX] = ALPHA;
	}//While end

    	//fill_cpu(l->outputs*l->batch, 0, l->output, 1);

}//end execute

int CON_TAO3::execute(int threadid){
	int m = l->batch;
        int K = l->inputs;
        int n = l->outputs;
	float *c = l->output;
	int ALPHA =1;
	int BETA = 1;
	//float *im =  net->input + (l->groups)*l->c/l->groups*l->h*l->w;
	int block_size = m/width; // can change to ceil
        int tid = threadid- leader;
	//std::cout <<"Thread " <<tid <<" entered in TAO3" <<"\n";
	int m_s= tid*block_size;
	int m_e;
	int lda = K;
	int ldb = K;
	int ldc = n;
	/*if(tid == width-1)
		m_e = ((tid+1)*block_size)+(m%width);
	else
		m_e = (tid+1)*block_size;
	
	//first part of gemm_cpu_xitao
	int i, j;
	//if(tid ==0){
	for(i = m_s; i < m_e; ++i){
		for(j = 0; j < n; ++j){
            		c[i*ldc + j] *= BETA;
        	}
    	}
	*/

	//NEW DYNAMIC
	int offset = index * len;
	int blocksize = len /(len/2);	
	//int blocksize = len / (width*PSLACK);
        int blocks = len / blocksize;
        //next=0;
	while(1){
		int blockid = next++;
		if(blockid >= blocks) return 0;
		m_s = offset +  blockid*blocksize;
		m_e = (blockid==blocks-1)? offset + len : offset + (blockid+1)*blocksize;
		for(int j = m_s; j < m_e; ++j)
			c[j] *= BETA;
	}
//}// end if
	//gemm_xitao((0,1,m,n,k,1,a,k,b,k,1,c,n));
}//end execute
int CON_TAO4::execute(int threadid){
        int m = l->batch;
        int K = l->inputs;
        int n = l->outputs;
	float *a = net->input;
	float *b = l->weights;
	float *c = l->output;
	int ALPHA =1;
        int BETA = 1;
	int block_size = m/width; // can change to ceil
        int tid = threadid - leader;
//        std::cout <<  "Thread " <<tid <<" entered in TAO4" <<"\n";
	int m_s= tid*block_size;
        int m_e;
        int lda = K;
        int ldb = K;
        int ldc = n;
/*
        if(tid == width-1)
                m_e = ((tid+1)*block_size)+(m%width);
        else
                m_e = (tid+1)*block_size;
	//if(tid==0){
	int i,j,k;
	for(i = m_s; i < m_e; ++i){
       		for(k = 0; k < K; ++k){
            		register float A_PART = ALPHA*a[i*lda+k];
            		for(j = 0; j < n; ++j){
                		c[i*ldc+j] += A_PART*b[k*ldb+j];
			}
        	}
    	}
*/ 
	// NEW DYNAMIC
	int offset = index * len;
	//int blocksize = len / (width*PSLACK);
        int blocksize = len /(len/2);
	int blocks = len / blocksize;
        //next=0;
	while(1){
		int blockid = next++;
		if(blockid >= blocks) return 0;
                m_s = offset +  blockid*blocksize;
                m_e = (blockid==blocks-1)? offset + len :offset + (blockid+1)*blocksize;
		for(int j = m_s; j < m_e; ++j){
			register float sum = 0;
                        for(int k = 0; k < K; ++k){
				sum += ALPHA*a[k]*b[j*ldb + k];
                        }
			c[j] += sum;
         	}
	}
}//end execute

// call GEMM_NT
/*
// TAO 4 let it be serial
int TAO4::execute(int threadid){
    if(l->batch_normalize){
        forward_batchnorm_layer(l, net);
    } else {
        add_bias(l->output, l->biases, l->batch, l->n, l->out_h*l->out_w);
    }

    activate_array(l->output, l->outputs*l->batch, l->activation);
    if(l->binary || l->xnor) swap_binary(&l);

 }//end execute

*/
