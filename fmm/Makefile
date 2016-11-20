.SUFFIXES: .cxx .o

#TARGET = OPENMP
#TARGET=TBB
TARGET=TAO


ifeq ($(TARGET), OPENMP) 
LIST_SCHEDULE := -DOPENMP -fopenmp 
PROGRAM_OBJS = test.o 
else ifeq ($(TARGET), TBB)
PROGRAM_OBJS = test.o 
LIST_SCHEDULE:=-DTASK_GROUP 
else ifeq ($(TARGET), TAO)
LIST_SCHEDULE := -DTAO -pthread -DTAO_PLACES -DLOCK_FREE_QUEUE -DSPINLOCK
PROGRAM_OBJS = test.o ../tao_sched.o
else
$(info "Invalid TARGET selected. I saw " $TARGET)
endif

include ../makefile.inc
CXX = g++ -std=c++11 -O3 -g3 -ffast-math -DLIST $(LIST_SCHEDULE)
include ../loi/loikrd.defs

# options for LOI/KRD
#
ifdef LOI
LOI_SOURCES =  ../loi/loi.c ../loi/krd_common.c ../loi/krd_trace.c ../loi/krd_bench.c
LOI_OBJS    =  ../loi/loi.o ../loi/krd_common.o ../loi/krd_trace.o ../loi/krd_bench.o
LOI_DIR = ../loi
LOI_HDR = ../loi/loi.h ../loi/krd.h
else
LOI_DIR = ../loi
endif

CXXFLAGS += -I$(LOI_DIR)  -I..

.cxx.o  :
	$(CXX) -c $^ -o $@ $(LOIFLAGS)  $(CXXFLAGS) $(TBBLFLAGS) 

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@

test: $(PROGRAM_OBJS) $(LOI_OBJS)
	$(CXX) $?  $(CXXFLAGS) $(TBBLFLAGS) 
	./a.out

clean:
	$(RM) *.o a.out taofmm $(PROGRAM_OBJS) $(LOI_OBJS)

