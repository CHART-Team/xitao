//this file is interface between c and c++ code
//example  is taken from : https://bytes.com/topic/c/insights/921728-calling-c-class-methods-c
//
//
#include "taos.h"
#include "convolutional_layer.h"
#include "crop_layer.h"
#include "tao.h" 
//#ifdef __cplusplus
//extern "C"{
//#endif

 void *new_CropLayer(layer l, network net);
 void *new_ConvolutionLayer(layer l, network net);
 void makeedge_crop_conv(void *crp_layer , void *conv_layer);
//#ifdef __cplusplus
//}// end extern:wq
//
//
//#endif
