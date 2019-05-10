#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "csapp.h"
#include "clienthandler.h"
#include "debug.h"

/* constants */
#define NUM_HEADERS 4
#define MAX_REQUEST_HEADERS 50
#define MAX_RESPONSE_HEADERS 50
#define CONTENT_LEN_STR "Content-Length"

/* structure definitions */
struct header {
    char name[MAXLINE];
    char value[MAXLINE];
    struct header* next;
};
struct serverresponse {
    char* headers;
    void* body;
    int bodysize;
};

/* function definitions */
void clienterror(int fd, char *cause, char *errnum,
        char *shortmsg, char *longmsg);
struct serverresponse getresponse(char server[MAXLINE], char path[MAXLINE], char port[10], char* user_agent,
        struct header emptyhead, int nclientheaderss); 
void read_requesthdrs(rio_t *rp); 


/* 
 * handles a connection from a client
 *
 * precondition: clientfd is a valid connection from a client
 * postcondition: clientfd is closed
 */
void handleconnection(const int clientfd, char* user_agent) {
    /* the hostname of the connected client */
    char clienthostname[MAXLINE];
    /* the port of the connected client */
    char clientport[MAXLINE];
    /* the request line */
    char request[MAXLINE];
    /* the elements of the request */ 
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    /* the elements of the URI */
    char server[MAXLINE], path[MAXLINE], port[MAXLINE];
    /* a temporary header string holder */
    char headerstr[MAXLINE];
    /* two pointers to headers */
    struct header* currheader; 
    struct header* prevheader;
    /* an empty header that serves as the head of a
     * linked list of headers */
    struct header emptyhead;
    /* the number of headers recieved from the client */
    int nclientheaders;
    /* the read buffer */
    rio_t rio;
    /* the response recieved from the server */
    struct serverresponse response;
    /* a temporary int */
    int temp;
    /* a temporary string */
    char tmp[MAXLINE], tmpstr[MAXLINE];
    

    assert(clientfd > 2);
    
    /* get information about the client */
    /*
    Getnameinfo((SA *) &clientaddr, sizeof(clientaddr), clienthostname,
            MAXLINE, clientport, MAXLINE, 0);
    debug("connected to %s:%s\n", clienthostname, clientport);*/

    /* initialize input buffer */
    Rio_readinitb(&rio, clientfd);

    debug("initialized input buffer\n");
    /* read the request line */
    Rio_readlineb(&rio, request, MAXLINE);
    //debug("reading input");
    //read(clientfd, request, MAXLINE);
    debug("read request line\n");
    debug("request line: %s", request);
    /* extract the method, uri and version from the request */
    if (sscanf(request, "%s %s %s \r\n", method, uri, version) != 3) {
        debug("bad request, exiting handleconection\n");
        clienterror(clientfd, request, "400", "Bad Request", 
                "bad request");
        goto cleanup;
    }

    debug("method: %s uri: %s version: %s\n", method, uri, version);


    /* ensure the request is valid */
    /* check that the uri is valid */
    if (!strcasecmp(uri, "")){
        debug("empty uri, exiting handleconection\n");
        clienterror(clientfd, request, "400", "Bad Request", 
                "uri empty in request");
        goto cleanup;
    }
    debug("uri is non-empty\n");
    /* check that the method is correct */
    if (strcasecmp(method, "GET")) {
        debug("bad method, exiting handleconection\n");
        clienterror(clientfd, method, "501", "Not Implemented", 
                "This proxy only accepts GET requests, not requests of type");
        goto cleanup;
    }
    debug("method is valid\n");
    /* check that the version is correct */
    if (strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1")) {
        debug("bad version, exiting handleconection\n");
        clienterror(clientfd, request, "400", "Bad Request", 
                "HTTP version not recognized in request");
        goto cleanup;
    }
    debug("version is valid\n");
    /* extract the server and the path from the URI */ 
    if (strncasecmp(uri, "http://", strlen("http://")))
        temp = sscanf(uri, "%[^/]%s",tmpstr, path);
    else
        temp = sscanf(uri+7, "%[^/]%s", tmpstr, path);
    debug("scan read %d things from the uri\n", temp);
    if ( temp < 1) {
        debug("bad uri in request, exiting handleconection\n");
        clienterror(clientfd, request, "400", "Bad Request", 
                "bad uri in request");
        goto cleanup;
    } 
    if (temp == 1){ 
        path[0] = '/';
        path[1] = '\0';
    }

    /* figure out the port */
    debug("server: %s, path: %s\n", tmpstr, path);
    if (sscanf(tmpstr, "%[^:]:%s", server, port) == 1) {
        debug("server: %s   port: %s \n", server, port);
        debug("no specified port, so port was set to 80\n");
        port[0] = '8';
        port[1] = '0';
        port[2] = '\0';
    }
    debug("server: %s   port: %s \n", server, port);

    /* read headers, there probably won't be enough malloc calls to make 
     * the individual calls a performance issue*/
    prevheader = &emptyhead;
    nclientheaders = 0;
    debug("entering header storage loop\n");
    while (nclientheaders <= MAX_REQUEST_HEADERS &&
            Rio_readlineb(&rio, headerstr, MAXLINE) != EOF && strcmp(headerstr,"\r\n")) {
        debug("read header: %s", headerstr);
        currheader = calloc(sizeof(struct header),1); 
        if (sscanf(headerstr, "%[^:]:%s", currheader->name, currheader->value) != 2) {
            //debug("error reading headers, exiting handleconection\n");
            //clienterror(clientfd, request, "400", "Bad Request", 
            //        "couldn't read headers in request");
           // goto cleanup;
           break;
        } 
        debug("parsed header to name: %s and value: %s\n", currheader->name, currheader->value);
        prevheader->next = currheader;
        prevheader = currheader;
        nclientheaders++;
    }
    debug("out of header storage loop\n");
    assert(currheader != NULL);
    currheader->next = NULL;
    debug("checking if there are enough headers\n");
    if (nclientheaders > MAX_REQUEST_HEADERS) {
        debug("too many headers in request\n");
        goto cleanup;
    }



    /* get a response from the server */
    debug("calling response function\n");
    response = getresponse(server, path, port, user_agent, emptyhead, nclientheaders);
    debug("control returned to handeclient\n");

    if (response.headers == NULL) {
        debug("error getting response\n");
        goto cleanup;
    }

    
    /* send the response back to the client */
    debug("sending response headers back to client\n");
    Rio_writen(clientfd, response.headers, strlen(response.headers));
    debug("sending response body back to client\n");
    Rio_writen(clientfd, response.body, response.bodysize);
    debug("forwarded response headers to client:\n%s\nEND FORWARDED RESPONSE HEADERS\n",response.headers);

    /* free vars*/
    free(response.headers);
    free(response.body);
    debug("freed response\n");

    cleanup:
    debug("freeing headers\n");
    prevheader = emptyhead.next;
    while (prevheader != NULL) {
       debug("freeing %s header\n", prevheader->name);
       currheader = prevheader->next; 
       free(prevheader);
       prevheader = currheader;
    }
    emptyhead.next = NULL;
    Close(clientfd);
    debug("exiting handleconnection cleanly\n");
}
/* gets the response from the server given the address 
 * of the server and the path to a resource in the
 * server.
 *
 * returns: the response from the server
 *
 * return NULL on error
 */
struct serverresponse getresponse(char server[MAXLINE], char path[MAXLINE], char port[10], 
        char* user_agent, struct header emptyhead, int nclientheaders) {
    /* the file descriptor of the socket to be used for the connection */
    int fd;
    /* the request string */
    char request[MAXLINE];
    /* the response from the server */
    struct serverresponse response;
    /* the headers to send */
    char* headers[NUM_HEADERS]; 
    /* a header struct pointer*/
    struct header* currheader;
    /* loop index */
    int i;
    /* a string to hold headers */
    char headerstr[MAXLINE];
    /* the full request string */
    char* fullrequest;
    /* the content size of the response body */
    char responsesize[MAXLINE];
    /* a temporary string to hold a line of input*/
    char tmp[MAXLINE];
    /* the number of response headers recieved */
    int nresponseheaders;
    /* a read buffer */
    rio_t rb;

    debug("beginning execution of getresponse function\n");

    fullrequest = calloc(1,(NUM_HEADERS+nclientheaders+2)*MAXLINE);

    debug("opening connection from proxy to server on port: %s\n", port);
    /* create the socket */
    fd = Open_clientfd(server, port);
    debug("connected to server %s\n", server);
    if (fd < 0) {
        debug("error creating socket from proxy to server\n");
        response.headers = NULL;
        goto cleanup;
    }

    /* format the request string */
    if (sprintf(request, "GET %s HTTP/1.0\r\n", path) < 14) {
        debug("error creating request string\n");
        response.headers = NULL;
        goto cleanup;
    }
    debug("created request string: %s", request);

    /* fill headers */
    headers[0] = calloc(1,MAXLINE);
    debug("filling headers\n");
    if (sprintf(headers[0], "Host: %s\r\n", server) < 8) {
        debug("error creating Host header\n");
        response.headers = NULL;
        goto cleanup;
    }
    debug("filled header 0 with: %s", headers[0]);
    headers[1] = user_agent;
    debug("filled header 1 with: %s", headers[1]);
    headers[2] = "Connection: close\r\n";
    debug("filled header 2 with: %s", headers[2]);
    headers[3] = "Proxy-Connection: close\r\n";
    debug("filled header 3 with: %s", headers[3]);


    /* write request */ 
    debug("writing %s to server socket\n", request);
    strncat(fullrequest, request, strlen(request));

    /* write proxy headers */
    for (i = 0; i < NUM_HEADERS; i++) {
        debug("writing header: %s", headers[i]);
        strcat(fullrequest, headers[i]);
    }

    /* write client headers */
    for (currheader = emptyhead.next; currheader != NULL; currheader = currheader->next) {
        if (strcasecmp(currheader->name, "Host") &&
                strcasecmp(currheader->name, "Connection") &&
                strcasecmp(currheader->name, "Proxy-Connection") &&
                strcasecmp(currheader->name, "User-Agent")) {
            debug("writing client header %s: %s\r\n", currheader->name, currheader->value);
            sprintf(headerstr, "%s: %s\r\n", currheader->name, currheader->value);
            strcat(fullrequest, headerstr);
        }
    }

    /* write end CRLF */
    strcat(fullrequest, "\r\n");


    /* send full request */
    Rio_writen(fd, fullrequest, strlen(fullrequest));
    debug("sent message: \n%s\n END OF SENT MESSAGE\n", fullrequest);



    /* read response */
    debug("reading server response\n");
    /* initialize the read buffer */
    rio_readinitb(&rb,fd);

    response.headers = calloc(MAXLINE, MAX_RESPONSE_HEADERS);
    /* read the response headers */
    Rio_readlineb(&rb, tmp, MAXLINE);
    if (!strncasecmp(tmp, "HTTP/1.1", strlen("HTTP/1.1"))) {
       tmp[7] = '0';
    }
    strncat(response.headers, tmp, MAXLINE); 
    nresponseheaders = 1;
    responsesize[0] = '0';
    responsesize[1] = '\0';
    while (nresponseheaders < MAX_RESPONSE_HEADERS && strcmp(tmp,"\r\n")) {
        debug("read line %s", tmp);
        /* check for the the content length header */
        if (!strncasecmp(tmp, "Content-Length", strlen("Content-Length"))) {
            if (sscanf(tmp, "%*[^:]: %s", responsesize) != 1) {
                debug("problem reading response Content-Length header, got: %s\n",responsesize);
                response.headers = NULL;
                goto cleanup;
            }
        }

        Rio_readlineb(&rb, tmp, MAXLINE);
        strncat(response.headers, tmp, MAXLINE); 
        nresponseheaders++;
    }



    /* allocate room for response */
    //response = malloc(strlen(responseheaders) + responsesize+1);
    response.body = calloc(1,atoi(responsesize)+1);
    response.bodysize = atoi(responsesize);

    /* copy the response headers into response */
    //strncat(response, responseheaders, strlen(responseheaders));

    /* read the response body into response after the headers*/
    Rio_readnb(&rb, response.body, atoi(responsesize));
    //responsebody[responsesize] = '\0';
    //response = calloc(atoi(responsesize) + strlen(responseheaders) + 1,1);
    //strncat(response, responseheaders, strlen(responseheaders) +1);
    //strncat(response, responsebody, atoi(responsesize));
    //response[responsesize + strlen(responseheaders)] = '\0'; 



    debug("recieved response headers: \n%s\n END OF RECIEVED RESPONSE HEADERS\n", response.headers);
    debug("responsesize: %s\n", responsesize);
    //debug("responseheaders: \n%s\nEND RESPONSEHEADERS\n", responseheaders);


    cleanup:

    /* clean up memory, it is the callers job to free response*/
    debug("freeing the dynamically allocated header\n");
    free(headers[0]);
    /* close connection */
    debug("closing connection to server\n");
    Close(fd); 
    
    /* return response */
    debug("getresponse: returning response\n");
    return response;
}
//TODO fix clienterror
//TODO fix unix_error
//TODO replace asserts
//TODO check calloc return vals
//TODO SIGPIPE EPIPE thing
/*
 * copied from TINY server
 * clienterror - returns an error message to the client
 * 
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
        char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em></em>\r\n", body); 

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body)); 
}
/* $end clienterror */

