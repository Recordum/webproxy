#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char* method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char* method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);

  int listenfd, connfd, clientfd, serverfd, proxy_to_server_fd;
  char hostname[MAXLINE], port[MAXLINE], buf[MAXLINE];
  struct stat sbuf;
  socklen_t clientlen, serverlen;
  rio_t rio;
  struct sockaddr_storage clientaddr, serveraddr;

    /* Check command line args */
  if (argc != 3) {
	  fprintf(stderr, "usage: %s <port> <server port>\n", argv[0]);
	  exit(1);
  }
  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
    if (connfd != -1){
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);
      break;
    }
  }
  proxy_to_server_fd = Open_clientfd(hostname, argv[2]);
  stat(connfd, &sbuf);

  Rio_readn(connfd, buf, sbuf.st_size);
  while(1){
    Rio_writen(proxy_to_server_fd, buf, sbuf.st_size);
    Rio_readn(proxy_to_server_fd, buf, sbuf.st_size);
    Rio_writen(connfd, buf, sbuf.st_size);
  }
  close(proxy_to_server_fd);
  close(connfd);
  close(listenfd);

  return 0;

}