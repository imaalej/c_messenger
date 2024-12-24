#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void send_to_server(int sockfd);
void recv_from_server(int sockfd);
void handle_exit(int sig);

int sockfd;

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: client ip_address port\n");
    return 0;
  }

  struct addrinfo hints, *res, *p;
  char ipstr[INET6_ADDRSTRLEN]; // Holds the human readible IP

  memset(&hints, 0, sizeof hints); // Clear hints array
  hints.ai_family = AF_UNSPEC;     // Either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Stream Socket

  int success;
  if ((success = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
    printf("%s", gai_strerror(success));
    exit(1);
  }
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  int status = connect(sockfd, res->ai_addr, res->ai_addrlen);

  freeaddrinfo(res); // Free the linked list

  int id = fork();

  if (id == 0) {
    recv_from_server(sockfd);
  } else {
    send_to_server(sockfd);
  }
  return EXIT_SUCCESS;
}

void send_to_server(int sockfd) {
  char send_msg[1024];
  while (1) {
    printf("\n");
    if (fgets(send_msg, sizeof(send_msg), stdin) == NULL) {
      perror("fgets");
      break;
    }
    send_msg[strcspn(send_msg, "\n")] = '\0';
    if (send(sockfd, &send_msg, strlen(send_msg), 0) == -1) {
      perror("send");
      break;
    }
  }
  close(sockfd);
  exit(EXIT_SUCCESS);
}

void recv_from_server(int sockfd) {
  char recv_msg[1024];
  ssize_t bytes_received;
  while (1) {
    memset(recv_msg, 0, sizeof(recv_msg));
    bytes_received = recv(sockfd, &recv_msg, sizeof(recv_msg) - 1, 0);
    if (bytes_received == -1) {
      perror("recv");
      break;
    }

    recv_msg[bytes_received] = '\0';
    printf("%s\n", recv_msg);
  }
  close(sockfd);
  exit(EXIT_SUCCESS);
}

void handle_exit(int sig) {
  (void)sig; // Unused parameter
  close(sockfd);
  exit(EXIT_SUCCESS);
}
