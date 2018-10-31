#ifndef UDP_H
#define UDP_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

static inline int udpFdBnd (const char *port, int options) {
  struct addrinfo hints, *result, *a;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // or AF_INET6 or AF_UNSPEC
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  int rc, fd;
  if ((rc = getaddrinfo(NULL, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo rc=%d, %s\n", rc, gai_strerror(rc));
    exit(EXIT_FAILURE);
  };
  for (a = result; a != NULL; a = a->ai_next) {
    fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (fd < 0) continue;
    if (bind(fd, a->ai_addr, a->ai_addrlen) == 0) break;
    close(fd);
  }
  freeaddrinfo(result);
  if (a == NULL) {
    fprintf(stderr, "Could not bind\n"); exit(EXIT_FAILURE);
  }
  return fd;
}

static inline int udpFd (const char *host, const char *port, int options) {
  struct addrinfo hints, *result, *a;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // or AF_INET6 or AF_UNSPEC
  hints.ai_socktype = SOCK_DGRAM;

  int rc, fd;
  if ((rc = getaddrinfo(host, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo rc=%d, %s\n", rc, gai_strerror(rc));
    exit(EXIT_FAILURE);
  };
  for (a = result; a != NULL; a = a->ai_next) {
    fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (fd < 0) continue;
    if (connect(fd, a->ai_addr, a->ai_addrlen) == 0) break;
    close(fd);
  }
  freeaddrinfo(result);
  if (a == NULL) {
    fprintf(stderr, "Could not connect\n"); exit(EXIT_FAILURE);
  }
  return fd;
}

#endif // UDP_H
