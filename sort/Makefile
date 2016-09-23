# makefile for simple sorting examples from cilksort
#

include ../makefile.inc
CFLAGS := -std=gnu11 -g3 -O3 ${EXTRAEINC}
CXXFLAGS = --std=c++11 -pthread  -g3 -O3 -DPAUSE ${EXTRAEINC}

include makefile.sort

CXXFLAGS += -DLEVEL1=$(LEVEL1) -DLEVEL2=$(LEVEL2) -DLEVEL3=$(LEVEL3) -DLEVEL4=$(LEVEL4) -DLEVEL5=$(LEVEL5)  -DLEVEL6=$(LEVEL6) -DLEVEL7=$(LEVEL7) 
CXXFLAGS += -DDYNW1=$(DYNW1)   -DDYNW2=$(DYNW2)   -DDYNW3=$(DYNW3)   -DDYNW4=$(DYNW4)   -DDYNW5=$(DYNW5) -DDYNW6=$(DYNW6)   -DDYNW7=$(DYNW7)

LFLAGS :=

LOIFLAGS := -DMERGEBLOCK=4096


include ../loi/loikrd.defs

# options for LOI/KRD
ifdef LOI
LOI_SOURCES =  ../loi/loi.c ../loi/krd_common.c ../loi/krd_trace.c ../loi/krd_bench.c
LOI_OBJS    =  ../loi/loi.o ../loi/krd_common.o ../loi/krd_trace.o ../loi/krd_bench.o
LOI_DIR = ../loi
LOI_HDR = ../loi/loi.h ../loi/krd.h
else
LOI_DIR = ../loi
endif

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@
 
%.o : %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@
 

CXXFLAGS += -I$(LOI_DIR)


TAO_OBJS=../tao_sched.o taosort.o sortlib.o
TAO4096_OBJS=../tao_sched.o taosort4096.o sortlib.o

taosort:  $(TAO_OBJS)  $(LOI_OBJS)
	$(CXX) $^ -o $@${SUFFIX} ${CXXFLAGS} $(LFLAGS)  -fpermissive -DTAO_SORT -L$(TBBLIB) -ltbbmalloc_proxy -ltbbmalloc -Wl,-rpath=$(TBBLIB) $(EXTRAELIBS)

taosort4096:  $(TAO4096_OBJS)  $(LOI_OBJS)
	$(CXX) $^ -o $@${SUFFIX} ${CXXFLAGS} $(LFLAGS)  -fpermissive -DTAO_SORT -L$(TBBLIB) -ltbbmalloc_proxy -ltbbmalloc -Wl,-rpath=$(TBBLIB)

tbbsort: tbbsort.cxx $(LOI_OBJS)
	$(CXX) $^ -o $@ ${CXXFLAGS} ${TBBLFLAGS} $(LOIFLAGS) $(LFLAGS) -fpermissive

qthsort: qthsort.cxx $(LOI_OBJS)
	$(CXX) $^ -o $@ ${CXXFLAGS} ${QTHLFLAGS} $(LOIFLAGS) $(LFLAGS) -fpermissive

qthserial: qthsort.cxx $(LOI_SOURCES)
	$(CXX) $^ -o $@ ${CXXFLAGS} $(LOIFLAGS) $(LFLAGS) -fpermissive

clean:  
	rm -f $(TAO_OBJS) $(LOI_OBJS)
