#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
    int id ;
    int money ;
} Account_info ;

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
	int account;
    int write_account ;
    int wait_for_write;
    int write_situation ;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* reject_header = "REJECT\n";

// Forwards

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
int lock_reg(int fd , int cmd , int type , off_t offset , int whence , off_t len){
    struct flock lock ;
    lock.l_type = type ;
    lock.l_start = offset ; 
    lock.l_len = len ;
    lock.l_whence = whence ;

    return (fcntl(fd , cmd , &lock)) ;
}

#define read_lock(fd , offset , whence , len) \
    lock_reg ((fd) , F_SETLK , F_RDLCK , (offset) , (whence) , (len) )
#define write_lock(fd , offset , whence , len) \
    lock_reg ((fd) , F_SETLK , F_WRLCK , (offset) , (whence) , (len) )
#define unlock(fd , offset , whence , len) \
    lock_reg ((fd) , F_SETLK , F_UNLCK , (offset) , (whence) , (len) )

int main(int argc, char** argv) {
    int i, ret , max_conn_fd = -1 ;

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    fd_set read_set , write_set , temp_set ;

    FD_ZERO(&read_set) ;
    FD_ZERO(&write_set) ;
    FD_ZERO(&temp_set) ;

    int local_write_lock[21] = {} ;

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    if (requestP[svr.listen_fd].conn_fd > max_conn_fd)
        max_conn_fd = requestP[svr.listen_fd].conn_fd ;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    FD_SET(svr.listen_fd , &read_set) ;

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    while (1) {
        // TODO: Add IO multiplexing
        // Check new connection
        temp_set = read_set ;
        int read_ready_num = select(max_conn_fd + 1 , &temp_set , NULL , NULL , NULL);

        for (int i = svr.listen_fd ; i <= max_conn_fd ; i++){
            //printf ("i : %d\n" , i) ;
            if (i == svr.listen_fd && FD_ISSET(i , &temp_set) ){
                clilen = sizeof(cliaddr);
                conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                if (conn_fd < 0) {
                  if (errno == EINTR || errno == EAGAIN) continue;  // try again
                    if (errno == ENFILE) {
                        (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                        continue;
                    }
                    ERR_EXIT("accept")
                }
                requestP[conn_fd].conn_fd = conn_fd;

                FD_SET(conn_fd , &read_set) ;
                //FD_SET(conn_fd , &write_set) ;
                if (conn_fd > max_conn_fd)
                    max_conn_fd = conn_fd ;
                strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
                continue;

            //do_requst
            }else if (FD_ISSET(i , &temp_set)){

        		ret = handle_read(&requestP[i]); // parse data from client to requestP[conn_fd].buf
        		if (ret < 0) {
        			fprintf(stderr, "bad request from %s\n", requestP[i].host);
        			continue;
        		}

                int fp ;
                fp = open("account_info" , O_RDWR) ;
                if (fp == -1){
                    fprintf (stderr , "open file is broken!\n") ;
                    exit(1) ;
                }


    #ifdef READ_SERVER 
            if (read_lock(fp , 8 * (atoi(requestP[i].buf) - 1) , SEEK_SET , sizeof(Account_info)) == 0 ){       
                Account_info *temp_account = malloc(sizeof(Account_info)) ;
                lseek (fp , 8 * (atoi(requestP[i].buf) - 1) , SEEK_SET ) ;
                if (read(fp , temp_account , sizeof(Account_info) ) == -1 ){
                    fprintf (stderr , "read is wrong\n") ;
                }
                sprintf(buf,"Balance : %d\n", temp_account->money );
                write(requestP[i].conn_fd, buf, strlen(buf));
                unlock(fp , 8 * (atoi(requestP[i].buf) - 1) , SEEK_SET , sizeof(Account_info)) ;
            }else{
                strcpy(buf , "This account is occupied.\n") ;
                write(requestP[i].conn_fd , buf , strlen(buf) ) ;
            }
    #else   

                if (requestP[i].write_situation == 0){
                    //printf ("191\n") ;
                    if (write_lock(fp , 8 * (atoi(requestP[i].buf) - 1) , SEEK_SET, sizeof(Account_info)) == 0){
                        if (local_write_lock[atoi(requestP[i].buf)] == 1){
                            strcpy(buf , "This account is occupied.\n") ;
                            write(requestP[i].conn_fd , buf , strlen(buf) ) ;

                        }else if (local_write_lock[atoi(requestP[i].buf)] == 0){
                            local_write_lock[atoi(requestP[i].buf)] = 1 ;
                            requestP[i].write_account = atoi(requestP[i].buf) ;
                            char buffer[50] ;
                            strcpy(buffer , "This account is available.\n") ;
                            write(requestP[i].conn_fd , buffer , strlen(buffer)) ;
                            requestP[i].write_situation = requestP[i].conn_fd ;
                            break ;
                        }else{
                            fprintf(stderr, "local_read_lock is broken\n" );
                        }
                    }else{
                        strcpy(buf , "This account is occupied.\n") ;
                        write(requestP[i].conn_fd , buf , strlen(buf) ) ;
                    }

                }else if(requestP[i].write_situation == requestP[i].conn_fd){
                    if (write_lock( fp , 8 * (requestP[i].write_account - 1) , SEEK_SET, sizeof(Account_info) ) == 0 ){
                        Account_info *temp_account = malloc(sizeof(Account_info));

                        lseek (fp , 8 * ( requestP[i].write_account - 1) , SEEK_SET) ;
                        read(fp , temp_account , sizeof(Account_info) ) ;
                        if (temp_account->money + atoi(requestP[i].buf) >= 0 ){

                            temp_account->money += atoi(requestP[i].buf) ;
                            lseek(fp , 8 * (temp_account->id - 1) + 4 , SEEK_SET) ;
                            write(fp , &temp_account->money , sizeof(int) ) ;
                            local_write_lock[requestP[i].write_account] = 0 ;
                            unlock(fp , 8 * ( requestP[i].write_account - 1 ) , SEEK_SET, sizeof(Account_info)) ;
                        }else{
                            strcpy(buf , "Operation fail.\n") ;
                            write(requestP[i].conn_fd , buf , strlen(buf) ) ;
                            unlock(fp , 8 * ( requestP[i].write_account - 1 ) , SEEK_SET, sizeof(Account_info)) ;
                        }
                    }else{
                        strcpy(buf , "This account is occupied.\n") ;
                        write(requestP[i].conn_fd , buf , strlen(buf) ) ;
                    }
                }else{
                    fprintf (stderr , "write_situation is broken\n") ;
                    exit(1) ;
                }
            
            
    		/*sprintf(buf,"%s : %s\n",accept_write_header,requestP[conn_fd].buf);
    		write(requestP[conn_fd].conn_fd, buf, strlen(buf));*/
    #endif
            FD_CLR(i , &read_set) ;
    		close(requestP[i].conn_fd);
    		free_request(&requestP[i]);
            }
        }
    }
    free(requestP);
    return 0;
}


// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void* e_malloc(size_t size);


static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->account = 0;
    reqP->wait_for_write = 0;
    reqP->write_situation = 0 ;
    reqP->write_account = -1 ;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
	char* p1 = strstr(buf, "\015\012");
	int newline_len = 2;
	// be careful that in Windows, line ends with \015\012
	if (p1 == NULL) {
		p1 = strstr(buf, "\012");
		newline_len = 1;
		if (p1 == NULL) {
			ERR_EXIT("this really should not happen...");
		}
	}
	size_t len = p1 - buf + 1;
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
    return 1;
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
}

static void* e_malloc(size_t size) {
    void* ptr;

    ptr = malloc(size);
    if (ptr == NULL) ERR_EXIT("out of memory");
    return ptr;
}

