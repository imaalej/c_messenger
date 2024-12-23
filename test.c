#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
  if(argc<2){
    printf("Please provide a domain name.\n");
    return 0;
  }

  struct addrinfo hints, *res, *p;
  char ipstr[INET6_ADDRSTRLEN]; // Holds the human readible IP
  
  memset(&hints, 0, sizeof hints); // Clear hints array
  hints.ai_family = AF_UNSPEC; // Either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Stream Socket

  int success;
  if ((success = getaddrinfo(argv[1], NULL, &hints, &res)) != 0){
    printf("%s", gai_strerror(success));
    exit(1);
  }

  for (p = res; p != NULL; p = p->ai_next){ // Iterate through result linked list
    void *addr;
    char *ipver;
    if(p->ai_family == AF_INET){ // If IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr; // Extract sockaddr_in struct (IPv4)
      addr = &(ipv4->sin_addr); // Extract IP address pointer
      ipver = "IPv4";
    } 
    else if(p->ai_family == AF_INET6){ // If IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr; // Extract sockaddr_in6 struct (IPv6)
      addr = &(ipv6->sin6_addr); // Extract IP address pointer
      ipver = "IPv6";
    }   
    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr); // Convert IP address to readible string, and store in ipstr
    printf("%s %s\n", ipver, ipstr);
  }
  freeaddrinfo(res); // Free the linked list
  return EXIT_SUCCESS;
}
