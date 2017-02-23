#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

FILE *log_fp , *data_fp ;
int count[3];
int check[3];
int condiction[3] ;

struct sigaction int_act , old_int_act ;
struct sigaction usr1_act , old_usr1_act ;
struct sigaction usr2_act , old_usr2_act ;
struct sigaction alarm_act , old_alarm_act ;

void sig_int(){
	//fprintf(stderr , "in sig_int\n") ;
	count[0]++ ;
	condiction[0] = 0 ;
	fprintf(log_fp , "finish %d %d\n" , 0 , count[0]) ;
}

void sig_usr1(){
	//fprintf(stderr , "in sig_usr1\n") ;
	count[1]++ ;
	condiction[1] = 0 ;
	fprintf(log_fp , "finish %d %d\n" , 1 , count[1]) ;
}

void sig_usr2(){
	//fprintf(stderr , "in sig_usr2\n") ;
	count[2]++ ;
	condiction[2] = 0 ;
	fprintf(log_fp , "finish %d %d\n" , 2 , count[2]) ;
}

void set_sigaction(struct sigaction *act , int sig_no , void *function){
	act->sa_handler = function ;
	act->sa_flags = SA_RESTART ;

	sigemptyset( &(act->sa_mask) ) ;

	//check what signal should be block

}

int main (int argc , char *argv[]){
	log_fp = fopen("sender_log" , "w+") ;
	if( (data_fp = fopen(argv[1] , "r+") ) == NULL ){
		//printf("open test_data is broken~\n") ;
	}
	int i ;

	for(i = 0 ; i < 3 ; i++){
		count[i] = 0 ;
		check[i] = 0 ;
		condiction[i] = 0 ;
	}

	set_sigaction(&int_act , SIGINT , sig_int) ;
	set_sigaction(&usr1_act , SIGUSR1 , sig_usr1) ;
	set_sigaction(&usr2_act , SIGUSR2 , sig_usr2) ;

	sigaction(SIGINT , &int_act , &old_int_act) ;
	sigaction(SIGUSR1 , &usr1_act , &old_usr1_act) ;
	sigaction(SIGUSR2 , &usr2_act , &old_usr2_act) ;




	int priority , start_time ;
	int to_do_num = 0 ;

	int run_time = 0 ;
	int dead_line[3] = {} ;

	while (fscanf(data_fp , "%d%d" , &priority , &start_time ) != EOF){
		
		while(run_time < start_time){
			struct timespec set_time , release_time ;
			set_time.tv_nsec = 100000000;
			set_time.tv_sec = 0;

			while(nanosleep(&set_time , &release_time) != 0)
				set_time = release_time ;

			run_time++ ;
			for(i = 1 ; i < 3 ; i++){
				if(condiction[i] == 1 && dead_line[i] < run_time){
					//fprintf(stderr , "dead_line : %d , run_time : %d\n" , dead_line[i] , run_time) ;
					fprintf(log_fp , "timeout %d %d\n" , i , check[i]) ;
					exit(0) ;
				}
			}
		}

		//fprintf(stderr , "priority : %d start_time : %d\n"  , priority , start_time) ;
		if(priority == 0){
			check[0]++ ;
			condiction[0] = 1 ;
			fprintf(log_fp , "send %d %d\n" , 0 , check[0]) ;
			printf("ordinary\n") ;
			fflush(stdout) ;
		}else if (priority == 1){
			check[1]++ ;
			condiction[1] = 1 ;
			dead_line[1] = start_time + 10 ;
			fprintf(log_fp , "send %d %d\n" , 1 , check[1]) ;
			kill(getppid() , SIGUSR1) ;
		}else if(priority == 2){
			check[2]++ ;
			condiction[2] = 1 ;
			dead_line[2] = start_time + 3 ;
			fprintf(log_fp , "send %d %d\n" , 2 , check[2]) ;
			kill(getppid() , SIGUSR2) ;
		}
	}
	while(1){
		int end_flag = 0 ;
		for(i = 0 ; i < 3 ; i++){
			if(condiction[i] == 1)
				end_flag = 1 ;
		}
		if(end_flag == 1){
			struct timespec set_time , release_time ;
			set_time.tv_nsec = 100000000;
			set_time.tv_sec = 0;

			while(nanosleep(&set_time , &release_time) != 0)
				set_time = release_time ;

			run_time++ ;
			for(i = 1 ; i < 3 ; i++){
				if(condiction[i] == 1 && dead_line[i] < run_time){
					//fprintf(stderr , "dead_line : %d , run_time : %d\n" , dead_line[i] , run_time) ;
					fprintf(log_fp , "timeout %d %d\n" , i , check[i]) ;
					exit(0) ;
				}
			}
		}else{
			break ;
		}
	}
	exit(0) ;
}