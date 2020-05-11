CC = g++
#CFLAGS  = -w  -std=c++0x -g -Wall -O3 -fopenmp -DCRIT_PERF_SCHED -pthread -I/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/include/ -L/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/lib
CFLAGS  = -w -std=c++0x -g -Wall -O3 -fopenmp -pthread -I/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/include/ -L/cephyr/users/pirah/Hebbe/Documents/homeathon/xitao/lib
IMAGE = coffee.jpg
imageSegmentation:  imseg_serial.o 
	$(CC) $(CFLAGS) -o imageSegmentation imseg_serial.o -lxitao

imseg_serial.o:  imseg_serial.cpp stb_image.h stb_image_write.h 
	$(CC) $(CFLAGS) -c imseg_serial.cpp

run: 
	 ./imageSegmentation -i ./images/$(IMAGE) -s 2 -t 4 -o images/out.jpg
clean:
	rm imseg_serial.o
