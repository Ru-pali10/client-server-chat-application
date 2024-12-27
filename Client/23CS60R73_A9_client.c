#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    char *ip = "127.0.0.1";
    int port = 5566;
    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;
    char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]FTP client socket created.\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[-]Connection error");
        exit(1);
    }
    printf("Connected to the FTP server.\n");

    while (1) {
        printf("ftp_command :");
        fgets(buffer, sizeof(buffer), stdin);
        
        send(sock, buffer, strlen(buffer), 0);
      buffer[strlen(buffer)-1]='\0';
        char *token = strtok(buffer, " ");
        if (token != NULL) {
        	if (strcmp(token, "put") == 0) {
                    char *filename = strtok(NULL, " ");
                    if (filename != NULL) {
                        FILE * file = fopen(filename, "r");
                        if (file != NULL) {
                            while (1) {
                                bzero(buffer, sizeof(buffer));
                                int n = fread(buffer, 1, sizeof(buffer), file);
                                if (n <= 0) {
                                    break;
                                }
                                send(sock, buffer, n, 0);
                            }
                            fclose(file);
                            printf("File sent successfully: %s\n", filename);
                        } else {
                            printf("File not found: %s\n", filename);
                        }
                    }
                }else if (strcmp(token, "get") == 0) {
                char *filename = strtok(NULL, " ");
                if (filename != NULL) {
                    FILE *file = fopen(filename, "w");
                    if (file != NULL) {
                        while (1) {
                            bzero(buffer, sizeof(buffer));
                            int n = recv(sock, buffer, sizeof(buffer), 0);
                            if (n <= 0) {
                                break;
                            }
                            fwrite(buffer, 1, n, file);
                        }
                        fclose(file);
                        printf("File received successfully: %s\n", filename);
                    } else {
                        printf("Error opening file for writing: %s\n", filename);
                    }
                }
            } else if (strcmp(token, "cd") == 0) {
                char *directory_name = strtok(NULL, " ");
                if (directory_name != NULL) {
                    printf("Changing directory to: %s\n", directory_name);
                }
            } else if (strcmp(token, "ls") == 0) {
                while (1) {
                    bzero(buffer, sizeof(buffer));
                    int n = recv(sock, buffer, sizeof(buffer), 0);
                    if (n <= 0) {
                        break;
                    }
                    printf("%s", buffer);
                }
            } else if (strcmp(token, "close") == 0) {
                close(sock);
                printf("Disconnected from the FTP server.\n");
                break;
            } else {
                printf("Invalid command\n");
            }
        }
    }

    return 0;
}

