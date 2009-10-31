/*****************************************************************************
 * udp_server: Control EIWOMISA over udp
 *****************************************************************************
 * Copyright (C) 2009 Kai Hermann
 *
 * Authors: Kai Hermann <kai.uwe.hermann at gmail dot com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#define VERSION "0.1"
#define PROGNAME "eiwomisarc_server"
#define COPYRIGHT "September-October 2009, Kai Hermann"
#define DEBUG 1

#define BUFFSIZE 6 /* receive-puffer */

/* UDP & other includes */
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

/* RS-232 */
#include <fcntl.h>   /* file control definitions */
#include <errno.h>   /* error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

/* argtable */
#include "argtable2/argtable2.h"

/* open serial port
 * returns the file descriptor on success or -1 on error */
int open_port(const char *pPort)
{
	/* serial port file descriptor */
	int fd = open(pPort, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd == -1) {
		perror("Unable to open serial-port");
	} else {
		fcntl(fd, F_SETFL, 0);

		struct termios options;

		/* get the current options for the port */
		tcgetattr(fd, &options);

		/* FIXME: Set the baud rates to 9600 */
		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);

		/* enable the receiver and set local mode */
		options.c_cflag |= (CLOCAL | CREAD);

		/* set the new options for the port */
		tcsetattr(fd, TCSANOW, &options);

		fprintf(stderr, "BAUDRATE SET TO 9600\n"); /* debug */
	}
	return (fd);
}

void die(char *message)
{
	perror(message);
	exit(1);
}


/* checkbuffer - debug=1 enables debug*/
int checkbuffer (unsigned char *buffer, int debug) {

	int error = 0;

	if(buffer[0] == 255) {
		if(debug > 0)
			fprintf(stderr, "buffer[0] == 255\n");
	} else {
		error = 1;
		if(debug > 0)
			fprintf(stderr, "buffer[0] != 255\n");
	}
	if(buffer[1] < 255) {
		if(debug > 0)
			fprintf(stderr, "buffer[1] <255\n");
	} else {
		error = 1;
		if(debug > 0)
			fprintf(stderr, "buffer[1] >= 255\n");
	}
	if(buffer[2] < 2) {
		if(debug > 0)
			fprintf(stderr, "buffer[2] <2\n");
	} else {
		error = 1;
		if(debug > 0)
			fprintf(stderr, "buffer[2] >= 2\n");
	}
	if(buffer[3] < 255) {
		if(debug > 0)
			fprintf(stderr, "buffer[3] <255\n");
	} else {
		error = 1;
		if(debug > 0)
			fprintf(stderr, "buffer[3] >= 255\n");
	}
	if(buffer[4] < 255) {
		if(debug > 0)
			fprintf(stderr, "buffer[4] <255\n");
	} else {
		error = 1;
		if(debug > 0)
			fprintf(stderr, "buffer[4] >= 255\n");
	}
	if(buffer[5] < 5) {
		if(debug > 0)
			fprintf(stderr, "buffer[5] <5\n");
	} else {
		error = 1;
		if(debug > 0)
			fprintf(stderr, "buffer[5] >= 255\n");
	}

	return error;
}

/* mainloop */
int mymain(const char* progname, int port, char *serialport, int baud)
{
	/* check if port, serialport and baudrate are set, otherwise use defaults */
	if (port == -1) {
		printf("No Port specified - using 1337.\n",progname);
		port = 1337;
	}

	if (serialport == NULL) {
		printf("No Serialport specified - using /dev/ttyS0.\n",progname);
		serialport = "/dev/ttyS0";
	}

	if (baud == -1) {
		printf("No Baudrate specified - using 9600.\n",progname);
		baud = 9600;
	}

	int sock;
	struct sockaddr_in server;
	struct sockaddr_in client;

	unsigned char buffer[BUFFSIZE];

	unsigned int clientlen, serverlen;
	int received = 0;

	/* create the UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		die("Failed to create socket\n");
	}

	/* construct the server sockaddr_in structure */
	memset(&server, 0, sizeof(server));			/* Clear struct */
	server.sin_family = AF_INET;				/* Internet/IP */
	server.sin_addr.s_addr = htonl(INADDR_ANY);	/* Any IP address */
	server.sin_port = htons(port);				/* server port */

	/* bind the socket */
	serverlen = sizeof(server);
	if (bind(sock, (struct sockaddr *) &server, serverlen) < 0) {
		die("Failed to bind server socket\n");
	}

	/* wait for UDP-packets */
	while (1) {
		/* Receive a message from the client */
		clientlen = sizeof(client);
		if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
								 (struct sockaddr *) &client,
								 &clientlen)) < 0) {
			die("Failed to receive message\n");
		}
		fprintf(stderr, "Client connected: %s\n", inet_ntoa(client.sin_addr));    

		/* debug */
		int error = 0;

		checkbuffer(buffer, DEBUG);

		if (error == 0) {
			fprintf(stderr, "buffer0-5: '%s'\n", buffer);

			/* RS-232 Code start */
			int fd = open_port(serialport);
			int n = write(fd, buffer, 6);

			if (n < 0)
				fputs("write() of 6 bytes failed!\n", stderr);
			else
				fprintf(stderr, "Value(s) written to serial port\n");   
			/* RS-232 Code end */
		}
	}
    return 0;
}

int main(int argc, char **argv)
{
	struct arg_int *serverport = arg_int0("pP","port","","specify the serverport, default: 1337");
	struct arg_str *serialport = arg_str0("sS", "serial", "", "specify the serial port, default /dev/ttyS0");
	struct arg_int *baud = arg_int0("bB", "baud","","baudrate, default: 9600");

    struct arg_lit  *help    = arg_lit0("hH","help",                    "print this help and exit");
    struct arg_lit  *version = arg_lit0(NULL,"version",                 "print version information and exit");

    struct arg_end  *end     = arg_end(20);

    void* argtable[] = {serverport,serialport,baud,help,version,end};

    int nerrors;
    int exitcode=0;

    /* verify the argtable[] entries were allocated sucessfully */
    if (arg_nullcheck(argtable) != 0) {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",PROGNAME);
        exitcode=1;
        goto exit;
	}

    /* set any command line default values prior to parsing */
	/* nothing */

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0) {
        printf("Usage: %s", PROGNAME);
        arg_print_syntax(stdout,argtable,"\n");
        printf("A server which receives udp-packets and controls\n");
		printf("the EIWOMISA controller over RS-232\n");
        arg_print_glossary(stdout,argtable,"  %-25s %s\n");
        exitcode=0;
        goto exit;
	}

    /* special case: '--version' takes precedence error reporting */
    if (version->count > 0) {
        printf("'%s' version ",PROGNAME);
		printf(VERSION);
        printf("\nA server which receives udp-packets and controls\n");
		printf("the EIWOMISA controller over RS-232\n");
        printf(COPYRIGHT);
        exitcode=0;
        goto exit;
	}

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0) {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout,end,PROGNAME);
        printf("Try '%s --help' for more information.\n",PROGNAME);
        exitcode=1;
        goto exit;
	}

    /* special case: uname with no command line options induces brief help */
    if (argc==1) {
		printf("No command-line options present, using defaults.\n",PROGNAME);
        printf("Try '%s --help' for more information.\n",PROGNAME);
        /* exitcode=0;
        goto exit; */
	}

    /* normal case: take the command line options at face value */

	/* check if server port is set */
	int i_serverport = -1;
	if(serverport->count>0)
		i_serverport = (int)serverport->ival[0];

	/* check if serial port is set */
	char* i_serialport = NULL;
	if(serialport->count>0)
		i_serialport = (char*)serialport->sval[0];

	/* check if baudrate is set */
	int i_baudrate = -1;
	if(baud->count>0)
		i_baudrate = (int)baud->ival[0];

	exitcode = mymain(PROGNAME, i_serverport, i_serialport, i_baudrate);

exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
}