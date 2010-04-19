#ifndef __STD_H__
#define __STD_H__

#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/time.h>

void* std_calloc(size_t nmemb, size_t size);
void* std_malloc(size_t size);
void* std_realloc(void *ptr, size_t size);
void std_free(void* p);
void* std_memcpy(void *dst, const void *src, size_t n);
char* std_getenv(const char *name);
void std_gethostname(char* hostname, size_t len);
struct hostent* std_gethostbyname(const char* name);
void std_pipe(int filedes[2]);
void std_socketpair(int d, int type, int protocol, int sv[2]);
int std_socket(int domain, int type, int protocol);
void std_tcp_socketpair(int fds[2]);
ssize_t std_write(int fd, const void* buf, size_t count);
ssize_t std_read(int fd, void *buf, size_t count);

void std_bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);
void std_listen(int sockfd, int backlog);
int std_accept(int sockfd, struct sockaddr *addr, socklen_t* addrlen);

void std_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
void std_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int std_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

void std_close(int fd);
FILE* std_fopen(const char *path, const char *mode);
FILE* std_fdopen(int filedes, const char* mode);
int std_fclose(FILE* fp);

unsigned int std_sleep(unsigned int seconds);
void std_usleep(unsigned long usec);
void std_gettimeofday(struct timeval *tv, struct timezone *tz);

void std_pthread_create(pthread_t*  thread, pthread_attr_t* attr, void*(*start_routine)(void *), void* arg);
void std_pthread_join(pthread_t th, void **thread_return);

void std_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
void std_pthread_mutex_destroy(pthread_mutex_t *mutex);
void std_pthread_mutex_lock(pthread_mutex_t *mutex);
void std_pthread_mutex_unlock(pthread_mutex_t *mutex);

void std_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
void std_pthread_cond_destroy(pthread_cond_t *cond);
void std_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
void std_pthread_cond_broadcast(pthread_cond_t *cond);

void std_getsockname(int s, struct sockaddr *name, socklen_t *namelen);
void std_inet_aton(const char* dst_addr, struct in_addr *inp); // DEPRECATED
char* std_inet_ntoa(struct in_addr in); // DEPRECATED
const char* std_inet_ntop(int af, const void *src, char*dst, socklen_t cnt);
int std_inet_pton(int af, const void *src, void *dst);


#endif // __STD_H__
