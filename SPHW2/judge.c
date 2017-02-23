#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

void do_shuffle(int *card){
	int index1 , index2 ;
	for (int i = 0 ; i < 500 ; i++){
		srand(time(NULL) * (i + 1) ) ;
		index1 = rand() % 53 ;
		index2 = rand() % 53 ;

		int temp = card[index1] ;
		card[index1] = card[index2] ;
		card[index2] = temp ;
	}
}

typedef struct Player_info{
	int player_id ;
	int random_key ;
	int card_num ;
	int next_index ;
}Player_info ;

int find_player_info_index(char temp_player_index , char player_index[5][2] ){
	int k ;
	for (int k = 0 ; k < 4 ; k++){
		if(temp_player_index == player_index[k][0] )
			return k ;
	}

	// fprintf(stderr , "wrong player_index : %c\n" , temp_player_index ) ;
	// fflush(stderr) ;
	return -1 ;
}

int main (int argc , char *argv[]){
	srand(time(NULL));
	char judge_id[10] = {} ;
	strcpy(judge_id , argv[1]) ;
	char path[5][30] = {} ;
	char player_index[5][2] = {"A" , "B" , "C" , "D"} ;

	//fprintf(stderr , "in judge%s\n" , judge_id) ;

	while (1){
		char temp_char[30] = {} ;
		Player_info player[4] ;
		fscanf(stdin , "%d%d%d%d" , &player[0].player_id , &player[1].player_id , &player[2].player_id , &player[3].player_id) ; 

		// fprintf(stderr , "%d %d %d %d\n" , player[0].player_id , player[1].player_id , player[2].player_id , player[3].player_id) ;
		// fflush(stderr) ;
		if (player[0].player_id == 0 && player[1].player_id == 0 && player[2].player_id == 0 && player[3].player_id == 0 ){
			//fprintf(stderr , "end of judge%s!\n" , judge_id) ;
			exit(0) ;
		}
		
		//make FIFO
		sprintf(path[0] , "./judge%s.FIFO" , judge_id) ;
		mkfifo(path[0] , 0755) ;

		for (int i = 0 ; i < 4 ; i++){
			sprintf(path[i + 1] , "./judge%s_%s.FIFO" , judge_id , player_index[i]) ;
			mkfifo(path[i + 1] , 0755) ;
		}	

		//ready shuffled card
		int card[53] = {} ;
		card[52] = 0 ;
		for (int i = 0 ; i < 52 ; i++){
			card[i] = i % 13 + 1 ; 
		}

		do_shuffle(card) ;

		//check 順序
		for (int i = 0 ; i < 3 ; i++){
			player[i].next_index = i + 1 ;
		}
		player[3].next_index = 0 ;

		int random_index[65536] = {0} ;
		char char_random_key[10] = {} ;
		pid_t pid[4] ;

		for (int i = 0 ; i < 4 ; i++){
			srand(time(NULL)  * (i + 1)) ;
			int random_temp = rand() % 65536 ;
			while(random_index[random_temp] == 1){
				random_temp = rand() % 65536 ;
			}
			random_index[random_temp] = 1 ;
			player[i].random_key = random_temp ;

			pid[i] = fork() ;
			if(pid[i] < 0){
				fprintf(stderr , "judge_fork is broken!\n") ;
				fflush(stderr) ;
				exit(0) ;
			}else if (pid[i] == 0){ //child fork

				memset(char_random_key , '\0' , sizeof(char_random_key) ) ;
				sprintf(char_random_key , "%d\n" , player[i].random_key ) ;

				if ( execl("./player" , "./player" , judge_id , player_index[i] , char_random_key , (char *)0 ) < 0 ){
					fprintf(stderr , "execl player broken\n") ;
					exit(0) ;
				}
			}
		} 

		//open fp
		FILE *fp[5] ;

		//judge.FIFO
		fp[0] = fopen ( path[0] , "r+" ) ;
		if (fp[0] == NULL){
			fprintf(stderr , "fopen is broken\n") ;
			exit(0) ;
		}

		//judge_A.FIFO
		for (int i = 1 ; i < 5 ; i++){
			fp[i] = fopen( path[i] , "w" ) ; 
			if (fp[i] == NULL){
				fprintf(stderr , "fopen is broken\n") ;
				exit(0) ;
			}
		}

		// send shuffled card
		fprintf(fp[1] , "%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n" , 
				card[0] , card[1] , card[2] , card[3] ,
				card[4] , card[5] , card[6] , card[7] ,
				card[8] , card[9] , card[10] , card[11] ,
				card[12] , card[13]) ;
		fflush(fp[1]) ;

		for (int i = 0 ; i < 3 ; i++){
			int k ;
			if (i == 0)
				k = 14 ;
			else if (i == 1)
				k = 27 ;
			else if (i == 2)
				k = 40 ;
			fprintf(fp[2 + i] , "%d %d %d %d %d %d %d %d %d %d %d %d %d\n" , 
				card[0 + k] , card[1 + k] , card[2 + k] , card[3 + k] ,
				card[4 + k] , card[5 + k] , card[6 + k] , card[7 + k] ,
				card[8 + k] , card[9 + k] , card[10 + k] , card[11 + k] ,
				card[12 + k] ) ;
			fflush(fp[2 + i]) ;
		}

		// first check each player`s card_num
		for (int i = 0 ; i < 4 ; i++){
			char temp_sit[40] , temp_player_index[10] ;
			int temp_random_key , temp_player_info_index , card_num ;

			fscanf(fp[0] , "%s %d %d" , temp_player_index , &temp_random_key , &card_num ) ;

			temp_player_info_index = find_player_info_index(temp_player_index[0] , player_index ) ;

			if (temp_player_info_index == -1 || player[temp_player_info_index].random_key != temp_random_key){
				i-- ;
				continue ;
			}
			player[temp_player_info_index].card_num = card_num ;
		}
		
		int draw_index = 0 ;
		int next_index ;
		int lose_id = -1 ;

		while(1){
			int draw_card_number ;
			int eliminate_sit , random_key , draw_card_id , real_player_id;
			char temp_player_index[3] , temp_type[3];

			//check draw card player index
			while (player[draw_index].card_num == 0)
				draw_index = player[draw_index].next_index ;

			//check be drawed player index
			next_index = player[draw_index].next_index ;
			while( player[next_index].card_num == 0 )
				next_index = player[next_index].next_index ;

			//check winner
			if (draw_index == next_index){
				lose_id = player[draw_index].player_id ;
				break ;
			}

			//tell the draw card player the total card_num of the next one
			memset(temp_type , '\0' , sizeof(temp_type) ) ;
			sprintf(temp_type , "<") ;
			fprintf(fp[draw_index + 1] , "%s %d\n" , temp_type , player[next_index].card_num) ;
			fflush(fp[draw_index + 1]) ;

			//read the card_id that draw card player want
			while(1){
				fscanf(fp[0] , "%s %d %d" , temp_player_index , &random_key , &draw_card_id ) ;
				fflush(fp[0]) ;

				if(temp_player_index[0] != player_index[draw_index][0]){
					// fprintf(stderr , "is not %c turn!\n" , temp_player_index[0]) ;
					// fflush(stderr) ;
					continue ;
				}
				if(draw_card_id > player[next_index].card_num){
					// fprintf(stderr , "there doesn`t exsit this card!\n") ;
					// fflush(stderr) ;
					continue ;
				}

				real_player_id = find_player_info_index(temp_player_index[0] , player_index) ;

				if(real_player_id == -1)
					continue ;
				
				if (player[real_player_id].random_key == random_key){
					break ;
				}
				// fprintf(stderr , "random_key is wrong\n") ;
				// fflush(stderr) ;
			}

			// tell the be drawed player which card_id be drawed
			memset(temp_type , '\0' , sizeof(temp_type) ) ;
			sprintf(temp_type , ">") ; 
			fprintf(fp[next_index + 1] , "%s %d\n" , temp_type , draw_card_id ) ;
			fflush(fp[next_index + 1]) ;

			//read be drawed card number
			while(1){
				fscanf(fp[0] , "%s %d %d" , temp_player_index , &random_key , &draw_card_number ) ;
				fflush(fp[0]) ;

				if(temp_player_index[0] != player_index[next_index][0]){
					// fprintf(stderr , "is not %c turn!\n" , temp_player_index[0]) ;
					// fflush(stderr) ;
					continue ;
				}

				real_player_id = find_player_info_index(temp_player_index[0] , player_index) ;

				if(real_player_id == -1)
					continue ;
				
				if (player[real_player_id].random_key == random_key){
					break ;
				}
				// fprintf(stderr , "random_key is wrong\n") ;
				// fflush(stderr) ;
				
			}
			player[next_index].card_num-- ;

			//tell draw card player the number of the be drawed card
			fprintf(fp[draw_index + 1] , "%d\n" , draw_card_number ) ;
			fflush(fp[draw_index + 1]) ;

			//read the eliminate_sit from the draw card player
			while(1){
				fscanf(fp[0] , "%s %d %d" , temp_player_index , &random_key , &eliminate_sit) ;
				fflush(fp[0]) ;

				if(temp_player_index[0] != player_index[draw_index][0]){
					// fprintf(stderr , "is not %c turn!\n" , temp_player_index[0]) ;
					// fflush(stderr) ;
					continue ;
				}

				real_player_id = find_player_info_index(temp_player_index[0] , player_index) ;

				if(real_player_id == -1)
					continue ;
				
				if (player[real_player_id].random_key == random_key){
					break ;
				}
				// fprintf(stderr , "random_key is wrong\n") ;
				// fflush(stderr) ;
				
			}

			if(eliminate_sit == 1){
				player[draw_index].card_num-- ;
			}else if(eliminate_sit == 0){
				player[draw_index].card_num++ ;
			}

			draw_index = next_index ;
			
		}

		for (int i = 0 ; i < 4 ; i++){
			kill(pid[i] , SIGKILL) ;
			wait(NULL) ;
		}
		for (int i = 0 ; i < 5 ; i++){
			fclose(fp[i]) ;
			unlink(path[i]) ;
		}
		
		fprintf(stdout , "%d\n" , lose_id) ;
		fflush(stdout) ;
	}

	exit(0) ;
}