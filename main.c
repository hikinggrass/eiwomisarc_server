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

//argtable
#include "argtable2/argtable2.h"

/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port(const char *pPort){
	int fd; /* File descriptor for the port */

	fd = open(pPort, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd == -1){
		/* Could not open the port.*/
		perror("open_port: Unable to open serial-port");
	}else{
		fcntl(fd, F_SETFL, 0);

		struct termios options;

		/*
		 * Get the current options for the port...
		 */

		tcgetattr(fd, &options);

		/*
		 * Set the baud rates to 9600...
		 */

		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);

		/*
		 * Enable the receiver and set local mode...
		 */

		options.c_cflag |= (CLOCAL | CREAD);

		/*
		 * Set the new options for the port...
		 */

		tcsetattr(fd, TCSANOW, &options);

		fprintf(stderr, "BAUDRATE SET TO 9600\n"); //debug

	}

	return (fd);
}

//code für udp
#define BUFFSIZE 6
void Die(char *mess) { perror(mess); exit(1); }

//mainloop
int mymain(const char* progname, int port, char *serialport, int baud)
{
	//check if ip & port are set, otherwise use defaults
	if(port == -1) {
		printf("No Port specified - using 1337.\n",progname);
		port = 1337;
	}

	int sock;
	struct sockaddr_in echoserver;
	struct sockaddr_in echoclient;

	unsigned char buffer[BUFFSIZE];

	unsigned int clientlen, serverlen;
	int received = 0;

	//if (argc != 2) {
	//	fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
	//	exit(1);
	//}

	/* Create the UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		Die("Failed to create socket\n");
	}

	/* Construct the server sockaddr_in structure */
	memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
	echoserver.sin_family = AF_INET;                  /* Internet/IP */
	echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Any IP address */
	echoserver.sin_port = htons(port);       /* server port */

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
		fprintf(stderr, "Client connected: %s\n", inet_ntoa(echoclient.sin_addr));    

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

				//RS-232 Code start
				int fd = open_port(serialport);
				int n = write(fd, buffer, 6);

				if (n < 0)
					fputs("write() of 6 bytes failed!\n", stderr);
				else
					fprintf(stderr, "Value(s) written to serial port\n");   
				//RS-232 Code end
			}
		}else{
			fprintf(stderr, "buffer0 != 254\n");
		}
	}

    return 0;
}

int main(int argc, char **argv)
{
	//init random number generator
	//srand(time(0));

	//server (ip adress, FIXME: check with regex)
	//struct arg_str *serverip = arg_str0("sS","server,ip","","specify the ip address of the server, default: localhost");

	struct arg_int *serverport = arg_int0("pP","port","","specify the serverport, default: 1337");
	/*struct arg_str *values = arg_strn("vV","values","",0,1,"specify up to 4 values separated by ',' - range 0-255, default: 0, negative values: random");
	struct arg_str *channels = arg_strn("cC","channels","",0,1,"specify up to 4 channels separated by ',' - range 0-512, default 0-3");
	struct arg_str *mixed = arg_strn("mM","mixed","",0,1,"set values for corresponding channels. Format: <channel0>,<value0>,<channel1>,[...]");
	*/
	struct arg_str *serialport = arg_str1("sS", "serial", "", "specify the serial port, default /dev/ttyS0");
	struct arg_int *baud = arg_int0("bB", "baud","","baudrate, default: 9600");

    struct arg_lit  *help    = arg_lit0("hH","help",                    "print this help and exit");
    struct arg_lit  *version = arg_lit0(NULL,"version",                 "print version information and exit");

    struct arg_end  *end     = arg_end(20);

    void* argtable[] = {serverport,serialport,baud,help,version,end};

	const char* progname = "udp_client_cmd"; //fixme!
    int nerrors;
    int exitcode=0;

    /* verify the argtable[] entries were allocated sucessfully */
    if (arg_nullcheck(argtable) != 0)
	{
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exitcode=1;
        goto exit;
	}

    /* set any command line default values prior to parsing */
	//nothing

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
	{
        printf("Usage: %s", progname);
        arg_print_syntax(stdout,argtable,"\n");
        printf("A client that sends udp-packets to a udp-server which controls\n");
		printf("the EIWOMISA controller over RS-232\n");
        arg_print_glossary(stdout,argtable,"  %-25s %s\n");
        exitcode=0;
        goto exit;
	}

    /* special case: '--version' takes precedence error reporting */
    if (version->count > 0)
	{
        printf("'%s' version 0.1\n",progname);
        printf("A client that sends udp-packets to a udp-server which controls\n");
		printf("the EIWOMISA controller over RS-232\n");
        printf("September-October 2009, Kai Hermann\n");
        exitcode=0;
        goto exit;
	}

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
	{
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout,end,progname);
        printf("Try '%s --help' for more information.\n",progname);
        exitcode=1;
        goto exit;
	}

    /* special case: uname with no command line options induces brief help */
    if (argc==1)
	{
        printf("Try '%s --help' for more information.\n",progname);
        exitcode=0;
        goto exit;
	}

    /* normal case: take the command line options at face value */

	//check if server ip is set
	//char *c_serverip = NULL;
	//if(serverip->count>0)
	//	c_serverip = (char *)serverip->sval[0];

	//check if server port is set
	int i_serverport = -1;
	if(serverport->count>0)
		i_serverport = (int)serverport->ival[0];
	//int mymain(const char* progname, int port, char *serialport, int baud);

	exitcode = mymain(progname, i_serverport, (char*)serialport->sval[0], (int)baud->ival[0]);

exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
}