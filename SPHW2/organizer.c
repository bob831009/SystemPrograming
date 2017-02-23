#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void do_swap(int *a_score , int *b_score , int *a_id , int *b_id){
	int temp = *a_score ;
	*a_score = *b_score ;
	*b_score = temp ;

	temp = *a_id ;
	*a_id = *b_id ;
	*b_id = temp ;
}

int main (int argc , char *argv[] ){

	int player_num = atoi(argv[2]) ;
	int judge_num = atoi(argv[1]) ;

	int player_score[16] = {} ;
	int player_id[16] = {} ;
	char judge[12][10] ;
	char game[2000][20] ;


	memset(judge , '\0' , sizeof(judge) ) ;
	memset(game , '\0' , sizeof(game) ) ;
	int total_game_num = 0 ;
	for (int i = 0 ; i < player_num ; i++)
		player_id[i] = i + 1 ;

	for (int i = 1 ; i <= player_num - 3 ; i++)
		for (int j = i + 1 ; j <= player_num - 2 ; j++)
			for (int k = j + 1 ; k <= player_num - 1 ; k++)
				for (int s = k + 1 ; s <= player_num ; s++){
					sprintf (game[total_game_num] , "%d %d %d %d\n" , i , j , k , s) ;
					total_game_num++ ;
				}

	fd_set read_set ;
	FD_ZERO(&read_set) ;

	int fd_org_write[12][2] ;
	int fd_org_read[12][2] ;
	int have_assign = 0 ;
	int finish = 0 ;

	for (int i = 0 ; i < judge_num ; i++)
		sprintf(judge[i] , "%d" , i + 1) ;

	for (int i = 0 ; i < judge_num ; i++) {
		pid_t pid ;

		if (pipe(fd_org_read[i]) < 0 || pipe(fd_org_write[i]) < 0 ){
			fprintf(stderr , "pipe is broken\n") ;
			exit(0) ;
		}

		pid = fork() ;
		if (pid < 0){
			fprintf(stderr , "fork is broken\n") ;
			exit(0) ;
		}else if (pid == 0){
			dup2(fd_org_write[i][0] , STDIN_FILENO) ;
			dup2(fd_org_read[i][1] , STDOUT_FILENO) ;
			close(fd_org_read[i][0]) ;
			close(fd_org_read[i][1]) ;
			close(fd_org_write[i][0]) ;
			close(fd_org_write[i][1]) ;


			if (execl("./judge" , "./judge" , judge[i] , (char *)0 ) < 0){
				fprintf(stderr , "execl is \n") ;
				exit(0) ;
			}
		}else{
			//sleep(2) ;
			close(fd_org_write[i][0]) ;
			close(fd_org_read[i][1]) ;

			write(fd_org_write[i][1] , game[have_assign] , strlen(game[have_assign]) ) ;
			FD_SET(fd_org_read[i][0] , &read_set) ;
			have_assign++ ;
		}
	}


	int select_sit ;
	char char_lose_id[30] ={} ;
	int lose_id ; 

	//sleep(2) ;
	while(1){
		if (finish >= total_game_num && have_assign >= total_game_num )
			break ;
		fd_set temp_set = read_set ;


		select_sit = select(FD_SETSIZE , &temp_set , NULL , NULL , NULL ) ;
		if (select_sit < 0){
			fprintf(stderr, "select is broken!\n") ;
		}else if (select_sit == 0){
			fprintf(stderr , "no one ready\n") ;
		}else{
			for (int i = 0 ; i < judge_num ; i++){
				if(FD_ISSET(fd_org_read[i][0] , &temp_set)){
					
					memset(char_lose_id , '\0' , sizeof(char_lose_id) ) ;
					if (read(fd_org_read[i][0] , char_lose_id , 4 ) < 0 )
						fprintf(stderr , "read_err\n") ;
					FD_CLR(fd_org_read[i][0] , &read_set) ;
					lose_id = atoi(char_lose_id) ;
					//printf("loser : %d\n" , lose_id) ;

					player_score[lose_id - 1]-- ;
					finish++ ;

					if (have_assign < total_game_num){
						//printf("game : %s" , game[have_assign] ) ;
						if ( write(fd_org_write[i][1] , game[have_assign] , strlen(game[have_assign]) ) < 0 )
							fprintf(stderr , "write_err\n") ; 
						FD_SET(fd_org_read[i][0] , &read_set) ;
						have_assign++ ;
					}
				}
			}
		}
	}

	if(finish != total_game_num){
		printf("finish is broken\n") ;
		exit(0) ;
	}


	//printf("already finish here\n") ;
	for (int i = 0 ; i < judge_num ; i++){
		char terminate[20] = {} ; 
		sprintf(terminate , "0 0 0 0\n") ;
		write(fd_org_write[i][1] , terminate , strlen(terminate)) ;
		close(fd_org_write[i][1]) ;
		close(fd_org_read[i][0]) ;


		int status ;
		wait(NULL) ;
	}


	//bubble sort
	for (int i = player_num - 1 ; i >= 0; i--)
		for (int j = 1 ; j <= i ; j++){
			if (player_score[j - 1] > player_score[j] ){
				do_swap(&player_score[j - 1] , &player_score[j] , &player_id[j - 1] , &player_id[j] ) ;

			}else if (player_score[j - 1] == player_score[j]){
				if (player_id[j - 1] > player_id[j]){
					do_swap(&player_score[j - 1] , &player_score[j] , &player_id[j - 1] , &player_id[j] ) ;
				}
			}
		}

	// for (int i = 0 ; i < player_num ; i++)
	// 	printf("player_id%d : %d\n" , player_id[i] , player_score[i]) ;
	for (int i = 0 ; i < player_num - 1 ; i++)
			printf ("%d " , player_id[i]) ;
	printf("%d\n", player_id[player_num - 1] );

	return 0 ;
}