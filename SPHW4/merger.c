#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

int total_data_size ;
int data[100000000] ;
pthread_t tid[100000000] ;
typedef struct argument{
	int first_head ;
	int first_tail ;
	int second_head ;
	int second_tail ;
}Argument ;

typedef struct return_struct{
	int duplicate ;
	int first_size ;
	int second_size ;
}Return ;

void *merge(void *arg){
	int first_head = ((Argument *)arg)->first_head ;
	int first_tail = ((Argument *)arg)->first_tail ;
	int second_head = ((Argument *)arg)->second_head ;
	int second_tail = ((Argument *)arg)->second_tail ;
	int first_size = first_tail - first_head + 1 ; 
	int second_size = second_tail - second_head + 1 ; 

	//printf("head : %d , %d\n" , first_head , second_head) ;

	Return *return_value = (Return *)malloc(sizeof(Return));
	return_value->first_size = first_size ;
	return_value->second_size = second_size ;
	return_value->duplicate = 0 ;

	int *temp = (int *)malloc(100000000*sizeof(int)) ;
	int i , j ;
	for(i = 0 , j = 0 ; i < first_size || j < second_size ;){
		if(i >= first_size && j < second_size){
			for( ; j < second_size ; j++)
				temp[i + j] = data[second_head + j] ;
			break ;
		}else if(i < first_size && j >= second_size){
			for( ; i < first_size ; i++)
				temp[i + j] = data[first_head + i] ;
			break ;
		}
		if(data[first_head + i] < data[second_head + j]){
			temp[i + j] = data[first_head + i] ;
			i++ ;
		}else if(data[first_head + i] == data[second_head + j]){
			int temp_i = data[first_head + i] ;
			int temp_j = data[second_head + j] ;

			temp[i + j] = data[first_head + i] ;
			i++ ;
			temp[i + j] = data[second_head + j] ;
			j++ ;
			return_value->duplicate++ ;
			while(temp_i == data[first_head + i]){
				temp[i + j] = data[first_head + i] ;
				i++ ;
			}
			while(temp_j == data[second_head + j]){
				temp[i + j] = data[second_head + j] ;
				j++ ;
			}
		}else{
			temp[i + j] = data[second_head + j] ;
			j++ ;
		}
	}
	for(i = 0 ; i < second_tail - first_head + 1 ; i++){
		data[first_head + i] = temp[i] ;
	}
	free(temp) ;
	pthread_exit(return_value) ;

}

int compare(const void *a , const void *b){
	return ( *(int*)a - *(int*)b ) ; 
}

int main (int argc , char *argv[]){
	int segment_size = atoi(argv[1]) ;
	// pthread_attr_t attr;
	//pthread_attr_init(&attr);

	scanf("%d" , &total_data_size) ;
	for (int i = 0 ; i < total_data_size ; i++){
		scanf("%d" , &data[i]) ;
		//printf("data : %d\n" , data[i]) ;
	}

	for (int i = 0 ; i < total_data_size/segment_size ; i++){
		qsort(&data[segment_size * i] , segment_size , sizeof(int) , compare) ;
		 printf("Sorted %d elements.\n" , segment_size) ;
	}
	if(total_data_size % segment_size != 0){
		qsort(&data[segment_size * (total_data_size/segment_size)] , total_data_size % segment_size , sizeof(int) , compare) ;
		printf("Sorted %d elements.\n" , total_data_size % segment_size) ;
	}

	while(segment_size < total_data_size){
		int thread_num = 0 ;
		int i ;
		for (i = 2 ; i <= total_data_size/segment_size ; i += 2){

			Argument *temp = malloc(sizeof(Argument));
			temp->first_head = (i - 2)* segment_size ;
			temp->first_tail = (i - 2)* segment_size + segment_size - 1 ;
			temp->second_head = (i - 1)* segment_size ;
			temp->second_tail = (i - 1)* segment_size + segment_size - 1 ;
			if(pthread_create( &tid[thread_num] , NULL , &merge , (void *)temp) != 0){
				printf("thread create is broken!\n") ;
				exit(0) ;
			}
			thread_num++ ;
		}
		if( (total_data_size/segment_size) % 2 == 1 && total_data_size % segment_size != 0 ){
			Argument *temp = malloc(sizeof(Argument));
			temp->first_head = (i - 2)* segment_size ;
			temp->first_tail = (i - 2)* segment_size + segment_size - 1 ;
			temp->second_head = (i - 1)* segment_size ;
			temp->second_tail = (i - 1)* segment_size + total_data_size % segment_size - 1;
			if (pthread_create( &tid[thread_num] , NULL , merge , (void *)temp) != 0){
				printf("thread create is broken!\n") ;
				exit(0) ;
			}
			thread_num++ ;
		}

		for(i = 0 ; i < thread_num ; i++){
			void *return_value ;
			if( pthread_join(tid[i] , &return_value) != 0){
				printf("thread join is broken!\n") ;
				exit(0) ;
			}
			printf("Merged %d and %d elements with %d duplicates.\n" , ((Return *)return_value)->first_size , ((Return *)return_value)->second_size , 
																	   ((Return *)return_value)->duplicate) ;
		}

		segment_size *= 2 ;
	}
	int j ;
	for(j = 0 ; j < total_data_size - 1 ; j++)
		printf("%d " , data[j]) ;
	printf("%d\n" , data[j]) ;


}