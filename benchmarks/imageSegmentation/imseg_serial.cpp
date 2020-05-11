#include <stdio.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <algorithm>
#include <sys/time.h>
#include <xitao.h>

#include <unordered_map>

using namespace std;
#define MATCH(s) (!strcmp(argv[ac], (s)))




static const double kMicro = 1.0e-6;
double getTime()
{
	struct timeval TV;
	struct timezone TZ;

	const int RC = gettimeofday(&TV, &TZ);
	if(RC == -1) {
		printf("ERROR: Bad call to gettimeofday\n");
		return(-1);
	}

	return( ((double)TV.tv_sec) + kMicro * ((double)TV.tv_usec) );

}

void imageSegmentation(int *labels, unsigned char *data, int width, int height, int pixelWidth, int Threshold)
{
	int maxN = std::max(width,height);
	int phases = (int) ceil(log(maxN)/log(2)) + 1;
	cout<<"Phases = "<<phases;
  	for(int pp = 0; pp <= phases; pp++)
	  { 
	    //LOOP NEST 1
	    // first pass over the image: Find neighbors with better labels.
	    for (int i = height - 1; i >= 0; i--) {
	      for (int j = width - 1; j >= 0; j--) {

		int idx = i*width + j;
		int idx3 = idx*pixelWidth;

		if (labels[idx] == 0)
		  continue;
		  
		int ll = labels[idx]; // save previous label

		// pixels are stored as 3 ints in "data" array. we just use the first of them.
		// Compare with each neighbor:east, west, south, north, ne, nw, se, sw

		//west
		if (j != 0 && abs((int)data[(i*width + j - 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[i*width + j - 1]);

		//east
		if (j != width-1 && abs((int)data[(i*width + j + 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[i*width + j + 1]);
		
		//south 
		if(i != height-1 && abs((int)data[((i+1)*width + j)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i+1)*width + j]);

		//north 
		if(i != 0 && abs((int)data[((i-1)*width + j)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i-1)*width + j]);

		//south east
		if(i != height-1 && j != width-1 && abs((int)data[((i+1)*width + j + 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i+1) * width + j + 1]);

		//north east
		if(i != 0 && j != width-1 && abs((int)data[((i-1)*width + j + 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i-1) * width + j + 1]);

		//south west 
		if(i != height-1 && j!= 0 && abs((int)data[((i+1)*width + j - 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i+1) * width + j - 1]);

		//north west
		if(i != 0 && j != 0 && abs((int)data[((i-1)*width + j - 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i-1) * width + j - 1]);

		// if label assigned to this pixel during this "follow the pointers" step is worse than one of its neighbors, 
		// then that means that we're converging to local maximum instead
		// of global one. To correct this, we replace our root pixel's label with better newly found one.
		if (ll < labels[idx]) {
		  labels[ll - 1] = std::max(labels[idx],labels[ll - 1]);		  
		}
	      }
	    }
	    
	    
	    //LOOP NEST 2
	    // Second pass on the labels. propagates the updated label of the parent to the children.
	    for (int i = 0; i < height; i++) {
	      for (int j = 0; j < width; j++) {

		int idx = i*width + j;

		if (labels[idx] != 0) {
		  labels[idx] = std::max(labels[idx], labels[labels[idx] - 1]);
		  // subtract 1 from pixel's label to convert it to array index
		}
	      }
	    }

	  }

}



void segmentation(int *labels, unsigned char *data, int width, int height, int pixelWidth, int Threshold, int start, int end)
{ 
	    //LOOP NEST 1
	    // first pass over the image: Find neighbors with better labels.
	    for (int i = end - 1; i >= start; i--) {
	      for (int j = width - 1; j >= 0; j--) {

		int idx = i*width + j;
		int idx3 = idx*pixelWidth;

		if (labels[idx] == 0)
		  continue;
		  
		int ll = labels[idx]; // save previous label

		// pixels are stored as 3 ints in "data" array. we just use the first of them.
		// Compare with each neighbor:east, west, south, north, ne, nw, se, sw

		//west
		if (j != 0 && abs((int)data[(i*width + j - 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[i*width + j - 1]);

		//east
		if (j != width-1 && abs((int)data[(i*width + j + 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[i*width + j + 1]);
		
		//south 
		if(i != height-1 && abs((int)data[((i+1)*width + j)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i+1)*width + j]);

		//north 
		if(i != 0 && abs((int)data[((i-1)*width + j)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i-1)*width + j]);

		//south east
		if(i != height-1 && j != width-1 && abs((int)data[((i+1)*width + j + 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i+1) * width + j + 1]);

		//north east
		if(i != 0 && j != width-1 && abs((int)data[((i-1)*width + j + 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i-1) * width + j + 1]);

		//south west 
		if(i != height-1 && j!= 0 && abs((int)data[((i+1)*width + j - 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i+1) * width + j - 1]);

		//north west
		if(i != 0 && j != 0 && abs((int)data[((i-1)*width + j - 1)*pixelWidth] - (int)data[idx3]) < Threshold)
		  labels[idx] = std::max(labels[idx], labels[(i-1) * width + j - 1]);

		// if label assigned to this pixel during this "follow the pointers" step is worse than one of its neighbors, 
		// then that means that we're converging to local maximum instead
		// of global one. To correct this, we replace our root pixel's label with better newly found one.
		if (ll < labels[idx]) {
		  labels[ll - 1] = std::max(labels[idx],labels[ll - 1]);		  
		}
	      }
	    }

}

//------------------------ XITAO CLASSES -----------------------------------------------------
class Segmentation_loop1 : public AssemblyTask 
{
public: 
    
  Segmentation_loop1(int *_labels, unsigned char *_data, int _w, int _h, int _pixelWidth, int _Threshold, int height_segment, int _h_start, int _h_end) :
  labels(_labels),
  data(_data),
  w(_w),
  h(_h),
  pixelWidth(_pixelWidth),
  Threshold(_Threshold),
  h_start(_h_start),
  h_end(_h_end),
  AssemblyTask(1) 
  {  
  	next = 0;
  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO. For this TAO, we let logical core 0 only do the addition to avoid reduction
  */
  void execute(int threadid)
  {
    int work_segment = height_segment/width;
			 while(1){
			 	int work = next++;
			 	if(work >= width) break;
			 	int start = h_start + work *work_segment;
			 	int end = (work == width-1) ? h_end : start + work_segment; // for last thread entering work have to do a little more work
				segmentation(labels, data, w, h, pixelWidth, Threshold, start, end);
			}
  }
  // Class variables
  atomic<int> next;
  int *labels; 
  unsigned char *data;
  int w; 
  int h; 
  int pixelWidth; 
  int Threshold;
  int h_start, h_end, height_segment;
};


class Segmentation_loop2 : public AssemblyTask 
{
public: 
    
  Segmentation_loop2(int *_labels, int _h, int _w) :labels(_labels), h(_h), w(_w), AssemblyTask(1) 
  {  

  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO. For this TAO, we let logical core 0 only do the addition to avoid reduction
  */
  void execute(int threadid)
  {
    if(threadid == leader){
    	//LOOP NEST 2
	    // Second pass on the labels. propagates the updated label of the parent to the children.
	    for (int i = 0; i < h; i++) {
	      for (int j = 0; j < w; j++) {
			int idx = i*w + j;

			if (labels[idx] != 0) {
		  		labels[idx] = std::max(labels[idx], labels[labels[idx] - 1]);
		  		// subtract 1 from pixel's label to convert it to array index
			}
	      }
	    }
    }
  }
  // Class variables
  int *labels;
  int h, w;
};

class Starter : public AssemblyTask 
{
public: 
    
  Starter() : AssemblyTask(1) 
  {  

  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }
  void execute(int threadid)
  {}

};

//--------------------------------------------------------------------------------------------

int main(int argc,char **argv)
{
	int width,height;
	int pixelWidth;
	int Threshold = 3;
	int numThreads = 1;
	int seed =1 ;
	const char *filename = "input.png";
	const char *outputname = "output.png";

	// Parse command line arguments
	if(argc<2)
	  {
	    printf("Usage: %s [-i < filename>] [-s <threshold>] [-t <numThreads>] [-o outputfilename]\n",argv[0]);
	    return(-1);
	  }
	for(int ac=1;ac<argc;ac++)
	  {
	    if(MATCH("-s")) {Threshold = atoi(argv[++ac]);}
	    else if(MATCH("-t")) {numThreads = atoi(argv[++ac]);}
	    else if(MATCH("-i"))  {filename = argv[++ac];}
	    else if(MATCH("-o"))  {outputname = argv[++ac];}
	    else {
	      printf("Usage: %s [-i < filename>] [-s <threshold>] [-t <numThreads>] [-o outputfilename]\n",argv[0]);
	      return(-1);
	    }
	  }
	
	printf("Reading image...\n");
	unsigned char *data = stbi_load(filename, &width, &height, &pixelWidth, 0);
	if (!data) {
		fprintf(stderr, "Couldn't load image.\n");
		return (-1);
	}

	printf("Image Read. Width : %d, Height : %d, nComp: %d\n",width,height,pixelWidth);

	int *labels = (int *)malloc(sizeof(int)*width*height);
	unsigned char *seg_data = (unsigned char *)malloc(sizeof(unsigned char)*width*height*3);


	

	printf("Applying segmentation...\n");

	double start_time = getTime(); 

	//Intially each pixel has a different label
	for(int i = 0; i < height; i++)
	  {
	    for(int j = 0; j < width; j++)
	      {
		int idx = (i*width+j);
		int idx3 = idx*pixelWidth;

		labels[idx] = 0;

		//comment this line if you want to label background pixels as well
		if((int)data[idx3] == 0) 
		  continue;

		//labels are positive integers
		labels[idx] = idx + 1;
	      }
	  }
	
	//Now perform relabeling 
	//imageSegmentation(labels,data,width,height,pixelWidth,Threshold);
	gotao_init();
	gotao_start();
	int maxN = std::max(width,height);
	int phases = (int) ceil(log(maxN)/log(2)) + 1;
	int num_segments = 8;
	int height_segment = height/num_segments;
	int h_start, h_end; 
	Segmentation_loop2 *sl2[phases];
	Starter *starter = new Starter();
	for(int pp = 0; pp <= phases; pp++){
		//printf("pp = %d", pp);
		sl2[pp] = new Segmentation_loop2(labels, height, width);
		for(int hh = 0; hh < num_segments; hh++){
			Segmentation_loop1 *sl1[hh];
			h_start = hh*height_segment;
			h_end = (hh == height_segment-1) ? height : h_start + height_segment;
			sl1[hh] = new Segmentation_loop1(labels,data,width,height,pixelWidth,Threshold, height_segment, h_start, h_end);
			//Building DAG
			if(pp == 0)
				starter->make_edge(sl1[hh]);
			if(pp > 0)
				sl2[pp-1]->make_edge(sl1[hh]);
			sl1[hh]->make_edge(sl2[pp]);
		}
		
	}
	printf("Dag created");
	gotao_push(starter);
	gotao_fini();
	
	double stop_time = getTime();
	double segTime = stop_time - start_time;

	std::unordered_map<int, int> red;
	std::unordered_map<int, int> green;
	std::unordered_map<int, int> blue;
	std::unordered_map<int, int> count;

	srand(seed);
	start_time = getTime();
	int clusters = 0;
	int min_cluster = height*width;
	int max_cluster = -1;
	double avg_cluster = 0.0;
	
	//LOOP NEST 3
	//Now we will assign colors to labels 
	for (int i = 0; i < height; i++) {
	  for (int j = 0; j < width; j++) {
	    int label = labels[i*width+j];

	    if(label == 0) //background 
	      {
		red[0] = 0;
		green[0] = 0;
		blue[0] = 0;
		
		}
	    else {
	      //if this is a new label, we need to assign a color
	      if(red.find(label) == red.end())
		{
		  clusters++;
		  count[label] = 1;
		  
		  red[label] = (int)random()*255;
		  green[label] = (int)random()*255;
		  blue[label] = (int)random()*255;
		}
	      else
		count[label]++;
	    }
	  }
	}
	//This loop can be parallelized into XiTAO
	//LOOP NEST 4
	for (int i = 0; i < height; i++) {
	  for (int j = 0; j < width; j++) {

	    int label = labels[i*width+j];
	    seg_data[(i*width+j)*3+0] = (char)red[label];
	    seg_data[(i*width+j)*3+1] = (char)blue[label];
	    seg_data[(i*width+j)*3+2] = (char)green[label];	    
	  }
	}

	for ( auto it = count.begin(); it != count.end(); ++it )
	  {
	    min_cluster = std::min( min_cluster, it->second);
	    max_cluster = std::max( max_cluster, it->second);
	    avg_cluster += it->second;
	  }
	
	stop_time = getTime();
	double colorTime = stop_time - start_time;
	
	printf("Segmentation Time (sec): %f\n", segTime);
	printf("Coloring Time     (sec): %f\n", colorTime);
	printf("Total Time        (sec): %f\n", colorTime + segTime);
	printf("-----------Statisctics---------------\n");
	printf("Number of Clusters   : %d\n", clusters);
	printf("Min Cluster Size     : %d\n", min_cluster);
	printf("Max Cluster Size     : %d\n", max_cluster);
	printf("Average sum          : %f\n", avg_cluster);
	printf("Average Cluster Size : %f\n", avg_cluster/clusters);
		
	printf("Writing Segmented Image...\n");
	stbi_write_png(outputname, width, height, 3, seg_data, 0);
	stbi_image_free(data);
	free(seg_data);
	free(labels);

	printf("Done...\n");
	return 0;
}


/*
STANDARD INPUT: -i ./images/coffee.jpg -s 2 -t 4 -o images/out.jpg
STANDARD OUTPUT:
Reading image...
Image Read. Width : 5184, Height : 3456, nComp: 3
Applying segmentation...
Segmentation Time (sec): 10.361304
Coloring Time     (sec): 3.554706
Total Time        (sec): 13.916010
-----------Statisctics---------------
Number of Clusters   : 1293114
Min Cluster Size     : 1
Max Cluster Size     : 4310023
Average sum          : 17915902.000000
Average Cluster Size : 13.854851
Writing Segmented Image...
Done...
*/
