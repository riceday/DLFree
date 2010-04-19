#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "std.h"

void*
std_calloc(size_t nmemb, size_t size){
  void* p;
  if( ( p = calloc(nmemb, size) ) == NULL){
    perror("calloc");
    exit(1);
  }
  return p;
}

void*
std_malloc(size_t size){
  void* p;
  if( ( p = malloc(size) ) == NULL){
    perror("malloc");
    exit(1);
  }
  return p;
}

void*
std_realloc(void *ptr, size_t size){
  void* p;
  if( ( p = realloc(ptr, size) ) == NULL){
    perror("realloc");
    exit(1);
  }
  return p;
}

void
std_free(void* p){
  free(p);
}

void*
std_memcpy(void *dst, const void *src, size_t n){
  return memcpy(dst, src, n);
}


char*
std_getenv(const char *name){
  char* ret;
  if((ret = getenv(name)) == NULL){
    perror("getenv");
    exit(1);
  }
  return ret;
}

void
std_gethostname(char* hostname, size_t len){
  if( gethostname(hostname, len) == -1){
    perror("gethostname");
    exit(1);
  }
}

struct hostent*
std_gethostbyname(const char* name){
  struct hostent* h_ent;
  if((h_ent = gethostbyname(name)) == NULL){
    perror("gethostbyname");
    exit(1);
  }
  return h_ent;
}

void
std_pipe(int filedes[2]){
  if(pipe(filedes) == -1){
    perror("pipe");
    exit(1);
  }
}

void
std_socketpair(int d, int type, int protocol, int sv[2]){
  if(socketpair(d, type, protocol, sv) == -1){
    perror("socketpair");
    exit(1);
  }
}

int
std_socket(int domain, int type, int protocol){
  int sockfd;
  if((sockfd = socket(domain, type, protocol)) == -1){
    perror("socket");
    exit(1);
  }
  return sockfd;
}

void
std_tcp_socketpair(int fds[2]){
  const int on = 1;
  struct sockaddr_in local_addr;
  socklen_t addrlen = sizeof(local_addr);
  int lfd = std_socket(AF_INET, SOCK_STREAM, 0);
  fds[0] = std_socket(AF_INET, SOCK_STREAM, 0);

  /* setup listen sock structs */
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port = htons(0); /* bind to any port */
  std_setsockopt(lfd, SOL_TCP, TCP_NODELAY, &on, sizeof(on));

  /* listen */
  std_bind(lfd, (struct sockaddr*)&local_addr, addrlen);
  std_listen(lfd, 1);

  /* connect to local sock */
  memset(&local_addr, 0, sizeof(local_addr));
  std_getsockname(lfd, (struct sockaddr*)&local_addr, &addrlen);
  std_inet_aton("127.0.0.1", &local_addr.sin_addr);
  if(connect(fds[0], (const struct sockaddr*)&local_addr, sizeof(local_addr)) == -1){
    perror("tcpsocketpair");
    exit(1);
  }

  /* accept and create other end */
  fds[1] = std_accept(lfd, (struct sockaddr*)&local_addr, &addrlen);

  std_close(lfd);
}

ssize_t
std_write(int fd, const void* buf, size_t count){
  ssize_t c;
  if( (c = write(fd, buf, count)) == -1){
    perror("write");
    exit(1);
  }
  return c;
}

ssize_t
std_read(int fd, void *buf, size_t count){
  ssize_t c;

  if( ( c = read(fd, buf, count) ) == -1){
    perror("read");
    exit(1);
  }
  return c;
}

void
std_close(int fd){
  if( close(fd) == -1){
    perror("close");
    exit(1);
  }
}

FILE*
std_fopen(const char *path, const char *mode){
  FILE* fp;
  if((fp = fopen(path, mode)) == NULL){
    perror("fopen");
    exit(1);
  }
  return fp;
}

FILE*
std_fdopen(int filedes, const char* mode){
  FILE* fp;
  if((fp = fdopen(filedes, mode)) == NULL){
    perror("fdopen");
    exit(1);
  }
  return fp;
}

int
std_fclose(FILE* fp){
  int stat;
  if((stat = fclose(fp)) != 0){
    perror("fclose");
    exit(1);
  }
  return stat;
}

int
std_fcntl(int fd, int cmd, long arg){
  int ret;
  if((ret = fcntl(fd, cmd, arg)) == -1){
    perror("fcntl");
    exit(1);
  }
  return ret;
}

void
std_bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen){
  if(bind(sockfd, my_addr, addrlen) == -1){
    perror("bind");
    exit(1);
  }
}

void
std_listen(int sockfd, int backlog){
  if(listen(sockfd, backlog) == -1){
    perror("listen");
    exit(1);
  }
}

int
std_accept(int sockfd, struct sockaddr *addr, socklen_t* addrlen){
  int dstfd;
  if((dstfd = accept(sockfd, addr, addrlen)) == -1){
    perror("accept");
    exit(1);
  }
  return dstfd;
}

void
std_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen){
  if(getsockopt(s, level, optname, optval, optlen) == -1){
    perror("getsockopt");
    exit(1);
  }
}

void
std_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen){
  if(setsockopt(s, level, optname, optval, optlen) == -1){
    perror("setsockopt");
    exit(1);
  }
}


int std_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout){
  int numfd;
 std_select_begin:
  if( (numfd = select(nfds, readfds, writefds, exceptfds, timeout)) == -1){
    if(errno == EINTR) goto std_select_begin;
    perror("select");
    exit(1);
  }
  return numfd;
}

unsigned int
std_sleep(unsigned int seconds){
  return sleep(seconds);
}

void
std_usleep(unsigned long usec){
  usleep(usec);
}

void
std_gettimeofday(struct timeval *tv, struct timezone *tz){
  if(gettimeofday(tv, tz) == -1){
    perror("gettimeofday");
    exit(1);
  }
  return;
}



void
std_pthread_create(pthread_t*  thread, pthread_attr_t* attr, void*(*start_routine)(void *), void* arg){
  if(pthread_create(thread, attr, start_routine, arg) == -1){
    perror("pthread_create");
    exit(1);
  }
}

void
std_pthread_join(pthread_t th, void **thread_return){
  if(pthread_join(th, thread_return) == -1){
    perror("pthread_join");
    exit(1);
  }
}

void
std_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr){
  if(pthread_mutex_init(mutex, mutexattr) != 0){
    perror("pthread_mutex_init");
    exit(1);
  }
}

void
std_pthread_mutex_destroy(pthread_mutex_t *mutex){
  if(pthread_mutex_destroy(mutex) != 0){
    perror("pthread_mutex_destroy");
    exit(1);
  }
}

void
std_pthread_mutex_lock(pthread_mutex_t *mutex){
  if(pthread_mutex_lock(mutex) != 0){
    perror("pthread_mutex_lock");
    exit(1);
  }
}

void
std_pthread_mutex_unlock(pthread_mutex_t *mutex){
  if(pthread_mutex_unlock(mutex) != 0){
    perror("pthread_mutex_unlock");
    exit(1);
  }
}

void
std_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr){
  if(pthread_cond_init(cond, attr) != 0){
    perror("pthread_cond_init");
    exit(1);
  }
}

void
std_pthread_cond_destroy(pthread_cond_t *cond){
  if(pthread_cond_destroy(cond) != 0){
    perror("pthread_cond_destroy");
    exit(1);
  }
}

void
std_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex){
  if(pthread_cond_wait(cond, mutex) != 0){
    perror("pthread_cond_wait");
    exit(1);
  }
}

void
std_pthread_cond_broadcast(pthread_cond_t *cond){
  if(pthread_cond_broadcast(cond)){
    perror("pthread_cond_broadcast");
    exit(1);
  }
}


void
std_getsockname(int s, struct sockaddr *name, socklen_t *namelen){
  if(getsockname(s, name, namelen) == -1){
    perror("getsockname");
    exit(1);
  }
}

void
std_inet_aton(const char* dst_addr, struct in_addr *inp){
  if(inet_aton(dst_addr, inp) == 0){
    perror("inet_aton");
    exit(1);
  }
}

char*
std_inet_ntoa(struct in_addr in){
  char* in_str;
  if((in_str = inet_ntoa(in)) == NULL){
    perror("inet_ntoa");
    exit(1);
  }
  return in_str;
}

const char*
std_inet_ntop(int af, const void *src, char *dst, socklen_t cnt){
  const char* ret;
  if((ret = inet_ntop(af, src, dst, cnt)) == NULL){
    perror("inet_ntop");
    exit(1);
  }
  return ret;
}

int
std_inet_pton(int af, const void *src, void *dst){
  int ret;
  if((ret = inet_pton(af, src, dst)) <= 0){
    perror("inet_pton");
    exit(1);
  }
  return ret;
}

