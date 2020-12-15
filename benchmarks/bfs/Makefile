# C compiler
CC = g++
CC_FLAGS = -O3 -fopenmp  
CC_FLAGS+=$(CXXFLAGS) -I../../include 
bfs: bfs.o
	$(CC) $(CC_FLAGS) bfs.o -o bfs

bfs_xitao: bfs_xitao.o
	$(CC) $(CC_FLAGS) bfs.o -L../../lib/ -lxitao  -o bfs_xitao

%.o: %.[ch]
	$(CC) $(CC_FLAGS) $< -c

bfs.o: bfs.cpp 
	$(CC) $(CC_FLAGS) bfs.cpp -c

bfs_xitao.o: bfs.cpp 
	$(CC) $(CC_FLAGS) -DUSE_XITAO bfs.cpp -c

all: bfs bfs_xitao

clean:
	rm -f *.o *~ bfs bfs_xitao result.txt 
