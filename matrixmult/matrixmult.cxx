#include <iostream>
#include "config.h"
#include "matrixmult.h"

int  matmult(int m_a[][ROW_SIZE], int m_b[][ROW_SIZE], int m_result[][ROW_SIZE]){

	int row = ROW_SIZE;
	int col = COL_SIZE;

	int temp_data[row][col];
	

	for(int i=0; i<col; i++){
		for(int j=0; j<row; j++){
			temp_data[i][j] = 0;
			for(int k=0; k<col; k++){
				temp_data[i][j] +=(m_a[i][k]* m_b[k][j]);
			}
		}
	}

	std::cout << temp_data[0][0] << "\n";
	std::cout << temp_data[0][1] << "\n";
	std::cout << temp_data[1][0] << "\n";
	std::cout << temp_data[1][1] << "\n";

	for(int t=0; t<col; t++){
		for(int p=0; p<row; p++){
			m_result[t][p] = temp_data[t][p];
		 }
	}

	return 0;
}


int main(){//(int** ia, int** mb, int** result){
	
	
	/*int a[2][2] = {{2,1},{1,1}};
	int b[2][2] = {{1,2},{2,1}};
	int c[2][2];


	matmult(a,b,c);
	std::cout << c[0][0] << "\n";
	std::cout << c[0][1] << "\n";
	std::cout << c[1][0] << "\n";
	std::cout << c[1][1] << "\n"; */
	return 0;
}

