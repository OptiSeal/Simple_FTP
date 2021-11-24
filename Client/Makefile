#makefile for teststack
#the filename must be either Makefile or makefile

myftp: myftp.o token.o stream.o	
	gcc myftp.o token.o stream.o -o myftp
myftp.o: myftp.c token.h stream.h
	gcc -c myftp.c
token.o: token.c token.h
	gcc -c token.c
stream.o: stream.c stream.h	
	gcc -c stream.c
clean:	
	rm *.o

