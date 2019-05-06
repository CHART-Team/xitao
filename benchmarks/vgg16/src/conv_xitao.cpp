#include "conv_xitao.h"
#if defined(CRIT_PERF_SCHED)
float TAO3::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
float TAO4::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
float TAO_barrier::time_table[GOTAO_NTHREADS][GOTAO_NTHREADS];
#endif
// SERIEL CODE STUBS
int seriel_TAO1::execute(int threadid){

	fill_cpu(l->outputs*l->batch, 0, l->output, 1);

    if(l->xnor){
        binarize_weights(l->weights, l->n, l->c/l->groups*l->size*l->size, l->binary_weights);
        swap_binary(l);
        binarize_cpu(net->input, l->c*l->h*l->w*l->batch, l->binary_input);
        net->input = l->binary_input;
    }
        int m = l->n;
        int k = l->size*l->size*l->c;
        int n = l->out_w*l->out_h;
        float *a = l->weights;
        float *c = l->output;
        float *b = net->workspace;
        float *im = net->input;
        if (l->size == 1) {
                b = im;
        }
        else{
                im2col_cpu(im, l->c, l->h, l->w, l->size, l->stride, l->pad, b);
        }
	net->gemm_b = b; // passing updated b pointer which is required in GEMM
}//end execute

int seriel_TAO5::execute(int threadid){

    if(l->batch_normalize){
        forward_batchnorm_layer(l[0], net[0]);
    } else {
        add_bias(l->output, l->biases, l->batch, l->n, l->out_h*l->out_w);
    }
    activate_array(l->output, l->outputs*l->batch, l->activation);
    if(l->binary || l->xnor) swap_binary(l);

}//end execute






// TAO 1
int TAO1::execute(int threadid){

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

// TAO 2
int TAO2::execute(int threadid){

    if(l->xnor){
        //binarize_weights(l->weights, l->n, l->c/l->groups*l->size*l->size, l->binary_weights);
        float *weights = l->weights;
	int n= l->n;
	int size= l->c/l->groups*l->size*l->size;
	float *binary=l->binary_weights;
	// xitao version of binarize_weights
	// each thread will get average number of chunks
	// take mean and replace with binarized value.
	int n_t = n/width; // can change to ceil
	int tid = threadid- leader;
//	std::cout <<  "Thread " <<tid <<" entered in TAO2" <<"\n";
	int i, f;
	int start;
        int end;
/*        start= tid*n_t;
        if(tid == width-1)
                end= ((tid+1)*n_t)+(n % width); // add remainder
        else
                end= ((tid+1)*n_t);
    	for(f = start; (f < n)  && (f < end); ++f){
        	float mean = 0;
        	for(i = 0; i < size; ++i){
            		mean += fabs(weights[f*size + i]);
        	}
        	mean = mean / size;
        	for(i = 0; i < size; ++i){
            		binary[f*size + i] = (weights[f*size + i] > 0) ? mean : -mean;
        	}
	}
	for(int i = start; (i < n)  && (i < end); ++i){
                l->binary_input[i] = (net->input[i] > 0) ? 1 : -1;} 
*/
	// NEW DYNAMIC
	int m_s, m_e;
        while(1){
                 int blockid = next++;
                 if(blockid >= blocks) return 0;
                m_s = blockid*blocksize;
                m_e = (blockid==blocks-1)? n : (blockid+1)*blocksize;
		for(f = m_s; f < m_e; ++f){
                	float mean = 0;
                	for(i = 0; i < size; ++i){
                        	mean += fabs(weights[f*size + i]);
                	}
                mean = mean / size;
                for(i = 0; i < size; ++i){ 
                        binary[f*size + i] = (weights[f*size + i] > 0) ? mean : -mean;
                	}
        	}
        for(int i = m_s; i< m_e ; ++i){
                l->binary_input[i] = (net->input[i] > 0) ? 1 : -1;}
	}//end while   	
} 
}//TAO2_1
/*int TAO2_2::execute(int threadid){

if(l->xnor){
	int n= l->n;
	int n_t = n/width; // can change to ceil
        int tid = threadid- leader;
	int start;
        int end;
        start= tid*n_t;
        if(tid == width-1)
                end= ((tid+1)*n_t)+(n % width); // add remainder
        else
                end= ((tid+1)*n_t);
        //binarize_cpu(net->input, l->c*l->h*l->w*l->batch, l->binary_input);
//        float *input=net->input;
//	binary= l->binary_input;
	for(int i = start; (i < n)  && (i < end); ++i){
        	l->binary_input[i] = (net->input[i] > 0) ? 1 : -1;
	}
    }
}//end execute
*/
int TAO3::execute(int threadid){
	int m = l->n;
	int K = l->size*l->size*l->c;
	int n = l->out_w*l->out_h;	
	//float *a = l->weights;
	//float *b = net->workspace;
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
	int ldb = n;
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
	//int blocksize = len / (width*PSLACK);
	int blocksize = len /(len/2);
        int blocks = len / blocksize;
        //:wq
        //next=0;
	int offset = index * len;
	while(1){
		 int blockid = next++;
                 if(blockid >= blocks) return 0;
		m_s =offset +  blockid*blocksize;
		m_e = (blockid==blocks-1)? offset + len : offset +  (blockid+1)*blocksize;
		
		for(int i = m_s; i< m_e; i++)
			for(int j = 0; j < n; ++j){
                        c[i*ldc + j] *= BETA;
                }
	}
//}// end if
	//gemm_xitao(0,0,m_s, m_e,n,k,1,l->weights,k,b,n,1,l->output,n);
}//end execute
int TAO4::execute(int threadid){
        int m = l->n;
        int K = l->size*l->size*l->c;
        int n = l->out_w*l->out_h;
	float *a = l->weights;
	float *c = l->output;
	float *b = net->gemm_b;
//	float *b = net->workspace;
	int ALPHA =1;
        int BETA = 1;
	int block_size = m/width; // can change to ceil
        int tid = threadid - leader;
//        std::cout <<  "Thread " <<tid <<" entered in TAO4" <<"\n";
	int m_s= tid*block_size;
        int m_e;
        int lda = K;
        int ldb = n;
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
	int blocksize = len / (len/2);
        int blocks = len / blocksize;
        //next=0;
	int offset = index * len;
	//cout<<"\nWIDTH = "<<width<<" blocksize = "<<blocksize<<endl;
	// NEW DYNAMIC
	while(1){
		int blockid = next++;
		if(blockid >= blocks) return 0;
                
		m_s = offset + blockid*blocksize;
                m_e = (blockid==blocks-1)? offset + len : offset + (blockid+1)*blocksize;

               for(int i = m_s; i< m_e; i++){
			for(int k = 0; k < K; ++k){
                        	register float A_PART = ALPHA*a[i*lda+k];
                        	for(int j = 0; j < n; ++j){
                                	c[i*ldc+j] += A_PART*b[k*ldb+j];
                        	}
         		}
	       	}
        }
}//end execute


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

int TAO_barrier::execute(int threadid){
  if(++calls > 1) {
   return 0;
  }


gotao_barrier();
}


