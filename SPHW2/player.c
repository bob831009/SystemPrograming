#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int compare(const void *a, const void *b){
    int c = *(int *)a;
    int d = *(int *)b;
    if(c < d) 
    	return -1;              
    else if (c == d)
    	return 0;      
    else
   		return 1;                     
}

int eliminate(int card[14] , int handle_card[14] , int max_card_num){
	int handle_card_num = 0 ;
	int same_condiction ;
	for (int i = 0 ; i < max_card_num ; i++){

		same_condiction = 0 ;
		if(card[i] == -1)
			continue ;
		for (int j = i + 1 ; j < max_card_num && card[i] != -1 ; j++){
			if (card[i] == card[j]){
				card[i] = -1 ;
				card[j] = -1 ;
				same_condiction = 1 ;
				break ;
			}
		}
		if(same_condiction == 0){
			handle_card[ handle_card_num ] = card[i] ;
			handle_card_num++ ;
		}
	}
	// fprintf(stderr , "handle_card_num : %d\n" , *handle_card_num) ;
	// fflush(stderr) ;
	return handle_card_num ;
}

int main (int argc , char *argv[]){
	char judge_id[10] = {} ;
	strcpy(judge_id , argv[1]) ;
	char player_index = argv[2][0] ;
	int random_key = atoi(argv[3]) ;

	// fprintf (stderr , "judge_id : %s , player_index : %c , random_key : %d\n" , judge_id , player_index , random_key) ;
	// fflush(stderr) ;

	FILE *fp_read , *fp_write ;
	srand(time(NULL)) ;

	char path[30] = {} ;
	memset(path , '\0' , sizeof(path) ) ;
	sprintf(path , "./judge%s_%c.FIFO" , judge_id , player_index) ;
	fp_read = fopen(path , "r") ;
	if (fp_read == NULL){
		fprintf(stderr , "fopen is broken\n") ;
		exit(0) ;
	}

	memset(path , '\0' , sizeof(path) ) ;
	sprintf(path , "./judge%s.FIFO" , judge_id ) ;
	fp_write = fopen(path , "a") ;
	if (fp_write == NULL){
		fprintf(stderr , "fopen is broken\n") ;
		exit(0) ;
	}

	int card[14] = {-1} ;
	int handle_card[14] = {-1} ;
	int handle_card_num = 0 ;
	int draw_card_num , draw_card_id , draw_card_number , temp_num , be_draw_card;
	char type[3] ;

	int max_card_num ;
	if (player_index == 'A'){
		max_card_num = 14 ;
		fscanf(fp_read , "%d %d %d %d %d %d %d %d %d %d %d %d %d %d" ,
			&card[0] , &card[1] , &card[2] , &card[3] ,
			&card[4] , &card[5] , &card[6] , &card[7] ,
			&card[8] , &card[9] , &card[10] , &card[11] ,
			&card[12] , &card[13]) ;
	}else{
		max_card_num = 13 ;
		fscanf(fp_read , "%d %d %d %d %d %d %d %d %d %d %d %d %d" ,
			&card[0] , &card[1] , &card[2] , &card[3] ,
			&card[4] , &card[5] , &card[6] , &card[7] ,
			&card[8] , &card[9] , &card[10] , &card[11] ,
			&card[12]) ;
	}
	
	handle_card_num = eliminate(card , handle_card , max_card_num) ;

	char temp_player_index[3] = {} ;
	sprintf(temp_player_index , "%c" , player_index) ;
	fprintf(fp_write , "%s %d %d\n" , temp_player_index , random_key , handle_card_num) ;
	fflush(fp_write) ;

	while(1){
		fscanf(fp_read , "%s %d" , type , &temp_num ) ;

		if(type[0] == '<'){
			draw_card_num = temp_num ;
			draw_card_id = rand() % draw_card_num + 1 ;
	 	
	 		memset(temp_player_index , '\0' , sizeof(temp_player_index) ) ;
			sprintf(temp_player_index , "%c" , player_index) ;
			fprintf(fp_write , "%s %d %d\n" , temp_player_index , random_key , draw_card_id) ;
			fflush(fp_write) ;

			int eliminate_sit = 0 ;
			fscanf(fp_read , "%d" , &draw_card_number) ;
	
			for (int i = 0 ; i < handle_card_num ; i++){
				if(draw_card_number == handle_card[i]){
					for (int j = i ; j < handle_card_num - 1 ; j++)
						handle_card[j] = handle_card[j + 1] ;
					eliminate_sit = 1 ;
					handle_card_num-- ;
					break ;
				}
			}
			if(eliminate_sit == 0){
				handle_card[handle_card_num] = draw_card_number ;
				handle_card_num++ ;
			}

			memset(temp_player_index , '\0' , sizeof(temp_player_index) ) ;
			sprintf(temp_player_index , "%c" , player_index) ;
			fprintf(fp_write , "%s %d %d\n" , temp_player_index , random_key , eliminate_sit) ;
			fflush(fp_write) ;

		}else if (type[0] == '>'){
			be_draw_card = temp_num ;
			int temp_be_draw_card_number = handle_card[be_draw_card - 1] ;
			for (int i = temp_num - 1 ; i < handle_card_num - 1 ; i++){
				handle_card[i] = handle_card[i + 1] ;
			}
			handle_card_num-- ;

			memset(temp_player_index , '\0' , sizeof(temp_player_index) ) ;
			sprintf(temp_player_index , "%c" , player_index) ;
			fprintf(fp_write , "%s %d %d\n" , temp_player_index , random_key , temp_be_draw_card_number ) ;
			fflush(fp_write) ;

		}else{
			fprintf(stderr , "type is wrong!\n") ;
			exit(0) ;
		}
	}

}