#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdio>
using namespace std ;

int main (){
	vector<int> temp_vector ;
	int data_num ;

	scanf("%d" , &data_num) ;
	for (int i = 0 ; i < data_num ; i++){
		int temp ;
		scanf("%d" , &temp) ;
		temp_vector.push_back(temp) ;
	}

	sort(temp_vector.begin() , temp_vector.end()) ;
	int i ;
	for (i = 0 ; i < data_num - 1 ; i++)
		printf("%d " , temp_vector[i]) ;
	printf("%d\n" , temp_vector[i]) ;
}