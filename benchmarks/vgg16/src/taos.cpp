#include "taos.h"
//********************** CROP LAYER TAO  **********************

        // this assembly can work totally asynchronously
        int CropLayer::execute(int threadid)
        {
        	if(l->delta){
            	fill_cpu(l->outputs * l->batch, 0, l->delta, 1);
        	}
        	int i,j,c,b,row,col;
    		int index;
    		int count = 0;
    		int flip = (l->flip && rand()%2);
    		int dh = rand()%(l->h - l->out_h + 1);
    		int dw = rand()%(l->w - l->out_w + 1);
    		float scale = 2;
    		float trans = -1;
    		if(l->noadjust){
        		scale = 1;
        		trans = 0;
    		}
    		if(!net->train){
        		flip = 0;
        		dh = (l->h - l->out_h)/2;
        		dw = (l->w - l->out_w)/2;
    		}
    		for(b = 0; b < l->batch; ++b){
        		for(c = 0; c < l->c; ++c){
            		for(i = 0; i < l->out_h; ++i){
                		for(j = 0; j < l->out_w; ++j){
                    		if(flip){
                        		col = l->w - dw - j - 1;    
                    		}else{
                        		col = j + dw;
                    		}
                    		row = i + dh;
                    		index = col+l->w*(row+l->h*(c + l->c*b)); 
                    		l->output[count++] = net->input[index]*scale + trans;
                		}
            		}
        		}
    		}
    		    	//// trailling code of netwok's loop
            net->input = l->output;
        if(l->truth) {
            net->truth = l->output;
        }
        }// end execute



//********************** CONVOLUTION  LAYER TAO  **********************


        // this assembly can work totally asynchronously
        int ConvolutionLayerHead:: execute(int threadid)
        {
        	if(l->delta){
            	fill_cpu(l->outputs * l->batch, 0, l->delta, 1);
        	}
        	int i, j;

    fill_cpu(l->outputs*l->batch, 0, l->output, 1);

    if(l->xnor){
        binarize_weights(l->weights, l->n, l->c/l->groups*l->size*l->size, l->binary_weights);
        swap_binary(l);
        binarize_cpu(net->input, l->c*l->h*l->w*l->batch, l->binary_input);
        net->input = l->binary_input;
    }

    int m = l->n/l->groups;
    int k = l->size*l->size*l->c/l->groups;
    int n = l->out_w*l->out_h;
    for(i = 0; i < l->batch; ++i){
        for(j = 0; j < l->groups; ++j){
            float *a = l->weights + j*l->nweights/l->groups;
            float *b = net->workspace;
            float *c = l->output + (i*l->groups + j)*n*m;
            float *im =  net->input + (i*l->groups + j)*l->c/l->groups*l->h*l->w;

            if (l->size == 1) {
                b = im;
            } else {
                im2col_cpu(im, l->c/l->groups, l->h, l->w, l->size, l->stride, l->pad, b);
            }
            gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);
        }
    }
	}// end convolution head

int ConvolutionLayerTail::execute(int threadid){
    if(l->batch_normalize){
        forward_batchnorm_layer(l[0], net[0]);
    } else {
        add_bias(l->output, l->biases, l->batch, l->n, l->out_h*l->out_w);
    }

    activate_array(l->output, l->outputs*l->batch, l->activation);
    if(l->binary || l->xnor) swap_binary(l);

    	//// trailling code of netwok's loop
            net->input = l->output;
        if(l->truth) {
            net->truth = l->output;
        }
        }//end execute


//********************** MAXPOOL LAYER TAO  **********************

        // this assembly can work totally asynchronously
        int MaxpoolLayer::execute(int threadid)
        {
        	if(l->delta){
            	fill_cpu(l->outputs * l->batch, 0, l->delta, 1);
        	}
		// code from forward_maxpool
		int b,i,j,k,m,n;
		int w_offset = -l->pad/2;
    		int h_offset = -l->pad/2;

    		int h = l->out_h;
    		int w = l->out_w;
    		int c = l->c;

    		for(b = 0; b < l->batch; ++b){
        		for(k = 0; k < c; ++k){
            			for(i = 0; i < h; ++i){
                			for(j = 0; j < w; ++j){
                    				int out_index = j + w*(i + h*(k + c*b));
                    				float max = -FLT_MAX;
                    				int max_i = -1;
                    				for(n = 0; n < l->size; ++n){
                        				for(m = 0; m < l->size; ++m){
                            					int cur_h = h_offset + i*l->stride + n;
                            					int cur_w = w_offset + j*l->stride + m;
                            					int index = cur_w + l->w*(cur_h + l->h*(k + b*l->c));
                            					int valid = (cur_h >= 0 && cur_h < l->h &&
                                         			cur_w >= 0 && cur_w < l->w);
                            					float val = (valid != 0) ? net->input[index] : -FLT_MAX;
                            					max_i = (val > max) ? index : max_i;
                            					max   = (val > max) ? val   : max;
                       						 }
								 }
                    					l->output[out_index] = max;
                    					l->indexes[out_index] = max_i;
                					}
            					}
       					 }
    				}

    		// trailling code of netwok's loop
            	net->input = l->output;
        	if(l->truth) {
            	net->truth = l->output;
        	}
        }// end execute


//********************** CONNECTED LAYER TAO  **********************

        // this assembly can work totally asynchronously
        int ConnectedLayerHead::execute(int threadid)
        {
        	if(l->delta){
            	fill_cpu(l->outputs * l->batch, 0, l->delta, 1);
        	}
		//code from forward_connected_layer
		fill_cpu(l->outputs*l->batch, 0, l->output, 1);
	} // end Connected Head
    	//	int m = l->batch;
    	//	int k = l->inputs;
    	//	int n = l->outputs;
    	//	float *a = net->input;
    	//	float *b = l->weights;
    	//	float *c = l->output;
    	//	gemm(0,1,m,n,k,1,a,k,b,k,1,c,n);
    	int ConnectedLayerTAIL::execute(int threadid){
    		if(l->batch_normalize){
        		forward_batchnorm_layer(l[0], net[0]);
    		} else {
        		add_bias(l->output, l->biases, l->batch, l->outputs, 1);
    		}
    		activate_array(l->output, l->outputs*l->batch, l->activation);
    		// trailling code of netwok's loop
        	net->input = l->output;
        	if(l->truth) {
            	net->truth = l->output;
        	}
        }// end execute


//********************** DROPOUT LAYER TAO  **********************

        // this assembly can work totally asynchronously
        int DropoutLayer::execute(int threadid)
        {
        	if(l->delta){
            	fill_cpu(l->outputs * l->batch, 0, l->delta, 1);
        	}
		// code from forward_dropout_layer
		int i;
    		if (!net->train) return;
    		for(i = 0; i < l->batch * l->inputs; ++i){
        		float r = rand_uniform(0, 1);
        		l->rand[i] = r;
        		if(r < l->probability) net->input[i] = 0;
        		else net->input[i] *= l->scale;
    		}
    		// trailling code of netwok's loop
            	net->input = l->output;
        	if(l->truth) {
            	net->truth = l->output;
        	}
        }// end execute


//********************** SOFTMAX LAYER TAO  **********************

        // this assembly can work totally asynchronously
        int SoftmaxLayer::execute(int threadid)
        {
        	if(l->delta){
            	fill_cpu(l->outputs * l->batch, 0, l->delta, 1);
        	}
		// Code from forward_softmax_layer
		if(l->softmax_tree){
        	int i;
        	int count = 0;
        		for (i = 0; i < l->softmax_tree->groups; ++i) {
            			int group_size = l->softmax_tree->group_size[i];
            			softmax_cpu(net->input + count, group_size, l->batch, l->inputs, 1, 0, 1, l->temperature, l->output + count);
            			count += group_size;
        		}
    		} else {
        	softmax_cpu(net->input, l->inputs/l->groups, l->batch, l->inputs, l->groups, l->inputs/l->groups, 1, l->temperature, l->output);
    		}

    		if(net->truth && !l->noloss){
        	softmax_x_ent_cpu(l->batch*l->inputs, l->output, net->truth, l->delta, l->loss);
        	l->cost[0] = sum_array(l->loss, l->batch*l->inputs);
    		}
    		// trailling code of netwok's loop
            	net->input = l->output;
        	if(l->truth) {
            	net->truth = l->output;
        	}
        }// end execute

//********************** GEMM TAO  **********************

int Gemm::execute(int threadid){
    	gemm(TA,  TB,  M, N, K, ALPHA,A,lda, B, ldb,BETA,C,ldc);
}// end execute

