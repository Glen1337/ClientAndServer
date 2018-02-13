/********************************************/
/* Glen Anderson 11/7/2015          
/* 91.413 Programming Assignment 1  
/* client.c
/********************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

typedef enum {
	true=1, false=0
} bool;

int main (int argc, char *argv []){

	/* Argument pointers */
	char *server = argv[1];
	char *endptr = argv[2];
	char *method = argv[3];
	char *filepath = argv[4];
	
	/* Port number holders */
	long portl =  strtol(argv[2], &endptr, 10);
	unsigned short port = 0;

	/* For comparing method input */
	char get[] = "get";
	char GET[] = "GET";
	char put[] = "put";
	char PUT[] = "PUT";

	bool getb = false;
	bool putb = false;

	/* Check for 5 arguments in total  */
	if (argc != 5) {
		printf("\n\n4 arguments not provided, you provided %d\n\n", argc-1);
		return 0;
	}

	/* Check for get or put, and make sure it is in the right place*/
	if ( (strcmp(method, get) == 0) ||
	     (strcmp(method, GET) == 0) ){
		getb = true;
	}
	else if ( (strcmp(method, put) == 0) ||
	     (strcmp(method, PUT) == 0) ){
		putb = true;
	}
	else {// if ( !putb && !getb  ) {
	     printf("\n\n ------INPUT ERROR------ \n\n GET or PUT not detected in right place\n");
	     exit(EXIT_FAILURE);
	}

	/* Check port # */
	if (portl > 65534){
		printf("\nPlease pick an in-range port number, you picked: %lu \n", portl);
		return 0;
	}
	else{
		port = (unsigned short) portl;
	}

	/* Give report of user input */
	printf("\n[client]------INPUT------\n");
	printf(" Host: %s \n Port: %d \n Method: %s \n File path: %s \n", server, port, method, filepath);

	struct addrinfo *result;
	struct addrinfo hints;
	struct sockaddr_in *addr2_in;
	struct sockaddr *addr2;
	int status;
	int gai_check = 0;

	/* Set hints in struct to give to getaddrinfo() */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ADDRCONFIG;

	/* Gets server address information using hints, ands stores it into result via addrinfo structs */
	gai_check = getaddrinfo(server, argv[2], &hints, &result);

	if(gai_check != 0){
		fprintf(stdout, "getaddrinfo error: %s \n", gai_strerror(gai_check));
		exit(24);	
	}

	//Get both versions of ai_addr struct returned by addrinfo()
	addr2_in = (struct sockaddr_in *) result -> ai_addr;
	addr2 = result -> ai_addr;       

	//print IP address of server
	int mysize = result->ai_addrlen;
	char ipaddr [mysize];
	memset (&ipaddr, 0, mysize);
	printf("\n[client]------Server IP Address------");

	struct sockaddr *my_sockaddr_ptr = result->ai_addr;
	struct sockaddr_in *my_sockaddr_in_ptr = (struct sockaddr_in* ) my_sockaddr_ptr;
	struct in_addr* my_struct_in_addr_ptr= &(my_sockaddr_in_ptr -> sin_addr);

	/* Call inet_ntop only to display IP address returned by getaddrinfo() */
	printf("\n %s\n", inet_ntop(addr2_in->sin_family, my_struct_in_addr_ptr, ipaddr, mysize));

	/* Create socket using socket() system call and struct addrinfo from getaddrinfo*/
	int sockfd = -1;
        sockfd = socket(result->ai_family, result->ai_socktype , result->ai_protocol);

	if (sockfd == -1){
		perror("couldn't create socket");
		exit(EXIT_FAILURE);
	}
	
	printf("\n[client]------Socket File Desc.------\n");
	printf(" FD: %d\n", sockfd);

//	Create a connection to the server with connect() using socket sockfd and server information in result struct 
	if (connect(sockfd, result->ai_addr, result-> ai_addrlen)== -1 ){
		perror("[client]could not connect socket");
		exit(EXIT_FAILURE);
		freeadrinfo(result);
	}
	else{
		printf("\n[client]----Socket is connected-----\n");
	}
	// Now connected, so struct no longer needed
	freeaddrinfo(result);

	struct stat stat_buf;   
	int fd;
/*	size_t totsize = 0; */
	char http_mess[]="";
	char buf_cont[20];

	// if method is PUT, we need to prepare to read it
	if (putb == true){
	
		/* Use fseek and ftell to obtain the size 
		   of the file to be written to the socket
		   because I could not get stat to work. */

		FILE *fp = fopen(filepath, "r+");
		int prev = ftell(fp);
		fseek(fp, 0L, SEEK_END);
		int sz = ftell(fp);
		fseek(fp,prev,SEEK_SET); //go back to where we were
		fclose(fp);

		/* Need to convert file size from an int to a string so it can be inserted into the HTTP message
		   Use snprintf to convert file size from an int to a string because itoa() seems to be unsupported.*/
		snprintf(buf_cont, 10, "%d", sz);
		printf("\n[client]---Num bytes seen in %s-- \n", filepath);
		printf(" %d\n", sz);
	}

	/*construct message differently based on wether method is GET or PUT */
	/* If the HTTP message is a GET, do this */
	if (getb == true){
		strcpy(http_mess, method);
		strcat(http_mess, " ");
		strcat(http_mess, filepath);
		strcat(http_mess, " HTTP/1.0");
		strcat(http_mess, "\r\n");
		strcat(http_mess, "Host: ");
		strcat(http_mess, server);
		strcat(http_mess, " \r\n\r\n");
	}

	/* But if the HTTP message is a PUT, do this */
	if (putb == true){
		strcpy(http_mess, method);
		strcat(http_mess, " ");
		strcat(http_mess, filepath);
		strcat(http_mess, " HTTP/1.0 ");
		strcat(http_mess, "\r\n");
		strcat(http_mess, "Host: ");
		strcat(http_mess, server);
		strcat(http_mess, "\r\n");
		strcat(http_mess, "Connection: Keep-Alive");
		strcat(http_mess, "\r\n");
		strcat(http_mess, "Content-Type: text/html");
		strcat(http_mess, "\r\n"); 
		strcat(http_mess, "Content-Length: ");
		strcat(http_mess, buf_cont);
		strcat(http_mess, "\r\n\r\n");
	}

	if(getb == true){

		printf("\n[client]----In GET-----------\n");
		//printf("\n[client]------Size of HTTP header message------\n %d bytes\n\n", ((unsigned int)strlen(http_mess)));
		printf("[client]------Sending HTTP GET message with header:-------\n\n%s",http_mess);
		printf("----------------------------------\n");
		
		/* Send HTTP message through the connected socket to the server */
		
		if (send(sockfd, http_mess, strlen(http_mess), 0) < 0) {
			perror("error with [client] send");
			exit(1);
		}
		shutdown(sockfd, 1);
		printf("\n[client]------HTTP GET Message Sent Successfully------\n");
		//receive response from server
		char rcv_msg [800]={0};
		sleep(1);
		
		//reply from server
		if (recv(sockfd, rcv_msg, sizeof(rcv_msg), 0) == -1){
			perror ("\ncouldn't recv()\n");
			exit(1);
		}

		close(sockfd);
		printf("\n[client]-----Reply Received and Socket Closed------\n");
		printf("[client]------Receive Message------\n\n");

		// Trim output due to glitch //
		rcv_msg[strlen(rcv_msg)] = '\0';
		//This prints garbage at end//printf(" %s \n", rcv_msg);
		write(1, rcv_msg, sizeof(rcv_msg));
		printf("\n[client]------End Receive Message------\n");


	}
	//////////////////////////
	if (putb == true){

		printf("\n[client]----In PUT-----------\n");
		printf("\n[client]-----Size of HTTP message:------\n %d bytes\n\n", ((unsigned int)strlen(http_mess)));
		printf("[client]----Sending HTTP Message Start: ------\n%s",http_mess);
		printf("[client]------Sending HTTP Message End----------\n");
		//                          Need strlen, not sizeof
		if (send(sockfd, http_mess, strlen(http_mess), 0) < 0) {
			perror("error with [client] send");
			exit(1);
		} else{
			printf("\n[client]---HTTP PUT Message Sent Successfully-----\n");
		}

//		printf(" \n[client]---- IN PUT______ \n");
		char buffer[10]="";
		char done[]="";
		int nread = 0;
		int f_des = open(filepath, O_RDONLY);
		char rcv_msg [600]={0};

		if (f_des == -1) {
			perror("\nclient couldn't open file to read\n");
			exit(25);
		}
		//keep reading data from file, and while there is more data, send it through socket to server
		while( (nread = read(f_des, buffer, sizeof(buffer))) > 0 ) {

			if ( send(sockfd, buffer, nread, 0) < 0) {
				perror("couldn't send file content through socket");
				exit(EXIT_FAILURE);
			}
			/*	write(1, buffer, nread); */
		}
		close(f_des);
		//give it a sec, then shut down client write side of socket
		sleep(1);
		shutdown(sockfd, 1);
//		char reply[10];
//		int bytes_read = 0;
//			
//			while(1){
//				bytes_read = recv(sockfd, reply, 
//	//receive reply from server
		   if (recv(sockfd, rcv_msg, sizeof(rcv_msg), 0) == -1){
			perror ("\ncouldn't recv()\n");
				exit(1);
			}
		close(sockfd);
//shutdown ??
		printf("[client]------Receive Message------\n");
		// Trim output due to glitch //
		rcv_msg[strlen(rcv_msg)] = '\0';
		printf("\n");//need write,print f prints garbage at end //printf(" %s \n", rcv_msg);
		write(1, rcv_msg, sizeof(rcv_msg));
		printf("\n");
		printf("\n[client]------End Receive Message------\n");


//	SO	char mystr [nread];
//		close(sockfd);
	}
//	if (putb == true){
	
	return 0;
}


/*	printf("---Bytes of Message Sent------\n %d",);
	if (putb == true){

//	Here I attempted to use sendfile() to easily send the file
//		   over the socket with ~1 function call, but I kept getting
//		   a connection closed by peer error, possibly due to the
//		   SSH connection

		//if (sendfile(sockfd, fd, NULL, stat_buf.st_size) < 0){
		//	perror("[client]couldnt sendfile()");
		//	exit(22);
		//	}
		
		// Send the file over the socket the hard way using
// system calls to open() the file, read() it into
//		   a buffer, write() the buffer to the socket, then
//		   close() the file 
		   
		char buffer1[10]={0};
		int buf_size = sizeof(buffer1);
		//	void *p = buffer1;
		int f_des = 0;

	
		while (1){
			// read 8 bytes into buffer
			int bytes_read_to_buf = read(f_des, buffer1, buf_size);
			if (bytes_read_to_buf == 0){
				write(1, '\0', 1);
				printf("\nEOF detected in read\n");
				break;
			}
	
			int bytes_wrote = write(1, buffer1, bytes_read_to_buf);
			if (bytes_wrote == -1) {
				printf("\n write error \n");
				perror("write");

			}
		}
		close(f_des);
	}
*/
