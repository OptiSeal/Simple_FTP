/* File: myftpd.c (SERVER)
 * Authors: Jarryd Kaczmarczyk & Daniel Dobson
 * Date: 28/10/2021
 * Purpose: A simple FTP server
 * Changes:
 * 17/10/2021 - Added basic server layout (socket, bind, listen, accept, address), gets port
 * 18/10/2021 - Made daemon process, make child proces per connection
 * 19/10/2021 - Added zombie claim, serve client, tokenise command, print working directory, change directory, show directory files
 * 20/10/2021 - Added stream.c/stream.h, fixed implementation
 * 21/10/2021 - Added message header (P, C, D, G, U)
 * 22/10/2021 - Added additional message headers (0 = ok, 1 = file doesn't exist)
 * 23/10/2021 - Added put and get functionality
 * 24/10/2021 - Fixed put and get
 * 27/10/2021 - Updated readDirFiles function (replaced space with newline character to separate each file and added ability to send error code to client if dir cannot be opened)
 * 			  	- Updated serveClient function (added ability to send the result of chdir() function to client)
 * 28/10/2021 - Added log functionality/output, added comments, sorted function placement and updated program execution syntax to allow an initial directory, 
 *			 	with port number hardcoded as per brief
 *			  - Seperated some functionality in main and serveClient to seperate functions, 
 *					- Setup of socket into socketSetup function
 *					- Connection to client into connectClient function
 *					- get functionality in getFile function
 *					- put functionality in putFile function
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include "stream.h"

#define SERV_TCP_PORT 41147     // Default server listening port
#define BUFSIZE (1024*5)


void daemonInit();
void claimChildren();
int socketSetup(unsigned short listen_port);
int connectClient(int loc_socket);
void serveClient(int sock);
void readDirFiles(char response[]);
void getFile(char *loc_buf, char command, int sock);
void putFile(char *loc_buf, char command, int sock);


/** MAIN function
*
*/
	int main(int argc, char *argv[]){
		int sock, newSock, fd;           						// Socket
		unsigned short port = SERV_TCP_PORT;                // Server listening port
		char logfilename[256]; // Test message recieved by server
		
		// Create log file
		sprintf(logfilename, "myftpd.log");
		fd = open(logfilename, O_WRONLY|O_CREAT|O_TRUNC, 0766);
		
		if(fd == -1)
			printf("Error: cannot open/create log file %s!\n", logfilename);
		else if((dup2 (fd, STDOUT_FILENO)) < 0)
			printf("Error: cannot redirect log file %s!\n", logfilename);
		
		// Check and get initial directory
		if (argc == 2) {
			chdir("/");
			if(chdir(argv[1]) < 0){     // Convert string to int
				fprintf(stderr,"Directory supplied does not exist.\n");
				exit(1);
			}
		} else if(argc > 2){
			fprintf(stderr,"Syntax: %s [ initial_current_directory ]\n", argv[0]);
			exit(1);
		}
		
		// Create daemon
		daemonInit();
			
		printf("Server pid = %d\n", getpid());

		sock = socketSetup(port);

		// Listen on socket
		listen(sock, 5);

		// Connect new client
		newSock = connectClient(sock);
		
		// In child process
		close(sock);

		// Serve the client connected
		serveClient(newSock);
		
	} //END of main function


/** Initiates daemon - Once created, the parent terminates and child continues to run as a daemon process
*
*/
	void daemonInit(){
		 pid_t   pid;
		 struct sigaction act;

		 if ((pid = fork()) < 0) {      // If pid less than 1
			  printf("Daemon fork error %s\n", strerror(errno));
			  exit(1);
		 } else if (pid > 0) {          // If parent
			  fprintf(stderr,"Remember PID: %d\n", pid);
			  printf("Parent exiting...\n");
			  exit(0);                  // Parent exits
		 }

		 // Child continues
		 setsid();                      // Session lead
		 umask(0);                      // Clear file mode creation mask
		 printf("New session created for child.\n");

		 // Catch SIGCHLD to remove zombies from system
		 act.sa_handler = claimChildren; // use reliable signal
		 sigemptyset(&act.sa_mask);       // not to block other signals
		 act.sa_flags   = SA_NOCLDSTOP;   // not catch stopped children
		 sigaction(SIGCHLD, (struct sigaction *) &act, (struct sigaction *) 0);
		 printf("Children processes caught\n");
		 
	} //END of daemonInit


/** Claims zombies
*
*/
	void claimChildren(){
		 pid_t pid=1;

		 while (pid>0) { // Claim zombies
			 pid = waitpid(0, (int *)0, WNOHANG);
		 }
		 
	} // END of claimChildren


/** Setup of socket - Socket is setup for use by multiple clients
 *	
 *  Pre: Port number must be valid
 *	Post: Socket setup and connected 
 *	Return: Connected socket number (integer) returned to main
 */
	int socketSetup(unsigned short listen_port){
		
		struct sockaddr_in ser_addr;        // Server address
		struct hostent *hp;                 // Host info
		int sock;
		
		// Erase data in memory starting at address
		bzero((char *) &ser_addr, sizeof(ser_addr));
		
		
		// Specify address for socket
		ser_addr.sin_family = AF_INET;
		ser_addr.sin_port = htons(listen_port);
		ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		// Setup socket
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("Server socket setup failed: %s\n", strerror(errno));
			exit(1);
		}

		// Bind socket
		if(bind(sock, (struct sockaddr *) &ser_addr, sizeof(ser_addr)) < 0){
			printf("Server bind failed: %s\n", strerror(errno));
			exit(1);
		}
		
		printf("Socket setup successful. Using socket %d\n", sock);
		
		return sock;
		
	} // END of socketSetup function
	
	
/** Connect new client
*	
*	Pre: Existing socket must be connected
*	Post: Client is connected to a server socket using their address and allocated to a child process
*   Return: New socket number (integer) 
*/
	int connectClient(int loc_socket){
		
		int newSock, isConnected = 0;
		pid_t   pid;                        // Process ID
		socklen_t cli_addr_len;             // Client address length
		struct sockaddr_in cli_addr; 		//Server and client address
		
		while(isConnected == 0){
			cli_addr_len = sizeof(cli_addr);    // Get client address length

			// Accept connection request
			newSock = accept(loc_socket, (struct sockaddr *) &cli_addr, (socklen_t *)&cli_addr_len);

			if(newSock < 0){
				if (errno == EINTR){   // If interrupted by SIGCHLD
					 continue;
				 }
				printf("Server accept failed: %s\n", strerror(errno));
				exit(1);
			}

			if((pid = fork()) < 0){
				printf("Error with fork: %s\n", strerror(errno));
				exit(1);
			} else if (pid > 0){
				close(newSock);     // Parent waits for connection
				continue;
			}
			
			isConnected = 1;
			printf("New client connected\n");
		}
		
		return newSock;
		
	} //END of connectClient function	


/** Serve client - Executes commands requested by the client. They include:
 *					pwd - to display current server directory, dir - displays file names in current server directory
 *					cd - change current server directory, get/put - send or receive files to/from client
 *
 *	Pre: Socket and client must be connected, buffer size has been predefined and socket connected
 *	Post: Commands requested by the user has been executed, or invalid command displayed to user
 */
	void serveClient(int sock){
		int nr, n, fd, chdir_result;
		char buf[BUFSIZE];
		char response[BUFSIZE];
		char filename[BUFSIZE];
		char command, code;

		while (1){
			 // Read data from client
			if ((nr = readn(sock, buf, sizeof(buf))) <= 0){
				printf("No data read from client. Connection from client stopped.\n");
				exit(1);   // Connection broken down
			}

			printf("Opcode %c received from client with a total of %d bytes recieved\n", buf[0], nr);

			command = buf[0];   // Get first character of string
			memmove(buf, buf+1, strlen(buf));   // Remove first character

			if(command == 'P'){      // pwd
				printf("pwd command received. Getting current working directory...\n");
				getcwd(response, sizeof(response));

				/* send results to client */
				writen(sock, response, strlen(response) + 1);
				printf("Current working directory %s returned to client\n", response);
			} else if(command == 'D'){  // dir
				printf("dir command received. Getting file names in current directory...\n");
				readDirFiles(response);

				/* send results to client */
				writen(sock, response, strlen(response) + 1);
				printf("File names in current directory sent to client\n");
			} else if(command == 'C'){  // cd
				printf("cd command received. Changing directory...\n");
				chdir_result = chdir(buf);
				if(chdir_result == -1)
					printf("Changing directory failed: %s\n", strerror(errno));
				else
					printf("Current directory successfully changed.\n");
				
				/* send chdir result to client */
				writen(sock, (char *) &chdir_result, 1);
			} else if(command == 'G' || command == 'H'){  // get
				getFile(buf,command,sock);
			} else if(command == 'U' || command == 'V'){  // put
				putFile(buf,command,sock);
			} else {
				 // Command not recognised
				 char unident[] = "Command not recognised.";
				 strcpy(response, unident);
				 writen(sock, response, strlen(response) + 1);
				 printf("%s.\n", unident);
			}
		}
		
	} // END of serveClient function


/** Read directory file names - Function reads file names in the current directory and adds to array with newline separator
 *
 *	Pre: Command 'dir' requested from the user, with empty char array provided, buffer size predefined
 *	Post: Directory pointer determined, and file names in current directory read. File names concatenated to the char array with newline separator
 *		  If file names could not be read, code '1' is read into the response char array back to the calling function
 */
	void readDirFiles(char response[]){
		DIR *dp;
		struct dirent *dirp;
		char directory[BUFSIZE];

		directory[0] = '\0';    // First is null terminator
		
		// Reopen directory to start from the start of directory
		if((dp = opendir(".")) != NULL){
			// Read directory names into array
			while((dirp = readdir(dp)) != NULL){
				strcat(directory, "\n");
				strcat(directory, dirp->d_name);
			}

			// Close directory
			closedir(dp);

			strcpy(response, directory);
		}else{
			strcpy(response, "1");
			printf("Directory read error %s\n", strerror(errno)); 
		}
		
	} //END of readDirFiles


/** get file - Function sends requested file to client
*
*	Pre: filename must exist in buffer, command from client must be 'G' or 'H' and socket must be connected.
*	Post: file data is written to socket (if file exists, can be accessed and if client is ready to accept the file)
*/
	void getFile(char *loc_buf, char command, int sock){
		
		int n, fd;
		char response[BUFSIZE], filename[BUFSIZE], code;
		char buf[BUFSIZE];
		
		strcpy(buf,loc_buf);
		
		if(command == 'G'){
			printf("get command received. Checking file %s exists...\n", buf);
			strcpy(response, "G");

			if(access(buf, F_OK) == 0){     // File exists
				strcat(response, "0");  // File exists & read access
				printf("File exists...\n");
				strcpy(filename, buf);
			} else {
				strcat(response, "1");  // File doesn't exist
				printf("File does not exist...\n");
			}

			writen(sock, response, strlen(response) + 1);
			printf("Acknowledgement sent to client\n");
		} else if(command == 'H'){  // get confirmed
			printf("Client ready to accept file. Sending...\n");
			code = buf[0];   // Get first character of string

			if(code == '0'){    // If server and client confirmed
				printf("Client ready to accept file. Sending...\n");
				fd = open(filename, O_RDONLY, S_IRUSR); // Open file

				while((n = read(fd, buf, sizeof(buf))) > 0){  // Transfer
					buf[n] = '\0';
					writen(sock, buf, n);
				}
				printf("File successfully sent to client\n");
			}else
				printf("Client not ready to accept file\n");
		}
		
	} //END of getFile function


/** put file - Function downloads file from client and places into current directory
*
*	Pre: filename must exist in buffer, command from client must be 'U' or 'V' and socket must be connected.
*	Post: file from client is placed into current directory (if does not already exist)
*/
	void putFile(char *loc_buf, char command, int sock){
		
		int n, fd;
		char response[BUFSIZE], filename[BUFSIZE], code;
		char buf[BUFSIZE];
		
		strcpy(buf,loc_buf);
		
		if(command == 'U'){
			printf("put command received. Checking file %s exists...\n", buf);
			strcpy(response, "U");

			if(access(buf, F_OK) == 0){ // Check file existance

				strcat(response, "1");  // File exists
				printf("File exists\n");
			} else {

				strcat(response, "0");  // Server ready
				strcpy(filename, buf);
				printf("File does not exist\n");
			}

			writen(sock, response, strlen(response) + 1);
			printf("Acknowledgement sent to client\n");

			if(response[1] == '0'){     // If server and client ready
				printf("Client sending file...\n");
				fd = open(filename, O_WRONLY|O_CREAT, S_IRWXU);     // Open file
				
				while ((n = readn(sock, buf, sizeof(buf))) > 0){    // Transfer
					buf[n] = '\0';
					write(fd, buf, n);
					
					if(n < sizeof(buf)-2){
						break;
					}
				}
				close(fd);      // Close file
				printf("File successfully received from client\n");
			}else
				printf("Client did not send file...\n");
		}
		
	} //END of putFile function
		
	
//END of myftpd (SERVER)
