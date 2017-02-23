#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main (int argc , char *argv[]){
	srand(time(NULL)) ;
	int test_num = atoi(argv[1]) ;

	printf("%d\n" , test_num) ;
	for (int i = 0 ; i < test_num ; i++){
		printf("%d " , rand()%1000000000 ) ;
		if(i % 15 == 0){
			srand(time(NULL)*i) ;
			printf("\n") ;
		}
	}
}