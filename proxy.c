#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct cache_storage{
  char* path;
  char* contents_buf;
  int contents_length;
  struct cache_storage* next_cache;
  struct cache_storage* previous_cache;
};
typedef struct cache_storage cache_storage;

typedef struct cache_storage;
void doit(int fd);
void *thread(void *vargp);
cache_storage* find_cache(char* path);
cache_storage* init_new_cache(char* client_path, size_t file_size, char* cachebuf);
void delete_cache(cache_storage* cache);
void insert_cache(cache_storage* cache);
int parse_uri(char *uri, char *port, char *path, char *hostname );
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";



static cache_storage* last_cache;

int main(int argc, char **argv) {
  printf("%s", user_agent_hdr);
  last_cache = NULL;
  int listenfd, *connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;
  
    /* Check command line args */
  if (argc != 2) {
	  fprintf(stderr, "usage: %s <port> <server port>\n", argv[0]);
	  exit(1);
  }
  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Malloc(sizeof(int)); //line:conc:echoservert:beginmalloc
	  *connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen); //line:conc:echoservert:endmalloc
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
	  Pthread_create(&tid, NULL, thread, connfd);
  }   
}

void *thread(void *vargp){
  int connfd = *((int*)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}

void doit(int fd){  
  int listenfd, connfd, clientfd, serverfd, proxy_to_server_fd, n, index, file_size;
  char buf[MAXLINE], totalbuf[MAXLINE], newbuf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], port[MAXLINE], hostname[MAXLINE];
  char* cachebuf, path;
  socklen_t clientlen, serverlen;
  rio_t rio, rio2;
  struct sockaddr_storage clientaddr, serveraddr;
  Rio_readinitb(&rio, fd);
  index = 0;
  strcpy(totalbuf, "");
  path = (char*)malloc(MAXLINE * sizeof(char));
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
  printf("uri %s\n", uri); 
  while(strcmp(buf,"\r\n")){  
    Rio_readlineb(&rio, buf, MAXLINE);
    strcat(totalbuf, buf);
    printf("buf : %s", buf);
  }
  
  cache_storage* cache = find_cache(path);
  if (cache != NULL){
    Rio_writen(fd, cache->contents_buf, cache->contents_length);
    return 0;
  }
  cachebuf = (char*)Malloc(MAX_OBJECT_SIZE * sizeof(char));
  strcpy(cachebuf,"");
  proxy_to_server_fd = Open_clientfd(hostname, port);
  Rio_writen(proxy_to_server_fd, totalbuf, strlen(totalbuf));
  Rio_readinitb(&rio2, proxy_to_server_fd);
  while((n = Rio_readlineb(&rio2, newbuf, MAXLINE)) > 0){
    Rio_writen(fd, newbuf, n);
    strcat(cachebuf, newbuf);
    file_size += n;
  }       
  // printf("cache_buf : %s\n", cachebuf);
  cache_storage* new_cache = init_new_cache(path, file_size, cachebuf);
  insert_cache(new_cache);
  printf("new_cache path : %s\n",new_cache->path);
  printf("last_cache path : %s\n",last_cache->path);
  Close(proxy_to_server_fd);  
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
  path[0] = '\0';
  if (path_index[strlen(path_index)-1] == '/'){
    strcat(path, "/");
    printf("path : %s\n", path);
  }else{
    strcpy(path, path_index);
  }
  return 0;
}
cache_storage* init_new_cache(char* client_path, size_t file_size, char* cachebuf){
  cache_storage* new_cache = (cache_storage*)malloc(sizeof(cache_storage));
  new_cache->contents_length = file_size;
  new_cache->contents_buf = cachebuf;
  new_cache->path = client_path;
  new_cache->next_cache = NULL;
  new_cache->previous_cache = NULL;
  printf("init_path : %s\n", client_path);
  return new_cache;
}

cache_storage* find_cache(char* client_path){
  cache_storage* current_cache = last_cache;
  
  while(1){
    if (current_cache == NULL){
      return NULL;
    }
    printf("clientpath : %s\n", client_path);
    printf("cachepath : %s\n", current_cache->path);
    printf("cachepath : %s\n", current_cache->contents_buf);
    printf("cachepath : %p\n", current_cache->next_cache);
    printf("cachepath : %p\n", current_cache->previous_cache);
    printf("cachepath : %d\n", current_cache->contents_length);
    
    if (current_cache->path == client_path){
      if (current_cache == last_cache){

        return current_cache;
      }
      delete_cache(current_cache);
      insert_cache(current_cache);
      return current_cache;
    }
    
    current_cache = current_cache->next_cache;
  }
}

void delete_cache(cache_storage* cache){
  cache->previous_cache->next_cache = cache->next_cache;
  cache->next_cache->previous_cache = cache->previous_cache;
}

void insert_cache(cache_storage* cache){
  if (last_cache == NULL){
    last_cache = cache;
  
    return;
  }
  last_cache->previous_cache = cache;
  cache->next_cache = last_cache;
  last_cache = cache;
  cache->previous_cache = NULL;
  
}


