#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main() {
    char *ip = "127.0.0.1";
    int port = 5566;

    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    fd_set readfds;
    int max_fd;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP client socket created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-]Connection error");
        exit(1);
    }
    printf("[+]Connected to the server.\n");

    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, "Hii I am Client\n");
    send(client_sock, buffer, sizeof(buffer), 0);
    printf("Client: %s\n", buffer);

    bzero(buffer, BUFFER_SIZE);
    recv(client_sock, buffer, sizeof(buffer), 0);
    printf("Server : %s\n", buffer);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fileno(stdin), &readfds);
        FD_SET(client_sock, &readfds);
        max_fd = (fileno(stdin) > client_sock) ? fileno(stdin) : client_sock;

        printf("Enter command: ");
        fflush(stdout);

        // Wait for activity on either stdin or client socket
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("[-]Select error");
            exit(1);
        }

        // Check if there's input from stdin (user command)
        if (FD_ISSET(fileno(stdin), &readfds)) {
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
                perror("[-]Send error");
                exit(1);
            }

            if (strcmp(buffer, "/logout") == 0) {
                printf("[+]Logout request sent. Closing connection.\n");
                close(client_sock);
                exit(0);
            }
        }

        // Check if there's incoming data from the server (messages from other clients)
        if (FD_ISSET(client_sock, &readfds)) {
            bzero(buffer, BUFFER_SIZE);
            ssize_t bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
            if (bytes_received < 0) {
                perror("[-]Receive error");
                exit(1);
            } else if (bytes_received == 0) {
                printf("[+]Server closed the connection.\n");
                close(client_sock);
                exit(0);
            } else {
                printf("\nServer response: %s\n", buffer);
            }
        }
    }

    return 0;
}
