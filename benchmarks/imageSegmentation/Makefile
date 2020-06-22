CC = g++
#CFLAGS  = -w  -std=c++0x -g -Wall -O3 -fopenmp -DCRIT_PERF_SCHED -pthread -I/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/include/ -L/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/lib
CFLAGS  = -w -std=c++0x -g -Wall -O3 -fopenmp -pthread -I/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/include/ -L/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/lib
IMAGE = coffee.jpg
imageSegmentation:  image_seg.o 
	$(CC) $(CFLAGS) -o imageSegmentation image_seg.o -lxitao

image_seg.o:  image_seg.cpp stb_image.h stb_image_write.h 
	$(CC) $(CFLAGS) -c image_seg.cpp

run: 
	 ./imageSegmentation -i ./images/$(IMAGE) -s 2 -t 4 -o images/out.jpg
clean:
	rm image_seg.o imageSegmentation
