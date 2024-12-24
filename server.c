#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CONNECTIONS 100

void *get_in_addr(struct sockaddr *addr);
void broadcast_message(int connections[], char msg[]);
void add_connected_peer(int new_fd);
void remove_disconnected_peer(int new_fd);
void *handle_client(void *arg);

int num_clients;
int connections[MAX_CONNECTIONS];
pthread_t threads[MAX_CONNECTIONS];
pthread_mutex_t mutex;

struct client_data {
  int fd;
  char *name;
};

int main(int argc, char *argv[]) {

  struct addrinfo hints, *res, *p;
  struct sockaddr_storage their_addr;
  int status, sockfd, b_status, l_status, new_fd;
  socklen_t sin_size;
  char ipstr[INET6_ADDRSTRLEN]; // Holds the human readable IP
  char s[INET6_ADDRSTRLEN];

  pthread_mutex_init(&mutex, NULL);

  memset(&hints, 0, sizeof hints); // Clear hints array
  hints.ai_family = AF_UNSPEC;     // Either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Stream Socket
  hints.ai_flags = AI_PASSIVE;

  if (argc < 2) {
    printf("Please indicate port to use.");
    return 0;
  }

  if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
    printf("%s\n", gai_strerror(status));
  }

  for (p = res; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("Server: socket");
      continue;
    }

    if ((b_status = bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
      shutdown(sockfd, SHUT_RDWR);
      perror("Server: bind");
      continue;
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
  }

  printf("Server initialized and listening...\n");

  while (1) {
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1) {
      perror("server: accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);

    printf("server: got connection from %s\n", s);

    struct client_data *data = malloc(sizeof(struct client_data));
    if (data == NULL) {
      perror("malloc");
      close(new_fd);
      continue;
    }

    data->fd = new_fd;
    data->name = s;

    if (pthread_create(&threads[num_clients], NULL, &handle_client,
                       (void *)data) != 0) {
      perror("pthread create");
      close(new_fd);
      free(data->name);
      free(data);
      continue;
    }
  }

  pthread_mutex_destroy(&mutex);

  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);

  return EXIT_SUCCESS;
}

void *handle_client(void *arg) {
  char recv_msg[1024];
  char packed_msg[1024];

  struct client_data *local_data = (struct client_data *)arg;
  int fd = local_data->fd;

  add_connected_peer(fd);

  while (1) {
    memset(recv_msg, 0, sizeof(recv_msg));
    ssize_t bytes_received = recv(fd, &recv_msg, sizeof(recv_msg) - 1, 0);
    if (bytes_received == -1) {
      perror("recv");
    } else if (bytes_received == 0) {
      remove_disconnected_peer(fd);
      free(local_data->name);
      free(local_data);
      break;
    }
    recv_msg[bytes_received] = '\0';

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    int msg_len = snprintf(packed_msg, sizeof(packed_msg), "%s at %s: %s",
                           local_data->name, time_str, recv_msg);

    pthread_mutex_lock(&mutex);
    broadcast_message(connections, packed_msg);
    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

void *get_in_addr(struct sockaddr *addr) {
  if (addr->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)addr)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)addr)->sin6_addr);
}

void broadcast_message(int connections[], char msg[]) {

  for (int i = 0; i < num_clients; i++) {
    if ((send(connections[i], msg, strlen(msg), 0)) == -1) {
      perror("broadcast");
      continue;
    }
  }
}

void add_connected_peer(int fd) {
  pthread_mutex_lock(&mutex);

  if (num_clients < MAX_CONNECTIONS) {
    connections[num_clients++] = fd;
  } else {
    perror("Max peers reached.");
    close(fd);
  }
  pthread_mutex_unlock(&mutex);
}

void remove_disconnected_peer(int fd) {
  pthread_mutex_lock(&mutex);
  for (int i = 0; i < num_clients; i++) {
    if (connections[i] == fd) {
      for (int j = i; j < num_clients - 1; j++) {
        connections[j] = connections[j + 1];
      }
      num_clients--;
      break;
    }
  }
  close(fd);
  pthread_mutex_unlock(&mutex);
}
