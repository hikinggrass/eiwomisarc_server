/*****************************************************************************
 * udp_server: Control EIWOMISA over udp
 *****************************************************************************
 * Copyright (C) 2009-2011 Kai Hermann
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

#include "git_rev.h"

#define VERSION "0.3"
#define PROGNAME "eiwomisarc_server"
#define COPYRIGHT "2009-2011, Kai Hermann"
#define BUFFSIZE 6

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

/* message functions */
#include "messages.h"

/* argtable */
#include "argtable2/argtable2.h"

/* signal handling */
#include <signal.h>

/* global serial port */
int global_serialport = -1;

/* signal handler */
void sigfunc(int sig) {
	if(global_serialport != -1) {
		close(global_serialport);
	}
	printf("\n");
	exit (0);
}

/* check for valid baudrate
 * throw _warning_ if baudrate is exotic */
int check_baudrate(int pBaud)
{
	switch (pBaud) {
		case B0: break;
		case B50: break;
		case B75: break;
		case B110: break;
		case B134: break;
		case B150: break;
		case B200: break;
		case B300: break;
		case B600: break;
		case B1200: break;
		case B1800: break;
		case B2400: break;
		case B4800: break;
		case B9600: break;
		case B19200: break;
		case B38400: break;
		default:
			msg_Dbg("BAUDRATE seems to be a little bit exotic\nbut hey, you'll know what you are doing ;)");
			break;
	}
	return 0;
}

/* open serial port
 * returns the file descriptor on success or -1 on error */
int open_port(const char *pPort, int pBaud)
{
	/* serial port file descriptor */
	int fd = open(pPort, O_RDWR | O_NOCTTY | O_NDELAY);
	
	if(fd == -1) {
		msg_Err("Unable to open serial-port");
		msg_Info("Retrying in 5 seconds...");
		sleep(5);
		fd = open(pPort, O_RDWR | O_NOCTTY | O_NDELAY);
	}
	
	if (fd == -1) {
		msg_Err("Unable to open serial-port");
	} else {
		fcntl(fd, F_SETFL, 0);
		
		struct termios options;
		
		/* get the current options for the port */
		tcgetattr(fd, &options);
		
		check_baudrate(pBaud);
		
		/* set baudrate */
		cfsetispeed(&options, pBaud);
		cfsetospeed(&options, pBaud);
		
		/* enable the receiver and set local mode */
		options.c_cflag |= (CLOCAL | CREAD);
		
		/* set the new options for the port */
		tcsetattr(fd, TCSANOW, &options);
		
		msg_Dbg("BAUDRATE SET TO %i",pBaud);
	}
	return (fd);
}

/* check if buffer is valid */
int checkbuffer (unsigned char *buffer) {
	int error = 0;

	/* byte0: startbyte = 255 */
	if(buffer[0] == 255) {
		msg_Dbg("buffer[0] == 255");
	} else {
		error = 1;
		msg_Dbg("buffer[0] != 255");
	}

	/* byte1: value part 1/2*/
	if(buffer[1] < 255) {
		msg_Dbg("buffer[1] <255");
	} else {
		error = 1;
		msg_Dbg("buffer[1] >= 255");
	}

	/* byte2: value part 2/2 */
	if(buffer[2] < 2) {
		msg_Dbg("buffer[2] <2");
	} else {
		error = 1;
		msg_Dbg("buffer[2] >= 2");
	}

	/* byte3: channel part 1/3 */
	if(buffer[3] < 255) {
		msg_Dbg("buffer[3] <255");
	} else {
		error = 1;
		msg_Dbg("buffer[3] >= 255");
	}

	/* byte4: channel part 2/3 */
	if(buffer[4] < 255) {
		msg_Dbg("buffer[4] <255");
	} else {
		error = 1;
		msg_Dbg("buffer[4] >= 255");
	}

	/* byte5: channel part 3/3 */
	if(buffer[5] < 5) {
		msg_Dbg("buffer[5] <5");
	} else {
		error = 1;
		msg_Dbg("buffer[5] >= 255");
	}

	return error;
}

/* mainloop */
int mymain(int port, char *serialport, int baud)
{
	/* check if port, serialport and baudrate are set, otherwise use defaults */
	if (port == -1) {
		msg_Info("No Port set - using 1337");
		port = 1337;
	}

	if (serialport == NULL) {
		msg_Info("No Serialport set - using /dev/ttyS0");
		serialport = "/dev/ttyS0";
	}

	if (baud == -1) {
		msg_Info("No Baudrate set - using 9600");
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
	
	/* signal handler */
	signal(SIGTERM,sigfunc);
	signal(SIGINT,sigfunc);

	/* open serial port */
	global_serialport = open_port(serialport, baud);
	
	/* wait for UDP-packets */
	while (42) {
		/* Receive a message from the client */
		clientlen = sizeof(client);

		if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
								 (struct sockaddr *) &client,
								 &clientlen)) < 0) {
			die("Failed to receive message\n");
		}
		msg_Info("Client connected: %s", inet_ntoa(client.sin_addr));

		if (checkbuffer(buffer) == 0) {
			msg_Dbg("buffer0-5: '%s'", buffer);

			/* RS-232 Code start */
			int n = write(global_serialport, buffer, BUFFSIZE);

			if (n < 0) {
				msg_Err("write() failed!");
			} else {
				msg_Dbg("Value(s) written to serial port");
			}
			/* RS-232 Code end */
		}
	}
	
	/* close serial port */
	close(global_serialport);
    return 0;
}

int main(int argc, char **argv)
{
	struct arg_int *serverport = arg_int0("pP","port","","serverport, default: 1337");
	struct arg_str *serialport = arg_str0("sS", "serial", "", "serial port, default /dev/ttyS0");

	struct arg_int *baud = arg_int0("bB", "baud","","baudrate, default: 9600");

    struct arg_lit  *help    = arg_lit0("hH","help","print this help and exit");
    struct arg_lit  *version = arg_lit0(NULL,"version","print version information and exit");

	struct arg_lit  *debug = arg_lit0(NULL,"debug","print debug messages");
    struct arg_lit  *silent = arg_lit0(NULL,"silent","print no messages");

    struct arg_end  *end     = arg_end(20);

    void* argtable[] = {serverport,serialport,baud,help,version,debug,silent,end};

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
		printf("usage: %s", PROGNAME);
        arg_print_syntax(stdout,argtable,"\n");
        arg_print_glossary(stdout,argtable,"  %-25s %s\n");
        exitcode=0;
        goto exit;
	}

    /* special case: '--version' takes precedence error reporting */
    if (version->count > 0) {
        printf("'%s' version ",PROGNAME);
		printf("%s",VERSION);
		printf("\nGIT-REVISION: ");
		printf("%s",GITREV);
        printf("\n%s receives udp-packets and controls\n",PROGNAME);
		printf("the EIWOMISA controller over RS-232\n");
        printf("%s",COPYRIGHT);
		printf("\n");
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

    /* special case: with no command line options induce brief help and use defaults */
    if (argc==1) {
		printf("No command-line options present, using defaults.\n",PROGNAME);
        printf("Try '%s --help' for more information.\n",PROGNAME);
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

	/* --debug enables debug messages */
    if (debug->count > 0) {
		printf("debug messages enabled\n");
		msglevel = 3;
	}

	/* --silent disables all (!) messages */
    if (silent->count > 0) {
		msglevel = 0;
	}

	exitcode = mymain(i_serverport, i_serialport, i_baudrate);

exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
}