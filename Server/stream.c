/* File: stream.c
 * Authors: Jarryd Kaczmarczyk & Daniel Dobson
 * Date: 20/10/2021
 * Purpose: A simple FTP server
 * Changes:
 * 20/10/2021 - Added stream.c/stream.h, fixed implementation
 * 24/10/2021 - Change two-byte short int to four-byte int
 */

#include  <unistd.h>
#include  <sys/types.h>
#include  <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
#include  "stream.h"


/*
 * Read a stream of bytes from "fd" to "buf".
 * 
 * Pre:      1) size of buf bufsize >= MAX_BLOCK_SIZE,
 * Post:     1) buf contains the byte stream;
 *           2) return value > 0   : number of bytes read
 *                           = 0   : connection closed
 *                           = -1  : read error
 *                           = -2  : protocol error
 *                           = -3  : buffer too small
 */
int readn(int fd, char *buf, int bufsize){
    short data_size;    /* sizeof (short) must be 2 */
    int n, nr, len;

    /* check buffer size len */
    if (bufsize < MAX_BLOCK_SIZE)
         return (-3);     /* buffer too small */

    /* get the size of data sent to me */
    if (read(fd, (char *) &data_size, 1) != 1) return (-1);
    if (read(fd, (char *) (&data_size)+1, 1) != 1) return (-1);
    len = (int) ntohs(data_size);  /* convert to host byte order */

    /* read len number of bytes to buf */
    for (n=0; n < len; n += nr) {
        if ((nr = read(fd, buf+n, len-n)) <= 0)
            return (nr);       /* error in reading */
    }
    return (len);
}


/*
 * Write "nbytes" bytes from "buf" to "fd".
 * Pre:      1) nbytes <= MAX_BLOCK_SIZE,
 * Post:     1) nbytes bytes from buf written to fd;
 *           2) return value = nbytes : number of bytes written
 *                           = -3     : too many bytes to send
 *                           otherwise: write error
 */
int writen(int fd, char *buf, int nbytes){
    short data_size = nbytes;     /* short must be two bytes long */
    int n, nw;

    if (nbytes > MAX_BLOCK_SIZE)
         return (-3);    /* too many bytes to send in one go */

    /* send the data size */
    data_size = htons(data_size);
    if (write(fd, (char *) &data_size, 1) != 1) return (-1);
    if (write(fd, (char *) (&data_size)+1, 1) != 1) return (-1);

    /* send nbytes */
    for (n=0; n<nbytes; n += nw) {
         if ((nw = write(fd, buf+n, nbytes-n)) <= 0)
             return (nw);    /* write error */
    }
    return (n);
}
