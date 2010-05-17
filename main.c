/*****************************************************************************
 * udp_server: Control EIWOMISA over udp
 *****************************************************************************
 * Copyright (C) 2009-2010 Kai Hermann
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

#define VERSION "0.2"
#define PROGNAME "eiwomisarc_server"
#define COPYRIGHT "2009-2010, Kai Hermann"

#define EIWOMISA 0
#define ATMO 1

#define BUFFSIZE 19 /*FIXME: receive-puffer */

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

int check_EIWOMISA (unsigned char *buffer) {
	int error = 0;

	if(buffer[0] == 255) {
		msg_Dbg("buffer[0] == 255");
	} else {
		error = 1;
		msg_Dbg("buffer[0] != 255");
	}
	if(buffer[1] < 255) {
		msg_Dbg("buffer[1] <255");
	} else {
		error = 1;
		msg_Dbg("buffer[1] >= 255");
	}
	if(buffer[2] < 2) {
		msg_Dbg("buffer[2] <2");
	} else {
		error = 1;
		msg_Dbg("buffer[2] >= 2");
	}
	if(buffer[3] < 255) {
		msg_Dbg("buffer[3] <255");
	} else {
		error = 1;
		msg_Dbg("buffer[3] >= 255");
	}
	if(buffer[4] < 255) {
		msg_Dbg("buffer[4] <255");
	} else {
		error = 1;
		msg_Dbg("buffer[4] >= 255");
	}
	if(buffer[5] < 5) {
		msg_Dbg("buffer[5] <5");
	} else {
		error = 1;
		msg_Dbg("buffer[5] >= 255");
	}

	return error;
}

int check_ATMO (unsigned char *buffer) {
	int error = 0;

	if(buffer[0] == 0xFF) {
		msg_Dbg("buffer[0] == 0xFF");
	}
	if(buffer[3] <= 0x0F) {
		msg_Dbg("buffer[3] <= 0x0F");
	}else{
		error = 1;
		msg_Dbg("buffer[3] > 0x0F");
	}

	return error;
}

/* checkbuffer - debug=1 enables debug*/
int checkbuffer (unsigned char *buffer, int protocol) {
	switch (protocol) {
		case EIWOMISA:
			return check_EIWOMISA(buffer);
			break;
		case ATMO:
			return check_ATMO(buffer);
			break;
		default:
			return 1;
			break;
	}
}

/* mainloop */
int mymain(const char* progname, int port, char *serialport, int protocol, int baud)
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

	if(protocol > 1)
		protocol = EIWOMISA;

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

	int bufferlen = 0;
	if(protocol == EIWOMISA) {
		bufferlen = 6;
	}else if(protocol == ATMO) {
		bufferlen = 19;
	}

	/* wait for UDP-packets */
	while (1) {
		/* Receive a message from the client */
		clientlen = sizeof(client);

		if ((received = recvfrom(sock, buffer, bufferlen, 0,
								 (struct sockaddr *) &client,
								 &clientlen)) < 0) {
			die("Failed to receive message\n");
		}
		msg_Info("Client connected: %s", inet_ntoa(client.sin_addr));

		/* debug */
		int error = 0;

		checkbuffer(buffer, protocol); //FIXME: other serial protocols?

		if (error == 0) {
			msg_Dbg("buffer0-5: '%s'", buffer);

			//FIXME:
			if(protocol == EIWOMISA) {
				bufferlen = 6;
			}else if(protocol == ATMO) {
				bufferlen = (buffer[3]*3)+4;
			}

			/* RS-232 Code start */
			int fd = open_port(serialport, baud);
			int n = write(fd, buffer, bufferlen);

			if (n < 0)
				msg_Err("write() failed!");
			else
				msg_Dbg("Value(s) written to serial port");

			close(fd);
			/* RS-232 Code end */
		}
	}
    return 0;
}

int main(int argc, char **argv)
{
	struct arg_int *serverport = arg_int0("pP","port","","specify the serverport, default: 1337");
	struct arg_str *serialport = arg_str0("sS", "serial", "", "specify the serial port, default /dev/ttyS0");
	struct arg_int *protocol = arg_int0("", "protocol", "", "RS-232 protocol, 0=EIWOMISA, 1=ATMO");

	struct arg_int *baud = arg_int0("bB", "baud","","baudrate, default: 9600");

    struct arg_lit  *help    = arg_lit0("hH","help","print this help and exit");
    struct arg_lit  *version = arg_lit0(NULL,"version","print version information and exit");

	struct arg_lit  *debug = arg_lit0(NULL,"debug","print debug messages");
    struct arg_lit  *silent = arg_lit0(NULL,"silent","print no messages");

    struct arg_end  *end     = arg_end(20);

    void* argtable[] = {serverport,serialport,protocol,baud,help,version,debug,silent,end};

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
		printf("%s",VERSION);
		printf("\nGIT-REVISION: ");
		printf("%s",GITREV);
        printf("\nA server which receives udp-packets and controls\n");
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

	/* --debug enables debug messages */
    if (debug->count > 0) {
		printf("debug messages enabled\n");
		msglevel = 3;
	}

	/* --silent disables all (!) messages */
    if (silent->count > 0) {
		printf("i'll be silent now...\n");
		msglevel = 0;
	}

	exitcode = mymain(PROGNAME, i_serverport, i_serialport, protocol->ival[0], i_baudrate);

exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
}