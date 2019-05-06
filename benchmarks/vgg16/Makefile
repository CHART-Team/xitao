GPU=0
CUDNN=0
OPENCV=0
OPENMP=1
DEBUG=0
XITAO=1
ARCH= -gencode arch=compute_30,code=sm_30 \
      -gencode arch=compute_35,code=sm_35 \
      -gencode arch=compute_50,code=[sm_50,compute_50] \
      -gencode arch=compute_52,code=[sm_52,compute_52]
#      -gencode arch=compute_20,code=[sm_20,sm_21] \ This one is deprecated?

# This is what I use, uncomment if you know your arch and want to specify
# ARCH= -gencode arch=compute_52,code=compute_52

VPATH=./src/:./examples
SLIB=libdarknet.so
ALIB=libdarknet.a
EXEC=darknet
OBJDIR=./obj/

CC=gcc
CPP=g++
NVCC=nvcc 
AR=ar
ARFLAGS=rcs
#OPTS=-Ofast
LDFLAGS= -lm -pthread 
COMMON= -Iinclude/ -Isrc/ -fpermissive 
CFLAGS=-Wall -w -Wno-unused-result -Wno-unknown-pragmas  -Wfatal-errors -fPIC
XITAOROOT = ../..
XITAOLIB  = $(XITAOROOT)/lib
ifeq ($(XITAO), 1) 
	include ../../makefile.hardware
	include ../../makefile.sched
	CFLAGS+= -I$(XITAOROOT)/include
	LDFLAGS+= -L$(XITAOLIB) -lxitao
endif

ifeq ($(DEBUG),1)
	OPTS+= -g3
else
	OPTS+= -O3 -mavx
endif

ifeq ($(OPENMP),1) 
	CFLAGS+= -fopenmp
endif
CFLAGS+=$(OPTS)

ifeq ($(OPENCV), 1) 
	COMMON+= -DOPENCV 
	CFLAGS+= -DOPENCV 
	LDFLAGS+= `pkg-config --libs opencv` -lstdc++ 
	COMMON+= `pkg-config --cflags opencv` 
endif

ifeq ($(GPU), 1) 
	COMMON+= -DGPU -I/usr/local/cuda/include/
	CFLAGS+= -DGPU
	LDFLAGS+= -L/usr/local/cuda/lib64 -lcuda -lcudart -lcublas -lcurand
endif

ifeq ($(CUDNN), 1) 
	COMMON+= -DCUDNN 
	CFLAGS+= -DCUDNN
	LDFLAGS+= -lcudnn
endif

OBJ=gemm.o utils.o taos.o conn_xitao.o conv_xitao.o  convolutional_layer.o list.o image.o activations.o box.o im2col.o col2im.o region_layer.o option_list.o detection_layer.o blas.o crop_layer.o dropout_layer.o maxpool_layer.o softmax_layer.o data.o matrix.o network.o connected_layer.o cost_layer.o parser.o normalization_layer.o layer.o local_layer.o  logistic_layer.o activation_layer.o batchnorm_layer.o  tree.o

EXECOBJA=classifier.o darknet.o
ifeq ($(GPU), 1) 
	LDFLAGS+= -lstdc++ 
#OBJ+=convolutional_kernels.o deconvolutional_kernels.o activation_kernels.o im2col_kernels.o col2im_kernels.o blas_kernels.o crop_layer_kernels.o dropout_layer_kernels.o maxpool_layer_kernels.o avgpool_layer_kernels.o
endif

EXECOBJ = $(addprefix $(OBJDIR), $(EXECOBJA))
OBJS = $(addprefix $(OBJDIR), $(OBJ))
DEPS = $(wildcard src/*.h) Makefile include/darknet.h

all: obj backup results $(SLIB) $(ALIB) $(EXEC)
#all: obj  results $(SLIB) $(ALIB) $(EXEC)


$(EXEC): $(EXECOBJ) $(ALIB)
	$(CPP) $(COMMON) $(CFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(ALIB)


$(ALIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(SLIB): $(OBJS)
	$(CPP) $(CFLAGS) -shared $^ -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp $(DEPS)
	$(CPP) $(COMMON) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)%.o: %.c $(DEPS)
	$(CPP) $(COMMON) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)%.o: %.cu $(DEPS)
	$(NVCC) $(ARCH) $(COMMON) --compiler-options "$(CFLAGS)" -c $< -o $@

obj:
	mkdir -p obj
backup:
	mkdir -p backup
results:
	mkdir -p results

.PHONY: clean

clean:
	rm -rf $(OBJS) $(SLIB) $(ALIB) $(EXEC) $(EXECOBJ) $(OBJDIR)/*

