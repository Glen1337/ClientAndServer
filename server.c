/********************************************/
/* Glen Anderson 11/7/2015          
/* 91.413 Programming Assignment 1  
/* server.c
/*		
/********************************************/

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>


void handler(int);

typedef enum{
	true=1, false=0
} bool;


int main (int argc, char *argv []){

	/* Check argument count */
	if (argc !=2 ){
		printf("Please enter port number as argument");
		return 0;
	}

	bool getb = false;
	bool putb = false;

	char put[] = "put";
	char PUT[] = "PUT";
	char get[] = "get";
	char GET[] = "GET";

	unsigned long portl = 0;
	unsigned short port = 0;
	char **endptr = NULL;

	/* Convert argument to long */
	portl = strtol(argv[1], endptr, 10);

	/* Check port range */
	if ((portl > 65534 ) | (portl < 1024)){
		printf("\nPort number out of range\n\n");
		return 0;
	}
	else{
		port = (unsigned short) portl;
	}
	
	printf("\n-----[server] Port #------\n %hu\n", port );

	struct addrinfo hint;
	struct addrinfo *result;
	/* Set up the hints struct addrinfo for getaddrinfo() */
	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_flags = AI_PASSIVE|AI_ADDRCONFIG;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

	//use s and gai_strerror for error checking
	// result struct will contain addrinfo struct used to connect with local socket 
	int s = getaddrinfo( NULL , argv[1], &hint, &result);
	if (s < 0){
		fprintf (stdout,"couldnt getaddrinfo is server: %s\n", gai_strerror(s));
		exit(22);
	}
	const int reuse_addr_on =1;
	int socketfd =0;
	//loop to find a connection that can be binded.
	do{
		//use result struct to get socket
		socketfd = socket(result->ai_family,result->ai_socktype, result->ai_protocol);
		
		if (socketfd < 0){
			//	perror("Couldn't create socket");
			//exit(EXIT_FAILURE);
			continue;
		}
		
		//set socket options- allow reuse of local addresses
		if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_on, sizeof(reuse_addr_on))< 0){
			perror("couldn't setsockopt");
			exit(26);
		}

		printf("\n------[server] Socket FD------\n %d\n", socketfd);
		
		/* Associate the socket with a port using the bind system call*/
		int bind_res =0; 
		if (bind(socketfd, result->ai_addr, result->ai_addrlen) == 0){
			/* success */
			break; 
		}else{
			fprintf(stdout, "couldn't bind: %s\n", gai_strerror(bind_res));
			perror("couldn't bind");
			close(socketfd);
		}
		//next struct
	} while ((result=result->ai_next) != NULL);
	
	//if no socket could be binded
	if (result == NULL){
		printf("\ndo-while couldnt bind socket\n\n");
		exit(EXIT_FAILURE);
	}
	//struct addrinfo no longer needed
	freeaddrinfo(result);

	while(1){
	if (listen(socketfd, 1) < 0){
		perror("couldn't listen");
		exit(22);
	}
	printf("\n------ [server] Now Listening------\n");

	//Accept an incoming connection or add it to queue to be accepted later
	struct sockaddr sockaddr_client;
	int sockaddr_size = sizeof(sockaddr_client);

	//accept a new incoming connection, and give it it's own socket file descriptor
	int new_socketfd = accept(socketfd, &sockaddr_client, &sockaddr_size);

	if (new_socketfd < 0){
		perror("accept failed");
		exit(EXIT_FAILURE);
	}
	//changed from ~25 to 40
	char message_from_client[40];
	char message_acc[1050]={0};
	int bytes_read = 0;
	
	printf("\n[server]-------Server FDs:------");
	printf("\n[server] new_socketfd is: %d", new_socketfd);
	printf("\n[server] old socket is: %d\n", socketfd);

	char header1[20]={0};
	char header2[20]={0};
	char header3[20]={0};

	//use recv in loop to continously read from socket into buffer. Every iteration,
	//concatenate the buffer with the rest of the message.

	//sleep because i am running both the client and server on the same machine
	//and the client was printing to the screen between the "Received Messag" line
	//and the actual received message
	sleep(3);
	printf("\n[server]------Received-Message----:\n");

	while (1){
//		printf("\n**loop iteration**\n");
		bytes_read = recv(new_socketfd, message_from_client, sizeof(message_from_client), 0);
		if (bytes_read == 0){
			break;
			
		}else{
		strcat(message_acc, message_from_client);
		write(1, message_from_client, bytes_read);			
		
		}
	}
	/*server is done reading*/
	shutdown(new_socketfd, 0);

	sscanf(message_acc,"%s %s %s", header1, header2, header3); 

	printf("\n[server]----Parsed args:--------");

	//print HTTP request line ( GET/PUT, filename, HTTP version)
	printf("\nMethod: %s\nFile Name: %s\nHTTP Version: %s\n", header1, header2, header3);

	//see if method is GET or PUT and set appropriate bool to true

	if ( (strcmp(header1, put) == 0) ||
	     (strcmp(header1, PUT) == 0) ){
		putb = true;
		getb = false;
		printf("\n----[server] Method Detected-----\nPUT\n");
	}else if( (strcmp(header1, get) == 0) ||
		  (strcmp(header1, GET) == 0) ){
		getb = true;
		putb = false;
		printf("\n ----[server] Method Detected-----\nGET\n");
	}else{
		printf("\nMethod not recognized by server--we only accept GET or PUT\n");
		exit(EXIT_FAILURE);
	}

	FILE * save_file;
	FILE * file_to_save;
	char good_reply[]="200 OK";
	char bad_reply[]="404 Not Found";
	char file_name[25];
	strcpy(file_name, header2);

	// PUT
	if (putb == true){
		
		//open file name given by client
		printf("\n[server]-----in PUT---------\n");
		file_to_save = fopen(file_name, "w");
		//write entire http message to file
		fprintf(file_to_save, "%s", message_acc);
		if (file_to_save == NULL){
			perror("\n server couldnt fopen");
			exit(EXIT_FAILURE);
	
		}else{
			printf("\n[server]-----Saving File--------\n");
		}

		fclose(file_to_save);
//works?		sleep(1);
		//file wrote succesfully send response to client
		printf("\n[server]-----Sending Response--------\n");
		send(new_socketfd, good_reply, sizeof(good_reply), 0);
		//close(new_socketfd);
	}
	//endifputb
	if (getb == true){

/* Test */
//		int bytesread = 0;
//		char pass[10]={0};
//		int file_des = 0;
		printf("\n[server]-----in GET---------\n");
//		file_des = open(file_name, O_RDONLY);
//		if (file_des == -1){
//			perror("couldnt fopen");
//			exit(EXIT_FAILURE);
//		}		
/* test */
		printf("\n\n*******[in server]  just before the reply send to client for get*****\n\n");
		//see if file exists and if it can be read, and send reply to client accordingly
		if ( access ( file_name, R_OK) != -1){
			send(new_socketfd ,good_reply , sizeof(good_reply), 0);
		} else{
			send(new_socketfd ,bad_reply , sizeof(bad_reply), 0);
		}

		//	shutdown(new_socketfd, 1);
		printf("\n\n*******[in server]  just after the reply send to client for get*****\n\n");
/* Test */
		
		




/* Test */

		//	while( fscanf(save_file, "%s", pass ) > 0){
		//	send(new_socketfd, pass , strlen(pass), 0);
			//	}
			//	close(new_socketfd);
		//printf("\n\n*******[in server]  just after the last looped send to send clientfile file for get ********\n\n");

		close(new_socketfd);
	}
	


	close(new_socketfd); 
	
	}

	return 0;

}
