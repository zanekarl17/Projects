#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SRVRBPORT "6666" // the port users will be connecting to

// get sockaddr, IPv4 or IPv6:
void *get_in_addr (struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &( ((struct sockaddr_in*)sa)->sin_addr );
  }
  return &( ((struct sockaddr_in6*)sa)->sin6_addr );
}

int main (void)
{
  int srvrBListeningFD;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;
  struct sockaddr_storage awsAddr;
  int bufSize = 2048;
  char *buf = (char *)calloc(bufSize, sizeof(*buf));
  socklen_t addr_len;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // set AF_INET to force IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use my IP
  if ( (rv = getaddrinfo(NULL, SRVRBPORT, &hints, &servinfo)) != 0 ) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ( (srvrBListeningFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1 ) {
      perror("listener: socket");
      continue;
    }
    if ( bind(srvrBListeningFD, p->ai_addr, p->ai_addrlen) == -1 ) {
      close(srvrBListeningFD);
      perror("listener: bind");
      continue;
    }
    break;
  }
  if (p == NULL) {
    fprintf(stderr, "listener: failed to bind socket\n");
    return 2;
  }
  freeaddrinfo(servinfo);
  printf("The ServerB is up and running using UDP on port %s.\n", SRVRBPORT);

  memset(buf, 0, bufSize * sizeof(*buf));
  addr_len = sizeof awsAddr;
  numbytes = recvfrom(srvrBListeningFD, buf, bufSize * sizeof(*buf), MSG_WAITALL, (struct sockaddr *)&awsAddr, &addr_len);
  if (numbytes == -1) {
    perror("recvfrom (awsAddr not filled)");
    exit(1);
  }
  printf("ServerB: got packet from %s\n", inet_ntop(awsAddr.ss_family, get_in_addr((struct sockaddr *)&awsAddr), s, sizeof s));
  printf("ServerB: packet is %d bytes long\n", numbytes);
  buf[numbytes] = '\0';
  printf("ServerB: packet contains \"%s\"\n", buf);
  
  memset(buf, 0, bufSize * sizeof(*buf));
  sprintf(buf, "ACK the message from AWS->ServerB");
  numbytes = sendto(srvrBListeningFD, buf, strlen(buf), 0, (struct sockaddr *)&awsAddr, addr_len);
  if (numbytes == -1) {
    perror("talker: sendto");
    exit(1);
  }
  
  close(srvrBListeningFD);
  
  return 0;
}