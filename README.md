# Simple_FTP - A Simple File Transfer Protocol
# Authors: Jarryd Kaczmarczyk & Daniel Dobson

The simple FTP program uses standard socket programming
functions such as, connects, binds, listens, accepts, reads, and writes to connect to a
server/client in order to transfer data. There are no unorthodox data structures or methods
involved. The client communicates with the server along a TCP connection where the client
makes a request to change the directory, show current working directory, show directory
files, get a file from the server, and put a file on the server. Each command is attributed an
ASCII character prepended to the message as a header to inform the server of which
command has been issued. The server takes the header character, compares it to the
characters of the available commands, and executes that commands portion of code. For
messages with sizable attachments, the length of the number of bytes is also sent.
The server and client also send ASCII characters of 0 and 1 in the header for some commands
such as put and get to determine if the client/server is ready to accept or send the file.

The program is platform and programming language independent, and
the server can accept connections from any other program made with the same protocol
specifications. The ASCII character opcode representing the command is not very explanatory, 
but it is efficient. An alternative would be to use the shortened version of the command as the opcode,
such as pwd, granting a greater understanding of which opcode was being generated and
sent and the code readability. ASCII opcodes only deal with English-based
characters, limiting the program to English-based code/coders.
