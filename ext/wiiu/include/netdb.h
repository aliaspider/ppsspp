#ifndef	_NETDB_H
#define _NETDB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint32_t socklen_t;

#define NI_NUMERICHOST  2
#define NI_NUMERICSERV  8
#define NI_MAXHOST      1025
#define NI_MAXSERV      32

#define AI_ADDRCONFIG  0 /*TODO*/
#define AI_PASSIVE     1
#define AI_CANONNAME   2
#define AI_NUMERICHOST 4
#define AI_NUMERICSERV 8

#define EAI_AGAIN 2

struct addrinfo {
  int     ai_flags;
  int     ai_family;
  int     ai_socktype;
  int     ai_protocol;
  int     ai_addrlen;
  char   *ai_canonname;
  struct sockaddr  *ai_addr;
  struct addrinfo  *ai_next;
};

struct hostent
{
   char *   h_name;
   char **h_aliases;
   int   h_addrtype;
   int   h_length;
   char **h_addr_list;
#define  h_addr   h_addr_list[0]
};

int getaddrinfo(const char *node, const char *service, struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *__ai);
int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
struct hostent * gethostbyname(const char * name);
const char *gai_strerror(int);
/* TODO */
static inline int gethostname(char *name, size_t len)
{
   const char* src = "localhost";
   while(*src)
      *name++ = *src++;
   *name = '\0';
}

#ifdef __cplusplus
}
#endif

#endif	/* _NETDB_H */
