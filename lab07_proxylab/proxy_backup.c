#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */


typedef struct Url
{
    char host[MAXLINE];
    char port[MAXLINE];
    char uri[MAXLINE]; 
} Url;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_url(Url *, char *);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void build_header(rio_t *, char *, Url*);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    Signal(SIGPIPE, SIG_IGN);
    /* Check command line args */
    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) 
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);                                             
        Close(connfd);                                            
    }

    return 0;
}

void doit(int fd) 
{
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char header[MAXBUF];
    rio_t rio;
    int serverfd;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  return;
    
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, url, version);       
    
    if (strcasecmp(method, "GET")) 
    {                     
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    Url *to_server = (Url *)Malloc(sizeof(Url)); 
    parse_url(to_server, url);
    //printf("server host: %s\tserver port: %s\n", to_server->host, to_server->port);
    serverfd = Open_clientfd(to_server->host, to_server->port);
    build_header(&rio, header, to_server); 

    Rio_writen(serverfd, header, strlen(header));
    Rio_readinitb(&rio, serverfd);

    while (rio_readlineb(&rio, buf, MAXLINE) != 0)
    {
        Rio_writen(fd, buf, strlen(buf));
    }

    Close(serverfd);
}

void build_header(rio_t * rio, char * header, Url* url)
{
    static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n"; 
    static const char *connection = "Connection: close\r\n";
    static const char *proxy_connection = "Proxy-Connection: close\r\n";
    char buf[MAXLINE];
    int cnt;

    cnt = sprintf(header, "GET %s HTTP/1.0\r\nHost: %s\r\n", url->uri, url->host);
    header += cnt;

    while ((cnt = Rio_readlineb(rio, buf, MAXLINE)) != 0)
    {
        if (!strcmp(buf, "\r\n")) break;
        if (!strncasecmp(buf, "Host:", 5)) continue;
        if (!strncasecmp(buf, "User-Agent:", 11)) continue;
        if (!strncasecmp(buf, "Connection:", 11)) continue;
        if (!strncasecmp(buf, "Proxy-Connection:", 18)) continue;

        cnt = sprintf(header, "%s", buf);
        header += cnt;
    }

    sprintf(header, "%s%s%s\r\n", user_agent_hdr,
            connection,
            proxy_connection);
}

int parse_url(Url *buf, char *url)
{
    char *host, *port, *uri;
    int val_port;

    if ((host = strstr(url, "//")))
    {
        host += 2;

        if ((port = strstr(host, ":")))
        {
            sscanf(port + 1, "%d", &val_port);
            sprintf(buf->port, "%d", val_port);
            //printf("val port = %d\n", val_port);
            *port = '\0';
        }
        else strcpy(buf->port, "80");

        if ((uri = strstr(port + 1, "/")))
        {
            strcpy(buf->uri, uri);
            *uri = '\0';
        }
        else return -1;     

        strcpy(buf->host, host);
    }
    else return -1;

    return 0;
}

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}