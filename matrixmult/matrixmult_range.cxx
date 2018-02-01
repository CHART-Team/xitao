#include <iostream>
#include "config.h"


int  matmult(int mini, int maxi, int minj, int maxj, int mink, int maxk,  int m_a[COL_SIZE][ROW_SIZE], int m_b[COL_SIZE][ROW_SIZE], int m_result[COL_SIZE][ROW_SIZE]){


	int temp_data[ROW_SIZE][COL_SIZE];
	

	for(int i=mini; i<maxi; i++){
		for(int j=minj; j<maxj; j++){
			temp_data[i][j] = 0;
			for(int k=mink; k<maxk; k++){
				temp_data[i][j] +=(m_a[i][k]* m_b[k][j]);
			}
			m_result[i][j] += temp_data[i][j];
		}
	}



	return 0;
}


int main(){	
	
	int a[ROW_SIZE][COL_SIZE] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
	int b[ROW_SIZE][COL_SIZE] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
	int c[ROW_SIZE][COL_SIZE] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};



	//matmult(0,2,0,2,0,2,a,b,c);
	
	int stepsize=2;

	//Initalization loops
	//For tao -> for each x,y create tao
	//For each tao -> create z loop and spawn tasks
	for(int x=0; x<ROW_SIZE; x+=stepsize){
		for(int y=0; y<COL_SIZE; y+=stepsize){
			for(int z=0; z<ROW_SIZE; z+=stepsize){
				matmult(x, x+stepsize, y, y+stepsize, z, z+stepsize, a,b,c);
			}
		}
	}


	std::cout << c[0][0] << c[0][1] << c[0][2] << c[0][3] << "\n";
	std::cout << c[1][0] << c[1][1] << c[1][2] << c[1][3] << "\n";
	std::cout << c[2][0] << c[2][1] << c[2][2] << c[2][3] << "\n";
	std::cout << c[3][0] << c[3][1] << c[3][2] << c[3][3] << "\n";
	return 0;
}

