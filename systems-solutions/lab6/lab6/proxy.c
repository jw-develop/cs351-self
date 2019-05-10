#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "csapp.h"
#include "debug.h"
#include "clienthandler.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
/* the lab machines have 4 hyper-threaded cores */
#define NUM_THREADS 8
#define QUEUE_SIZE 32


/* You won't lose style points for including this long line in your code */
static char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


/* struct definitions */
struct simplequeue {
    int top;
    int data[QUEUE_SIZE];
};

/* function definition */
void *thread(void *vargp); 

/* the lock for clients */
pthread_mutex_t clientslock;
/* queued clients (with 0s afterwards) 
 * should not be modified unless the thread has clientslock*/
struct simplequeue clients;

/* code examples in chapter 11 of CSAPP used as a reference 
 * network code example from class used as reference 
 * some code copied from TINY server, the instruction seem to indicate that this is acceptable
 * www.cplusplus.com used strictly as a reference for standard c functions 
 * https://computing.llnl.gov/ used strictly as reference for pthreads
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
    /* the identifier for a thread */
    pthread_t tid;
    /* loop index variable */
    int i;


    /* initialize the clients lock */
    debug("initializing client lock\n");
    pthread_mutex_init(&clientslock, NULL);

    /*initialize clients */
    pthread_mutex_lock(&clientslock);
    clients.top = -1;
    pthread_mutex_unlock(&clientslock);

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


    /* initialize the threads */
    for (i = 0; i < NUM_THREADS; i++) {
        Pthread_create(&tid, NULL, thread, NULL);
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
        debug("recieved connection\n");
        /* add the connection to the queue */
        debug("main: locking\n");
        pthread_mutex_lock(&clientslock);
        debug("main: got lock\n");

        if (clients.top+1  < QUEUE_SIZE) {
            debug("adding connection to queue\n");
            clients.top++;
            clients.data[clients.top] = connfd;
        }
        else {
            debug("too many queued clients, refusing connection");
            Close(connfd);
        }
        pthread_mutex_unlock(&clientslock);
        debug("main: unlocking\n");

    }




    return 0;
}
/* the function the threads run */
void *thread(void *vargp) {
    /* the file desciptor of the connection this 
     * thread will handle */
    int connfd;

    /* detach the thread */
    Pthread_detach(pthread_self());

    /* take connections from the queque */
    while(1) {
        /* sleep so other functions have a chance to get 
         * the lock */
        /* remove a connection from the queue */
        pthread_mutex_lock(&clientslock);
        if (clients.top > -1) {
            connfd = clients.data[clients.top];
            clients.top--;
            pthread_mutex_unlock(&clientslock);

            /* give the connection and the address to a handler with the assumption 
             * that the handler will close the connection. I'm a litte concerned that 
             * passing connaddr by value might be inefficient, but I don't think it's 
             * large enough to matter*/
            debug("recieved connection from queue, passing connection to handleconnection\n");
            handleconnection(connfd, user_agent_hdr); 
        }
        else 
        {
            pthread_mutex_unlock(&clientslock);
        }
    }
}


