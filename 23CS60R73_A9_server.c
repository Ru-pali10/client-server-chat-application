#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>

int main() {
    char *ip = "127.0.0.1";
    int port = 5566;
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[1024];
    int n;
    FILE *file;
    char current_directory[1024];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]FTP server socket created.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Bind to the port number: %d\n", port);

    listen(server_sock, 5);
    printf("Listening...\n");

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        printf("[+]Client connected.\n");

        // Set the initial current directory
        getcwd(current_directory, sizeof(current_directory));

        while (1) {
            bzero(buffer, sizeof(buffer));
            recv(client_sock, buffer, sizeof(buffer), 0);
		buffer[strlen(buffer)-1]='\0';
            char *token = strtok(buffer, " ");
           
            if (token != NULL) {
            if (strcmp(token, "put") == 0) {
                char *filename = strtok(NULL, " ");
                if (filename != NULL) {
                    FILE *file = fopen(filename, "w");
                    if (file != NULL) {
                        while (1) {
                            bzero(buffer, sizeof(buffer));
                            int n = recv(client_sock, buffer, sizeof(buffer), 0);
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
            }else if (strcmp(token, "get") == 0) {
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
                                send(client_sock, buffer, n, 0);
                            }
                            fclose(file);
                            printf("File sent successfully: %s\n", filename);
                        } else {
                            printf("File not found: %s\n", filename);
                        }
                    }
                } else if (strcmp(token, "close") == 0) {
                    close(client_sock);
                    printf("[+]Client disconnected.\n");
                    break;
                } else if (strcmp(token, "cd") == 0) {
                    char *directory_name = strtok(NULL, " ");
                    if (directory_name != NULL) {
                        if (chdir(directory_name) == 0) {
                            getcwd(current_directory, sizeof(current_directory));
                            printf("Current directory changed to: %s\n", current_directory);
                        } else {
                            printf("Failed to change directory to: %s\n", directory_name);
                        }
                    }
                } else if (strcmp(token, "ls") == 0) {
                    struct dirent *entry;
                    DIR *dp = opendir(current_directory);
                    if (dp != NULL) {
                        while ((entry = readdir(dp))) {
                            send(client_sock, entry->d_name, strlen(entry->d_name), 0);
                            send(client_sock, "\n", 1, 0);
                        }
                        closedir(dp);
                    }
                } else {
                    printf("Invalid command\n");
                }
            }
        }
    }

    close(server_sock);
    return 0;
}

