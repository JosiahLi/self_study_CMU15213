#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"
/* Recommended max cache and object sizes */
#define NTHREADS 4
#define SBUFSIZE 16
/* You won't lose style points for including this long line in your code */

sbuf_t sbuf; /* all threads share the buffer */

typedef struct Url
{
    char host[MAXLINE];
    char port[MAXLINE];
    char uri[MAXLINE]; 
} Url;
/* --------------------the data below are shared by all threads--------------------------- */
Cache cache[NCACHE];
int cache_cnt;
char pool[NCACHE][MAX_OBJECT_SIZE];
int readcnt;
sem_t mutex, w;
size_t total_cache;
/* --------------------------------------------------------------------------------------- */
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_url(Url *, char *);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void build_header(rio_t *, char *, Url*);
/* ------------------------------------------------------- */
int reader(Url *url, char *temp_cache, size_t *size);
int writer(Url *url, char *temp_cache, size_t size);
/* ----------------------------------------------- */
void *thread(void *vargp);
/* ----------------------------------------------- */

int main(int argc, char **argv)
{
    int listenfd, connfd, i;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    Signal(SIGPIPE, SIG_IGN);
    /* Check command line args */
    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    cache_init(cache);
    sbuf_init(&sbuf, SBUFSIZE); /* initialize sbuf */
    Sem_init(&mutex, 0, 1), Sem_init(&w, 0, 1);
    /* initialize threads */
    for (i = 0; i < NTHREADS; ++i)
        Pthread_create(&tid, NULL, thread, NULL); 
        
    while (1) 
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        sbuf_insert(&sbuf, connfd);
    }

    return 0;
}

void doit(int fd) 
{
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char header[MAXBUF];
    char *pos;
    char *temp_cache = (char *)malloc(sizeof(char) * MAX_OBJECT_SIZE);
    rio_t rio;
    int serverfd;
    size_t n, size = 0;
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
    /* HIT */
    if (reader(to_server, temp_cache, &size) != -1)
    {
        //printf("HIT size = %ld\n", size);
        Rio_writen(fd, temp_cache, size);
        Free(to_server);
        Free(temp_cache);
        return;
    }
    /* MISS */
    //printf("MISS\n");
    serverfd = Open_clientfd(to_server->host, to_server->port);
    build_header(&rio, header, to_server); 
 
    Rio_writen(serverfd, header, strlen(header));
    Rio_readinitb(&rio, serverfd);

    pos = temp_cache;
    n = 0;
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        Rio_writen(fd, buf, n);
        size += n;
        if (size <= MAX_OBJECT_SIZE) 
        {
            memcpy(pos, buf, n);
            //printf("buf=%s\n", buf);
            pos += n;
        }
        printf("%ld bytes transfered to client\n", n);
    }
    
    writer(to_server, temp_cache, size);

    Free(temp_cache);
    Free(to_server);
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

    while ((cnt = Rio_readlineb(rio, buf, MAXLINE)) > 0)
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

void *thread(void *vargp)
{
    Pthread_detach(Pthread_self()); /* in order to avoid memory leak */
    while (1)
    {
        int connfd = sbuf_remove(&sbuf); /* sbuf_remove returns only if remove(get) a fd */
        doit(connfd);
        Close(connfd);
    }
}

int reader(Url *url, char *temp_cache, size_t *size)
{
    int idx;

    P(&mutex);
    readcnt++;
    if (readcnt == 1) P(&w);
    V(&mutex);
    /* reading */
    idx = cache_isin(cache, url->host, url->uri);
    if (idx != -1) /* Hit */
    {
        *size = cache[idx].state;
        memcpy(temp_cache, pool[idx], *size);
        P(&mutex);
        cache_update(cache);
        cache[idx].lru_cnt = 0;
        V(&mutex);
    }

    /* reading OK */
    P(&mutex);
    readcnt--;
    if (!readcnt) V(&w);
    V(&mutex);

    return idx;
}

int writer(Url *url, char *temp_cache, size_t size)
{
    int idx;
    P(&w);

    if (size > MAX_OBJECT_SIZE || total_cache + size > MAX_CACHE_SIZE)
    {
        V(&w);
        return - 1;
    }

    idx = cache_find(cache); 
    if (idx == -1)
    {
        idx = cache_evict(cache);
    }
    total_cache += size;
    printf("total = %ld\tsize = %ld\n", total_cache, size);
    cache_put(cache, idx, url->host, url->uri, size);
    cache_update(cache);
    memcpy(pool[idx], temp_cache, size);

    V(&w);
    return idx;
}