#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Make connections a pointer

void *get_in_addr(struct sockaddr *addr);
void broadcast_message(int connections[], char msg[]);
void add_connection(int connections[], int new_fd);

int MAX_CONNECTIONS = 50;
int num_clients;

int main(int argc, char *argv[]) {

  struct addrinfo hints, *res, *p;
  struct sockaddr_storage their_addr;
  int status, sockfd, b_status, l_status, new_fd;
  int connections[MAX_CONNECTIONS];
  socklen_t sin_size;
  char ipstr[INET6_ADDRSTRLEN]; // Holds the human readable IP
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints); // Clear hints array
  hints.ai_family = AF_UNSPEC;     // Either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Stream Socket
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, "3496", &hints, &res)) != 0) {
    printf("%s\n", gai_strerror(status));
  }

  for (p = res; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("Server: socket");
      continue;
    } else {
      printf("Socket descriptor initialized.\n");
    }
    if ((b_status = bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
      shutdown(sockfd, SHUT_RDWR);
      perror("Server: bind");
      continue;
    } else {
      printf("Bound socket descriptor sucecssfully.\n");
    }

    break;
  }

  freeaddrinfo(res);

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if ((l_status = listen(sockfd, 10)) == -1) {
    shutdown(sockfd, SHUT_RDWR);
    perror("Server: listen");
  } else {
    printf("Listening...");
  }

  while (1) {
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1) {
      perror("server: accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);

    printf("server: got connection from %s\n", s);

    int id = fork();

    if (id == -1) {
      perror("server: fork");
      continue;
    }

    if (id == 0) {
      close(sockfd);
      add_connection(connections, new_fd);
      char recv_msg[1024];
      while (1) {
        memset(recv_msg, 0, sizeof(recv_msg));
        ssize_t bytes_received;
        bytes_received = recv(new_fd, &recv_msg, sizeof(recv_msg) - 1, 0);
        if (bytes_received == -1) {
          perror("recv");
        }
        recv_msg[bytes_received] = '\0';
        broadcast_message(connections, recv_msg);
      }
    }
  }

  return EXIT_SUCCESS;
}

void *get_in_addr(struct sockaddr *addr) {
  if (addr->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)addr)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)addr)->sin6_addr);
}

void broadcast_message(int connections[], char msg[]) {
  printf("%s\n", msg);
  for (int i = 0; i < num_clients; i++) {
    if ((send(connections[i], msg, strlen(msg), 0)) == -1) {
      perror("broadcast");
      continue;
    }
  }
}

void add_connection(int connections[], int new_fd) {
  if (num_clients < MAX_CONNECTIONS) {
    connections[num_clients++] = new_fd;
    printf("Server: Added new connection.");
  } else {
    printf("Maximum client limit reached.");
    close(new_fd);
  }
}
