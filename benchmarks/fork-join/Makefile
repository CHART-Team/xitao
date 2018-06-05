# makefile for simple sorting examples from cilksort
#

include ../../makefile.inc
CFLAGS := -std=gnu11 -g3 -O0 ${EXTRAEINC}
CXXFLAGS = --std=c++11 -pthread  -g3 -O0 ${EXTRAEINC}

include Makefile.forkjoin

CXXFLAGS += -DLEVEL1=$(LEVEL1) -DLEVEL2=$(LEVEL2) -DLEVEL3=$(LEVEL3) -DLEVEL4=$(LEVEL4) -DLEVEL5=$(LEVEL5)  -DLEVEL6=$(LEVEL6) -DLEVEL7=$(LEVEL7) 
CXXFLAGS += -DDYNW1=$(DYNW1)   -DDYNW2=$(DYNW2)   -DDYNW3=$(DYNW3)   -DDYNW4=$(DYNW4)   -DDYNW5=$(DYNW5) -DDYNW6=$(DYNW6)   -DDYNW7=$(DYNW7)

LFLAGS :=

LOIFLAGS := -DMERGEBLOCK=4096


include ../../loi/loikrd.defs

# options for LOI/KRD
ifdef LOI
LOI_SOURCES =  ../../loi/loi.c ../../loi/krd_common.c ../../loi/krd_trace.c ../../loi/krd_bench.c
LOI_OBJS    =  ../../loi/loi.o ../../loi/krd_common.o ../../loi/krd_trace.o ../../loi/krd_bench.o
LOI_DIR = ../../loi
LOI_HDR = ../../loi/loi.h ../../loi/krd.h
else
LOI_DIR = ../../loi
endif

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@
 
%.o : %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@
 

CXXFLAGS += -I$(LOI_DIR)


LIBS = -lm ${EXTRAELIBS}
PROGRAM_OBJS=fork-join.o ../../tao_sched.o ../sortlib.o

all: fork-join

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

taosort:  $(TAO_OBJS)  $(LOI_OBJS)
	$(CXX) $^ -o $@${SUFFIX} ${CXXFLAGS} $(LFLAGS)  -fpermissive -DTAO_SORT -L$(TBBLIB) -Wl,-rpath=$(TBBLIB) $(EXTRAELIBS)

sort:  $(TAO_OBJS)  $(LOI_OBJS)
	$(CXX) $^ -o $@${SUFFIX} ${CXXFLAGS} $(LFLAGS)  -fpermissive -DTAO_SORT -L$(TBBLIB) -Wl,-rpath=$(TBBLIB) $(EXTRAELIBS)



fork-join: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS) ${LIBS} -L$(TBBLIB) -fpermissive -DTAO_SORT -Wl,-rpath=$(TBBLIB)

clean:
	rm $(PROGRAM_OBJS)
