#ifndef TAOS_H
#define TAOS_H

#include "tao.h"
#include "darknet.h"
#include "crop_layer.h"
#include "convolutional_layer.h"
#include "dropout_layer.h"
#include "maxpool_layer.h"
#include "connected_layer.h"
#include "softmax_layer.h"
#include "utils.h"
#include "batchnorm_layer.h"
#include "im2col.h"
#include "col2im.h"
#include "blas.h"
#include "gemm.h"
#include <stdio.h>
#include <time.h>


//********************** CROP LAYER TAO  **********************
class CropLayer : public AssemblyTask
{
	public:
		// initialize static parameters
	CropLayer(layer *_l, network *_net) : l(_l),net(_net), AssemblyTask(1)
		{

		}
	int cleanup(){ 	} 
	int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};

//********************** CONVOLUTION HEAD  LAYER TAO  **********************

class ConvolutionLayerHead : public AssemblyTask{
	public:
		//initialize static parameters
	ConvolutionLayerHead(layer *_l, network *_net) :l(_l),net(_net),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};// end class

//********************** CONVOLUTION TAIL  LAYER TAO  **********************

class ConvolutionLayerTail : public AssemblyTask{
	public:
		//initialize static parameters
	ConvolutionLayerTail(layer *_l, network *_net) :l(_l),net(_net),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};// end class


//********************** MAX POOl  LAYER TAO  **********************


class MaxpoolLayer : public AssemblyTask{
	public:
		//initialize static parameters
	MaxpoolLayer(layer *_l, network *_net) :l(_l),net(_net),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};// end class

//********************** CONNECTED HEAD  LAYER TAO  **********************

class ConnectedLayerHead : public AssemblyTask{
	public:
		//initialize static parameters
	ConnectedLayerHead(layer *_l, network *_net) :l(_l),net(_net),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};// end class


//********************** CONNECTED TAIL  LAYER TAO  **********************

class ConnectedLayerTAIL : public AssemblyTask{
	public:
		//initialize static parameters
	ConnectedLayerTAIL(layer *_l, network *_net) :l(_l),net(_net),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};// end class



//********************** DROPOUT  LAYER TAO  **********************


class DropoutLayer : public AssemblyTask{
	public:
		//initialize static parameters
	DropoutLayer(layer *_l, network *_net) :l(_l),net(_net),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};// end class


//********************** SOFTMAX  LAYER TAO  **********************


class SoftmaxLayer : public AssemblyTask{
	public:
		//initialize static parameters
	SoftmaxLayer(layer *_l, network *_net) :l(_l),net(_net),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
	network *net;
};// end class

//********************** GEMM TAO  **********************

class Gemm : public AssemblyTask{
	public:
		//initialize static parameters
	Gemm(int _TA, int _TB, int _M, int _N, int _K, float _ALPHA,float *_A, int _lda, float *_B, int _ldb, float _BETA, float *_C, int _ldc) :TA(_TA), TB(_TB), M(_M), N(_N), K(_K), ALPHA(_ALPHA), A(_A), lda(_lda), B(_B), ldb(_ldb), BETA(_BETA), C(_C), ldc(_ldc),  AssemblyTask(1)
                {

		}
	int cleanup(){ }
        int execute(int threadid);
	//parameters for this TAO
	int TA;
	int TB;
	int M;
	int N;
	int K;
	int ALPHA;
	int A;
	int lda;
	int B;
	int ldb;
	int BETA;
	int C;
	int ldc;
};// end class




#endif
