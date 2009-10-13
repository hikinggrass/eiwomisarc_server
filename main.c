//UDP & gemeinsame includes
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

//RS-232
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */


/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port(void){
	int fd; /* File descriptor for the port */

	//fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
	fd = open("test", O_RDWR | O_NOCTTY | O_NDELAY);
	
	if (fd == -1){
		/* Could not open the port.*/
		perror("open_port: Unable to open /dev/ttyS1 - ");
	}else{
		fcntl(fd, F_SETFL, 0);
	}
	
	return (fd);
}

//code für udp
#define BUFFSIZE 6
void Die(char *mess) { perror(mess); exit(1); }

//mainloop
int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in echoserver;
	struct sockaddr_in echoclient;

	unsigned char buffer[BUFFSIZE];

	unsigned int clientlen, serverlen;
	int received = 0;

	if (argc != 2) {
		fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
		exit(1);
	}

	/* Create the UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		Die("Failed to create socket\n");
	}

	/* Construct the server sockaddr_in structure */
	memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
	echoserver.sin_family = AF_INET;                  /* Internet/IP */
	echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Any IP address */
	echoserver.sin_port = htons(atoi(argv[1]));       /* server port */

	/* Bind the socket */
	serverlen = sizeof(echoserver);
	if (bind(sock, (struct sockaddr *) &echoserver, serverlen) < 0) {
		Die("Failed to bind server socket\n");
	}

	/* Run until cancelled */
	while (1) {
		/* Receive a message from the client */
		clientlen = sizeof(echoclient);
		if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
								 (struct sockaddr *) &echoclient,
								 &clientlen)) < 0) {
			Die("Failed to receive message\n");
		}

		if(buffer[0] == 255){
			if( buffer[1] < 255)
			   fprintf(stderr, "buffer 1 <255\n"); //debug
			if(buffer[2] < 2)
			   fprintf(stderr, "buffer 2 <2\n"); //debug
			if(buffer[3] < 255)
			   fprintf(stderr, "buffer 3 <255\n"); //debug
			if(buffer[4] < 255)
			   fprintf(stderr, "buffer 4 <255\n"); //debug
			if(buffer[5] < 5) {
				fprintf(stderr, "buffer 5 <5\n"); //debug
				fprintf(stderr, "buffer0-5: '%s'\n", buffer); //debug
				//RS-232 Code
				int fd = open_port();
				int n = write(fd, buffer, 6);
				if (n < 0)
					fputs("write() of 6 bytes failed!\n", stderr);
			}
		}else{
			fprintf(stderr, "buffer0 != 254\n");
		}

		fprintf(stderr, "Client connected: %s\n", inet_ntoa(echoclient.sin_addr));    
	}
}
