#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

FILE * log_fp ;

int ordinary_count ;
int urgent_count ;
int v_urgent_count ;
pid_t pid ;

struct sigaction terminate_act , old_terminate_act ;
struct sigaction urgent_act , old_urgent_act ;
struct sigaction v_urgent_act , old_v_urgent_act ;

void set_sigaction(struct sigaction *act , int sig_no , void *function){
	
	sigemptyset( &(act->sa_mask) ) ;
	if(sig_no == SIGUSR2){
		sigaddset(&(act->sa_mask) , SIGUSR1) ;
	}

	act->sa_handler = function ;
	//sigprocmask(SIG_BLOCK , &(act->sa_mask) , NULL) ;
	act->sa_flags = SA_RESTART ;
}

void sig_int(){
	//printf("in sig_int\n");
	fprintf(log_fp , "terminate\n") ;
	kill(pid , SIGKILL) ;
	wait(NULL) ;

	sigaction(SIGINT , &old_terminate_act , NULL) ;
	sigaction(SIGUSR1 , &old_urgent_act , NULL) ;
	sigaction(SIGUSR2 , &old_v_urgent_act , NULL) ;
	exit(0) ;

}

void sig_usr1(){
	//printf("in sig_usr1\n");
	urgent_count++ ;
	fprintf(log_fp , "receive %d %d\n" , 1 , urgent_count) ;
	
	struct timespec set_time , release_time ;
	set_time.tv_nsec = 500000000;
	set_time.tv_sec = 0 ;
	
	while(nanosleep(&set_time , &release_time) != 0)
		set_time = release_time ;

	kill(pid , SIGUSR1) ;
	fprintf(log_fp , "finish %d %d\n" , 1 , urgent_count) ;
}

void sig_usr2(){
	//printf("in sig_usr2\n") ;

	v_urgent_count++ ;
	fprintf(log_fp , "receive %d %d\n" , 2 , v_urgent_count) ;

	struct timespec set_time , release_time ;
	set_time.tv_nsec = 200000000;
	set_time.tv_sec = 0 ;
	
	while(nanosleep(&set_time , &release_time) != 0)
		set_time = release_time ;

	kill(pid , SIGUSR2) ;
	fprintf(log_fp , "finish %d %d\n" , 2 , v_urgent_count) ;
}


int main (int argc , char *argv[] ){
	int receiver_to_sender[2] ;
	int sender_to_receiver[2] ;

	log_fp = fopen("receiver_log" , "w+") ;
	ordinary_count = 0 ;
	urgent_count = 0 ;
	v_urgent_count = 0 ;

	if(pipe(receiver_to_sender) < 0){
		printf("open receiver_to_sender pipe is broken\n") ;
		exit(0) ;
	}
	if(pipe(sender_to_receiver) < 0){
		printf("open sender_to_receiver pipe is broken\n") ;
		exit(0) ;
	}

	set_sigaction(&terminate_act , SIGINT , sig_int) ;
	set_sigaction(&urgent_act , SIGUSR1 , sig_usr1) ;
	set_sigaction(&v_urgent_act , SIGUSR2 , sig_usr2) ;

	if(sigaction(SIGINT , &terminate_act , &old_terminate_act) < 0){
		printf("SIGINT sigaction is broken!\n");
		exit(0) ;
	}

	if(sigaction(SIGUSR1 , &urgent_act , &old_urgent_act) < 0){
		printf("SIGUSR1 sigaction is broken!\n") ;
		exit(0) ;
	}

	if(sigaction(SIGUSR2 , &v_urgent_act , &old_v_urgent_act) < 0){
		printf("SIGUSR2 is broken!\n") ;
		exit(0) ;
	}


	if(( pid = vfork() ) < 0){
		printf("fork is broken!\n") ;
		exit(0) ;
	}else if(pid == 0){ //child
		//dup2(receiver_to_sender[0] , STDIN_FILENO) ;
		dup2(sender_to_receiver[1] , STDOUT_FILENO) ;
		close(receiver_to_sender[1]) ;
		close(receiver_to_sender[0]) ;
		close(sender_to_receiver[1]) ;
		close(sender_to_receiver[0]) ;

		if(execl("./sender" , "./sender" , argv[1] , (char *)0 ) < 0){
			fprintf(stderr , "exec is broken !\n") ;
			exit(0) ;
		}
	}else{ //parent

		//kill(getpid() , SIGUSR1) ;
		//kill(getpid() , SIGUSR2) ;

		close(receiver_to_sender[1]) ;
		close(receiver_to_sender[0]) ;
		close(sender_to_receiver[1]) ;

		while(1){
			char temp_char[20] = {} ;

			if(read(sender_to_receiver[0] , temp_char , 10) == 0){
				fprintf(log_fp , "terminate\n") ;
				//printf("EOF\n");
				exit(0) ;
			}else{
				ordinary_count++ ;
				fprintf(log_fp , "receive %d %d\n" , 0 , ordinary_count) ;
				//printf("temp_char : %s" , temp_char) ;

				if(strcmp(temp_char , "ordinary\n") == 0){

					struct timespec set_time , release_time ;
					set_time.tv_nsec = 0;
					set_time.tv_sec = 1 ;
	
					while(nanosleep(&set_time , &release_time) > 0)
						set_time = release_time ;

					//printf("send SIGINT\n") ;
					kill(pid , SIGINT) ;
					fprintf(log_fp, "finish %d %d\n", 0 , ordinary_count);

				}else{
					printf("temp_char is broken!\n") ;
					exit(0) ;
				}
			}
		}

	}
}