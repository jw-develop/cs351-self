#include <stdio.h>
#include <assert.h>
#include "csapp.h"
#include "debug.h"
#include "clienthandler.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* code examples in chapter 11 of CSAPP used as a reference 
 * network code example from class used as reference 
 * some code copied from TINY server, the instruction seem to indicate that this is acceptable
 * www.cplusplus.com used strictly a reference for standard c functions 
 */
int main(int argc, char** argv)
{
   /* the file descriptor of the listening socket */
   int listenfd; 
   /* the fd of a socket used to accept connections */
   int connfd;
   /* the socket address structure used to accept connections */
   SA connaddr;
   /* the size of the socket address structure */
   socklen_t connaddrlen;

   /* check if the user gave a port to listen at*/
   if (argc < 2) 
   {
       fprintf(stderr, "no port number given, aborting\n");
       exit(1);
   }
   else 
   {
       /* check if the given port is valid */
       int listenport = atoi(argv[1]);
       if (listenport <= 1024 || listenport >= 65536) {
           fprintf(stderr, "bad port number given, aborting\n");
           exit(1);
       }
   }

   /* open a listening socket */
   listenfd = Open_listenfd(argv[1]);
   debug("started listening on port %s\n", argv[1]);
   assert(listenfd > 0);

   /* loop while listening for clients */
   while (1) {
       /* store the size of the connaddr structure */
       connaddrlen = sizeof(connaddr);
       assert(connaddrlen > 0);
       /* accept a connection */
       connfd = Accept(listenfd, (SA *) &connaddr, &connaddrlen);
       debug("recieved connection, passing connection to handleconnection\n");
       /* give the connection and the address to a handler with the assumption 
        * that the handler will close the connection. I'm a litte concerned that 
        * passing connaddr by value might be inefficient, but I don't think it's 
        * large enough to matter*/
       handleconnection(connfd, connaddr, user_agent_hdr); 
   }



   return 0;
}


