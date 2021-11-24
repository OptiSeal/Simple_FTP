/* File: stream.h
 * Authors: Jarryd Kaczmarczyk & Daniel Dobson
 * Date: 20/10/2021
 * Purpose: Head file for stream read and stream write.
 * Changes: 20/10/2021 - Added stream.c/stream.h, fixed implementation
 */


#define MAX_BLOCK_SIZE (1024*5)    /* maximum size of any piece of */
                                   /* data that can be sent by client */

/*
 * purpose:  read a stream of bytes from "fd" to "buf".
 * pre:      1) size of buf bufsize >= MAX_BLOCK_SIZE,
 * post:     1) buf contains the byte stream;
 *           2) return value > 0   : number of bytes read
 *                           = 0   : connection closed
 *                           = -1  : read error
 *                           = -2  : protocol error
 *                           = -3  : buffer too small
 */
int readn(int fd, char *buf, int bufsize);



/*
 * purpose:  write "nbytes" bytes from "buf" to "fd".
 * pre:      1) nbytes <= MAX_BLOCK_SIZE,
 * post:     1) nbytes bytes from buf written to fd;
 *           2) return value = nbytes : number of bytes written
 *                           = -3     : too many bytes to send
 *                           otherwise: write error
 */
int writen(int fd, char *buf, int nbytes);
