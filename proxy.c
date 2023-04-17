#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);

  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen, serverlen;
  struct sockaddr_storage clientaddr, serveraddr;
  
    /* Check command line args */
  if (argc != 2) {
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
      doit(connfd);
    }
  }
}

void doit(int fd){  
  int listenfd, connfd, clientfd, serverfd, proxy_to_server_fd, n, index;
  char buf[MAXLINE], totalbuf[MAXLINE], newbuf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], port[MAXLINE], path[MAXLINE], hostname[MAXLINE];
  socklen_t clientlen, serverlen;
  rio_t rio, rio2;
  struct sockaddr_storage clientaddr, serveraddr;
  Rio_readinitb(&rio, fd);
  index = 0;
  while(strcmp(buf,"\r\n")){
    if (index == 0){
      Rio_readlineb(&rio, buf, MAXLINE);
      sscanf(buf, "%s %s %s", method, uri, version);
      parse_uri(uri, port, path, hostname);
      strcat(method, " ");
      strcat(method, path);
      strcat(method, " ");
      strcat(method, version);
      strcpy(buf, method);
      strcat(totalbuf, buf);
      printf("parse uri  : %s\n", totalbuf);
      index ++;
      continue;
    }
    Rio_readlineb(&rio, buf, MAXLINE);
    strcat(totalbuf, buf);
    printf("buf : %s", buf);
  }
  
  proxy_to_server_fd = Open_clientfd(hostname, port);

  Rio_writen(proxy_to_server_fd, totalbuf, strlen(totalbuf)); // 수정된 부분

  Rio_readinitb(&rio2, proxy_to_server_fd);

 
  while((n = Rio_readlineb(&rio2, newbuf, MAXLINE)) > 0){ // 수정된 부분
    Rio_writen(fd, newbuf, n); // 수정된 부분
  }                     
  close(proxy_to_server_fd);  



  return 0;

}

int parse_uri(char *uri, char *port, char *path, char *hostname ) {
  
  char *ptr;
  char *slash_index = strchr(uri, ':');
  char *host_index = slash_index + 3;
  char *port_index = strchr(slash_index + 1, ':') + 1;
  char *path_index = strchr(port_index + 1, '/');

  strncpy(hostname, host_index, strlen(host_index) - strlen(port_index) -1);
  strncpy(port, port_index, strlen(port_index) - strlen(path_index)); 
  if (path_index[strlen(path_index)-1] == '/'){
    strcat(path, "/");
    printf("path : %s\n", path);
  }else{
    strcpy(path, path_index);
  }
  return 0;
}