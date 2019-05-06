#include "taos_c.h"
#include "tao.h"
//extern "C"{
void *new_CropLayer(const crop_layer l, network net){
	return new CropLayer(l,net);
	}
void *new_ConvolutionLayer(convolutional_layer l, network net){
	new ConvolutionLayer(l,net);
}

void makeedge_crop_conv(void *crp_layer , void *conv_layer){

CropLayer *crp = (CropLayer *) crp_layer;
ConvolutionLayer *conv = (ConvolutionLayer *) conv_layer; 
crp->make_edge(conv);

}
//}//end extern C
