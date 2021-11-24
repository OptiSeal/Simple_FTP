/* File: myftp.c (CLIENT)
 * Authors: Jarryd Kaczmarczyk & Daniel Dobson
 * Date: 28/10/2021
 * Purpose: A simple FTP client
 * Changes:
 * 17/10/2021 - Added basic client layout (socket, connect, address), gets host and port
 * 19/10/2021 - Added do-while loop to send command
 * 20/10/2021 - Added client-side pwd, dir, cd, and tokenisation
 * 21/10/2021 - Added message header (P, C, D, G, U)
 * 22/10/2021 - Added additional message headers (0 = ok, 1 = file doesn't exist)
 * 23/10/2021 - Added put and get functionality
 * 24/10/2021 - Fixed put and get
 * 27/10/2021 - Added various error checks from local and remote server command functions and updated readDirFiles function
 * 27/10/2021 - Seperated some functionality in main to seperate functions & added comments
 *					- Reorganised functions in file: main at top and other functions follow in execution order
 *					- Setup of socket into socketSetup function
 *					- Getting user input into FTPExec function
 *					- Executing commands into locCommands and serverCommands functions			
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "token.h"
#include "stream.h"

#define SERV_TCP_PORT 41147     // Default server listening port
#define BUFSIZE (1024*5)		// Size of buffer

int socketSetup(unsigned short listen_port, char * listen_host);
void FTPExec(int loc_sock);
void locCommands(char **loc_token);
void serverCommands(char **loc_token, int loc_sock);
void readDirFiles(char response[]);
void getFile(int sock, char send[], char **token);
void sendFile(int sock, char send[], char **token);


/** MAIN function
 *
 *	Pre: TCP port number and buffer size must be predefined before execution
 *		 Syntax to execute program: "myftp [<host name> | <ip address>]"
 */
	int main(int argc, char *argv[]){
		
		int sock;                           	// Socket
		char host[60];                      	// Host address
		unsigned short port;    // Server listening port

		   // Get server IP and port number */
		if (argc==1) {  // Server running on the local host and on default port
			strcpy(host, "localhost");
			port = SERV_TCP_PORT;
		} else if (argc == 2) { // Get server IP and use default port
			strcpy(host, argv[1]);
			port = SERV_TCP_PORT;
		} else if (argc == 3) { // Get server IP and port
			strcpy(host, argv[1]);
			int n = atoi(argv[2]);          // Convert string to int

			if (n >= 1024 && n < 65536)
				port = n;
			else {
				printf("Error: server port number must be between 1024 and 65535\n");
				exit(1);
			}
		} else {
			printf("Syntax: %s <server host name> <server listening port>\n", argv[0]);
			exit(1);
		}
		
		//Setup socket
		sock = socketSetup(port, host);
		//Execute user commands
		FTPExec(sock);
		
		return 0;
		
	} //END of main function


/** Setup of socket - Socket is setup and connected to the server address
 *	
 *  Pre: Port number must be valid, host must be identified
 *	Post: Socket setup and connected 
 *	Return: Connected socket number (integer) returned to main
 */
	int socketSetup(unsigned short listen_port, char * listen_host){
		
		struct sockaddr_in ser_addr;        // Server address
		struct hostent *hp;                 // Host info
		int sock;
		
		// Erase data in memory starting at address
		bzero((char *) &ser_addr, sizeof(ser_addr));
		
		
		// Specify address for socket
		ser_addr.sin_family = AF_INET;
		ser_addr.sin_port = htons(listen_port);
		if ((hp = gethostbyname(listen_host)) == NULL){
			printf("Host %s not found\n", listen_host);
			exit(1);
		}
		ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;

		// Setup socket
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			  perror("Client socket");
			  exit(1);
		}

		// Connect socket to server
		if (connect(sock, (struct sockaddr *) &ser_addr, sizeof(ser_addr)) < 0) {
			perror("Client connect");
			exit(1);
		}
		
		return sock;
		
	} // END of socketSetup function


/** Execution of user input - Gets user input and passes input to local or server command functions
 *	
 *	Pre: Socket must be connected and predefined BUFSIZE must be provided
 *	Post: Input is set to lowercase, tokenised and sent to local or server function, otherwise,
 *		  user enters "quit" to return to main function
 */
	void FTPExec(int loc_sock){
		
		char input[BUFSIZE];
		char *token[BUFSIZE];
		
		// Get user input
		while(1) {
			printf("> ");       // Prompt
			fgets(input, BUFSIZE, stdin);   // Input

			int n = strlen(input);
			
			//Check input
			if((input[0] >= 'A' && input[0] <= 'Z') || (input[0] >= 'a' && input[0] <= 'z')){
				
				if (input[n-1] == '\n') {  // Replace newline
					input[n-1] = '\0';
					n--;
				}
				
				// Set command input to lowercase
				for (int i = 0; i <= strlen(input)-1; i++) {
					if(input[i] == ' ') 
						break;
					
					input[i] = tolower(input[i]); 
				}

				//If user enters "quit", return to main and quit program
				if(strcmp(input, "quit") == 0){
					printf("Bye from client\n");
					break;
				}else{
					tokenise(input, token);    // Tokenise input
					
					/*If input contains 'l', execute local command function,
					  otherwise server command function */
					if(strchr(token[0],'l'))
						locCommands(token);
					else
						serverCommands(token,loc_sock);
				}
			}else
				printf("Invalid input! Please try again.\n");
		}
		
	} //END of FTPExec function


/** Local command execution - Executes commands to display/change local directories, and display file names
 *
 *	Pre: Command (token) from user contains 'l' and buffer size has been predefined
 *	Post: Command requested by the user has been executed, or invalid command displayed to user
 */
	void locCommands(char **loc_token){
		
		char response[BUFSIZE];
		
		//lpwd Command - Display current directory of the client (INPUT FORMAT: "lpwd")
		if(strcmp(loc_token[0], "lpwd") == 0 && loc_token[1] == NULL){
			getcwd(response, sizeof(response));
			printf("Client working dir: %s\n", response);
		
		//ldir Command - Display the file names under the current directory of the client (INPUT FORMAT: "ldir")
		} else if(strcmp(loc_token[0], "ldir") == 0 && loc_token[1] == NULL){
			readDirFiles(response);
			
			//Read response from readDirFiles function. If successful (response = 0), display file names.
			if(response[0] == 1)
				printf("Could not open directory\n");
			else
				printf("Files in client working dir: %s\n", response);
		
		//lcd Command - Change the current directory of the client (INPUT FORMAT: "lcd <pathname>")
		} else if(strcmp(loc_token[0], "lcd") == 0 && loc_token[2] == NULL){
			if(loc_token[1] == NULL){	
				chdir("/");
				printf("Successfully changed to default directory \"/\"\n");
			}else{
				//If directory could not be successfully changed, display error.
				if(chdir(loc_token[1]) < 0)
					printf("Error changing directory\n");
				else
					printf("Directory successfully changed\n");
			}
		} else
			printf("Invalid input! Please try again.\n");
		
	} //END of locCommands function


/** Server command execution - Executes commands to display/change server directories, display file names
 *							   and send or retrieve files from/to the remote server		
 *
 *	Pre: User input (token) must exist, not contain 'l', buffer size has been predefined and socket connected
 *	Post: Command requested by the user has been executed, or invalid command displayed to user
 */
	void serverCommands(char **loc_token, int loc_sock){
		
		char send[BUFSIZE], response[BUFSIZE];
		
		//pwd Command - Display current directory of the server (INPUT FORMAT: "pwd")
		if(strcmp(loc_token[0], "pwd") == 0 && loc_token[1] == NULL){       // Server pwd
			strcpy(send, "P");     // Single ASCII character for header command
			writen(loc_sock, send, strlen(send) + 1);
			readn(loc_sock, response, sizeof(response));

			printf("Server working dir: %s\n", response);
		
		//dir Command - Display the file names under the current directory of the server (INPUT FORMAT: "dir")	
		} else if(strcmp(loc_token[0], "dir") == 0 && loc_token[1] == NULL){    // Server dir
			strcpy(send, "D");     // Single ASCII character for header command
			writen(loc_sock, send, strlen(send) + 1);
			
			//Read response from server. If successful (response != 0), display file names.
			readn(loc_sock, response, sizeof(response));
			if(response[0] == 1)
				printf("Server could not open directory\n");
			else
				printf("Files in server working dir: %s\n", response);
		
		//cd Command - Change the current directory of the client (INPUT FORMAT: "cd <pathname>")	
		} else if(strcmp(loc_token[0], "cd") == 0 && loc_token[2] == NULL){     // Server cd
			strcpy(send, "C");     // Single ASCII character for header command
			
			//Check token - must have no more than 2 tokens (1 for command and other for new dir)
			if(loc_token[1] != NULL)
				strcat(send, loc_token[1]);
			else if(loc_token[1] == NULL)
				strcat(send, "/");	//If no dir provided, set new dir to default system dir "/"
			else
				printf("Incorrect input\n");
			
			writen(loc_sock, send, strlen(send) + 1);
			
			//Read response from server (if response is not 0, server could not change dir)
			readn(loc_sock,response,sizeof(response));
			if(response[0] != 0)
				printf("Error changing directory\n");
			else
				printf("Directory successfully changed\n");
		
		//get Command - Retrieve the named file from the current directory of the server (INPUT FORMAT: "get <filename>")	
		} else if(strcmp(loc_token[0], "get") == 0 && loc_token[2] == NULL){    // Server get
			strcpy(send, "G");     // Single ASCII character for header command
			//If file name exists, get file from server, otherwise display error.
			if(loc_token[1] != NULL){
				strcat(send, loc_token[1]);
				getFile(loc_sock, send, loc_token);     // get file functionality
			}else
				printf("No file name provided!\n");
		
		//put Command - Send the named file to the current directory of the server (INPUT FORMAT: "put <filename>")	
		} else if(strcmp(loc_token[0], "put") == 0 && loc_token[2] == NULL){    // Server put
			strcpy(send, "U");      // Single ASCII character for header command
			//If file name exists, send file to server, otherwise display error.
			if(loc_token[1] != NULL){
				strcat(send, loc_token[1]);
				sendFile(loc_sock, send, loc_token);     // send file functionality
			}else
				printf("No file name provided!\n");
			
		} else
			printf("Invalid input! Please try again.\n");
		
	} //END of serverCommands function


/** Read directory file names - Function reads file names in the current directory and adds to array with newline separator
 *
 *	Pre: Command 'ldir' requested from the user, with empty char array provided, buffer size predefined
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
		}else
			strcpy(response, "1");
		
	} //END of readDirFiles function


/** Get file from remote server
 *
 *	Pre: Command 'get' requested from the user, a connected socket to the server, 
 *		 an array containing the requested file, address of token pointer and buffer size predefined.
 *	Post: Confirm file exists in the servers current directory, open the file,
 *		  write contents of file to the client current directory then close file
 */
	void getFile(int sock, char send[], char **token){
		int fd, n;
		char buf[BUFSIZE];
		char response[BUFSIZE];
		
		if(access(token[1], F_OK) ==0)
			printf("File already exists in the current client directory!\n");
		else{
			writen(sock, send, strlen(send) + 1);       // Send command code to server
			readn(sock, response, sizeof(response));    // Read response

			if(response[1] == '0'){     // If server ready and file exists
				strcpy(send, "H");      // Single ASCII character for header command
				strcat(send, "0");      // Single ASCII character for header command status

				writen(sock, send, strlen(send) + 1);       // Write ready status to server
				fd = open(token[1], O_WRONLY | O_CREAT, S_IRWXU);  // Open file

				while((n = readn(sock, buf, sizeof(buf))) > 0){     // Read file contents from server
					buf[n] = '\0';        // Null-terminator string
					write(fd, buf, n);    // Write contents to file
					if(n < BUFSIZE-2){
						break;
					}
				}

				close(fd);
				printf("File successfully downloaded from server\n");
			}else if(response[1] == '1')
				printf("File does not exist in the current server directory!\n");
			else
				printf("Server does not have permission to send the file!\n");
		}
		
	} //END of getFile function


/** Send file from remote server
 *
 *	Pre: Command 'put' requested from the user, a connected socket to the server, 
 *		 an array containing the requested file, address of token pointer and buffer size predefined.
 *	Post: Confirm file exists current directory, open the file,
 *		  send contents of file to the server current directory then close file.
 */
	void sendFile(int sock, char send[], char **token){
		char response[BUFSIZE];             // Test message recieved from server
		char data[BUFSIZE];
		char buf[BUFSIZE];
		int fd, size, n;
		
		if(access(token[1], F_OK) !=0)
			printf("File does not exist in the current client directory!\n");
		else{
			writen(sock, send, strlen(send) + 1);       // Send command code to server
			readn(sock, response, sizeof(response));     // Read response

			if(response[1] == '0'){         // If server ready and file exists
				fd = open(token[1], O_RDONLY, S_IRUSR);     // Open file

				while((n = read(fd, buf, BUFSIZE-1)) > 0){      // Read contents of file
					buf[n] = '\0';
					writen(sock, buf, n);   // Send contents to server
				}
				
				close(fd);
				printf("File successfully sent to server\n");
			}else if(response[1] == '1')
				printf("File already exists on the server!\n");
			else
				printf("Server does not have permission to accept the file!");
		}
		
	} //END of sendFile function

//END OF myftp (CLIENT)


