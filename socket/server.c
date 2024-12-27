#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "uuid4.h"
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>

#define READ_END 0
#define WRITE_END 1
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOG_FILE "server_log.txt"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_FAQ_ENTRIES 231

typedef struct {
    int sockfd;
    int id;
    int chatbot_active;
    int chatbot_v2;
    char uuid[UUID4_STR_BUFFER_SIZE];
} Client;

typedef struct {
    char question[BUFFER_SIZE];
    char answer[BUFFER_SIZE];
} FAQ_Entry;

FAQ_Entry faq[MAX_FAQ_ENTRIES];

void initialize_faq() {
    // Read FAQ data from file
    FILE *faq_file = fopen("./socket/FAQs.txt", "r");
    char buff[BUFFER_SIZE];
    int i=0;
    //FAQ_Entry faq[231];
    if (faq_file == NULL) {
        perror("[-]Error opening FAQ file");
        exit(EXIT_FAILURE);
    }

    while (fgets(buff, BUFFER_SIZE, faq_file) != NULL)  {
        char *token = strtok(buff, "|||");
        if (token != NULL) {
            strcpy(faq[i].question, token);
            token = strtok(NULL, "|||");
            if (token != NULL) {
                strcpy(faq[i].answer, token);
                i++;
            }
        }
    }
}
/* void initialize_mutexes() {
    if (pthread_mutex_init(&log_mutex, NULL) != 0 || pthread_mutex_init(&clients_mutex, NULL) != 0) {
        perror("[-]Error initializing mutex");
        exit(EXIT_FAILURE);
    }
} */
// Function to remove trailing spaces from a string
void remove_trailing_spaces(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}
char* generate_uuid() {
    UUID4_STATE_T state;
    UUID4_T uuid;
    static char buff[UUID4_STR_BUFFER_SIZE];

    uuid4_seed(&state);
    uuid4_gen(&state, &uuid);

    if (!uuid4_to_s(uuid, buff, sizeof(buff)))
        return NULL;

    return buff;
}
    
void log_message(const char *message) {
    pthread_mutex_lock(&log_mutex);
    FILE *log_file = fopen("server_log.txt", "a");
    if (log_file == NULL) {
        perror("[-]Error opening log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    fprintf(log_file, "%s", message);
    fclose(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void handle_new_connection(int server_sock, Client *clients, int *num_clients, fd_set *readfds, int *max_sd) {
    struct sockaddr_in client_addr;
    socklen_t addr_size;
    int client_sock;
    char buffer[BUFFER_SIZE];
    char log_buffer[BUFFER_SIZE+20];
    addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    if (client_sock < 0) {
        perror("[-]Accept error");
        exit(EXIT_FAILURE);
    }
    printf("[+]Client connected.\n");

    FD_SET(client_sock, readfds);
    if (client_sock > *max_sd) {
        *max_sd = client_sock;
    }

    bzero(buffer, BUFFER_SIZE);
    recv(client_sock, buffer, sizeof(buffer), 0);
    printf("Client: %s\n", buffer);
    bzero(log_buffer, BUFFER_SIZE);
    snprintf(log_buffer,sizeof(log_buffer),"Client : %s",buffer);
    log_message(log_buffer);


    char *uuid = generate_uuid();
    if (uuid == NULL) {
        fprintf(stderr, "Error generating UUID\n");
        exit(EXIT_FAILURE);
    }
    if (*num_clients == MAX_CLIENTS) {
    int client_found = 0;
    for (int i = 0; i < *num_clients; i++) {
        if (clients[i].sockfd == 0) {
            // Found an empty slot in the array
            clients[i].sockfd = client_sock;
            clients[i].id = i + 1;
            clients[i].chatbot_active = 0;
            clients[i].chatbot_v2=0;
            strcpy(clients[i].uuid, uuid);
            client_found = 1;
            break;
        }
    }
    if (!client_found) {
        // No empty slot found, send a message to the client indicating server load full
        send(client_sock, "Server load full. Please try again later.", strlen("Server load full. Please try again later."), 0);
        return ;
        // You might want to close the connection or take other actions here
    }
} else {
        clients[*num_clients].sockfd = client_sock;
        clients[*num_clients].id = *num_clients+1;
        //strcpy(clients[*num_clients].name, "Unknown0");
        clients[*num_clients].chatbot_active = 0;
        clients[*num_clients].chatbot_v2 = 0;
        strcpy(clients[*num_clients].uuid, uuid);
        (*num_clients)++;

    }
        

    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, "HI, THIS IS SERVER. HAVE A NICE DAY!!! ");
    strcat(buffer,uuid);
    send(client_sock, buffer, strlen(buffer), 0);
    bzero(log_buffer, BUFFER_SIZE);
    snprintf(log_buffer,sizeof(log_buffer),"Server : %s\n", buffer);
    log_message(log_buffer);
    printf("Server: %s\n", buffer);
}


void trim_whitespace(char *str) {
    char *start = str;
    char *end = str + strlen(str) - 1;

    // Trim leading whitespace
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // Trim trailing whitespace
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }

    // Null-terminate the trimmed string
    *(end + 1) = '\0';

    // If start and end have shifted, move the trimmed string to the beginning
    if (start != str) {
        memmove(str, start, end - start + 2);
    }
}
void handle_delete_request(Client *client) {
    pthread_mutex_lock(&log_mutex);
   FILE *input_file = fopen(LOG_FILE, "r");
    FILE *output_file = fopen("temp_log_file.txt", "w");

    if (input_file == NULL || output_file == NULL) {
        perror("Error opening log files");
        pthread_mutex_unlock(&log_mutex);
        return;

    }

    char buffer[BUFFER_SIZE];
    char *token, *message,*ids, *id1, *id2;
    
    // Read each line from the log file
    while (fgets(buffer, sizeof(buffer), input_file)) {
        // If the line contains a chat message
           token = strtok(buffer, ":");
           trim_whitespace(token);
        if (strncmp(token, "Chatting", strlen("Chatting")) == 0) {
            ids = strtok(NULL, ":");
            message=strtok(NULL,"");
            id1 = strtok(ids, "to");
            id2 = strtok(NULL, "to");
            trim_whitespace(id1);
            trim_whitespace(id2);
            // Check if the IDs match the client's ID
            if (strcmp(id1, client->uuid) == 0 || strcmp(id2, client->uuid) == 0) {
                continue;
            }
        }
        
        fputs(buffer, output_file);
        
        
    }

    // Close files
    fclose(input_file);
    fclose(output_file);

    // Replace the original log file with the temporary one
    if (remove(LOG_FILE) != 0) {
        perror("Error deleting log file");
    } else if (rename("temp_log_file.txt", LOG_FILE) != 0) {
        perror("Error renaming log file");
    }
    else{
        send(client->sockfd,"Sucessfully deleted history",strlen("Sucessfully deleted history"),0);
        printf("Server : Sucessfully deleted history ");
        log_message("Server : Sucessfully deleted history ");
    }
    pthread_mutex_unlock(&log_mutex);

}
void handle_history_delete_request(Client *client, const char *client_id) {
    pthread_mutex_lock(&log_mutex);
   FILE *input_file = fopen(LOG_FILE, "r");
    FILE *output_file = fopen("temp_log_file.txt", "w");

    if (input_file == NULL || output_file == NULL) {
        perror("Error opening log files");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    char buffer[BUFFER_SIZE];
    char *token, *message,*ids, *id1, *id2;
    
    // Read each line from the log file
    while (fgets(buffer, sizeof(buffer), input_file)) {
        // If the line contains a chat message
            token = strtok(buffer, ":");
            trim_whitespace(token);
        if (strncmp(token, "Chatting", strlen("Chatting")) == 0) {
            ids = strtok(NULL, ":");
            message=strtok(NULL,"");
            id1 = strtok(ids, "to");
            id2 = strtok(NULL, "to");
            trim_whitespace(id1);
            trim_whitespace(id2);
            // Check if the IDs match the client's ID
            if (strcmp(id1, client->uuid) == 0 && strcmp(id2, client_id) == 0) {
                // Skip writing this line to the output file (effectively deleting it)
                continue;
            }
        }
        // Write the line to the output file
        fputs(buffer, output_file);
    }

    // Close files
    fclose(input_file);
    fclose(output_file);

    // Replace the original log file with the temporary one
    if (remove(LOG_FILE) != 0) {
        perror("Error deleting log file");
    } else if (rename("temp_log_file.txt", LOG_FILE) != 0) {
        perror("Error renaming log file");
    }
    else{
        send(client->sockfd,"Sucessfully deleted history",strlen("Sucessfully deleted history"),0);
        printf("Server : Sucessfully deleted history\n");
        log_message("Server : Sucessfully deleted history\n");
    }
    pthread_mutex_unlock(&log_mutex);

}
void handle_history_request(Client *client, const char *client_id) {
    pthread_mutex_lock(&log_mutex);
    FILE *log_file = fopen(LOG_FILE, "r");
    if (log_file == NULL) {
        perror("[-]Error opening log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    char buffer[BUFFER_SIZE];
    char history[BUFFER_SIZE * 10]=""; // Assuming maximum size for history
    //memset(history, 0, sizeof(history)); // Clear history buffer
    

    char *token ,* ids ,*message,*id1,*id2;
    while(fgets(buffer, BUFFER_SIZE, log_file) != NULL)
    {

        token=strtok(buffer, ":");
        trim_whitespace(token);
        if(strncmp(token,"Chatting",strlen("Chatting"))==0)
        {
            ids=strtok(NULL,":");
            message=strtok(NULL,"");
            id1=strtok(ids,"to");
            id2=strtok(NULL,"to");
            trim_whitespace(id1);
            trim_whitespace(id2);
            
            char formatted_message[BUFFER_SIZE * 2]; // Twice the size for safety
            if (((strcmp(id1, client->uuid) == 0) && (strcmp(id2, client_id) == 0))||
                ((strcmp(id2, client->uuid) == 0) && (strcmp(id1, client_id) == 0)) ){
                    if(strcmp(id1, client->uuid) == 0 && strcmp(id2, client_id) == 0){
                        snprintf(formatted_message, sizeof(formatted_message), "%s to %s: %s\n", id1,id2, message);
                        strcat(history, formatted_message);
                    }
                    else{
                        snprintf(formatted_message, sizeof(formatted_message), "%s to %s: %s\n", id2,id1, message);
                        strcat(history, formatted_message);
                    }
                
            }

        }

    }
    fclose(log_file);
    // Send history to client
    if (strlen(history) > 0) {
        send(client->sockfd, history, strlen(history), 0);
        printf("Server : %s",history);
        log_message("Server : History data sent\n");
    } else {
        // No history found
        char error_msg[] = "No chat history found for the specified client ID.\n";
        send(client->sockfd, error_msg, strlen(error_msg), 0);
        printf("Server : %s",error_msg);
        log_message("Server : No chat history found for the specified client ID\n");
    }
    pthread_mutex_unlock(&log_mutex);
}


void handle_client_command(Client *clients, Client *client ,char *commands,int *num_clients, fd_set *readfds) {
    
            if (strncmp(commands, "/active", strlen("/active")) == 0  && client->chatbot_active==0 && client->chatbot_v2==0) {
                char log_buffer[BUFFER_SIZE+20];
                printf("Server : Sending active clients list to Client %d\n", client->id);
                snprintf(log_buffer,sizeof(log_buffer),"Server : Sending active clients list to Client %d\n", client->id);
                log_message(log_buffer);
                char active_clients[BUFFER_SIZE];
                sprintf(active_clients, "Active Clients :\n");
                for (int j = 0; j < *num_clients; ++j) {
                    if (clients[j].sockfd != 0) {
                        sprintf(active_clients + strlen(active_clients), "ID: %d, UUID: %s\n", clients[j].id, clients[j].uuid);
                    }
                }
                send(client->sockfd, active_clients, strlen(active_clients), 0);
                } else if (strncmp(commands, "/send", strlen("/send")) == 0  && client->chatbot_active==0 && client->chatbot_v2==0) {
                // Extract destination ID and message
                char *token = strtok(commands, " ");
                char * dest_id = strtok(NULL, " ");
                char *message = strtok(NULL, "");

                    if (dest_id) {
                        int found=0;
                        // Send message to the destination client
                        char send_buffer[BUFFER_SIZE];
                        char log_buffer[BUFFER_SIZE+20];
                        snprintf(log_buffer, sizeof(log_buffer), "Chatting : %s to %s : %s\n", client->uuid, dest_id,message);
                        log_message(log_buffer);
                        snprintf(send_buffer,sizeof(send_buffer),"%s : %s",client->uuid,message);
                        for (int j = 0; j < *num_clients; ++j) {
                        if (strcmp(clients[j].uuid,dest_id)==0 && clients[j].sockfd!=0) {
                            found=1;
                            printf("CLient %s \n",send_buffer);
                            send(clients[j].sockfd, send_buffer, strlen(send_buffer), 0);
                            break;
                        }
                        else{
                            found=0;
                        }
                    }
                    if(found==0) {
                    // Destination client not found or offline
                    char error_msg[] = "Error: Destination client not found or offline\n";
                    char log_buffer[BUFFER_SIZE];
                    printf("CLient %s : %s",client->uuid,error_msg);
                    send(client->sockfd, error_msg, strlen(error_msg), 0);
                    snprintf(log_buffer,sizeof(log_buffer),"Chatting : %s : %s\n",client->uuid,error_msg);
                    log_message(error_msg);
                    }
                } 
             } else if(strncmp(commands,"/chatbot login",strlen("/chatbot login"))==0 && client->chatbot_active==0 && client->chatbot_v2 == 0){

                printf("Server : Chatbot activated for Client %d\n", client->id);
                client->chatbot_active = 1;
                send(client->sockfd, "stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions\n", strlen("stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions\n"), 0);
                log_message("stupidbot : Hi, I am stupid bot, I am able to answer a limited set of your questions\n");
                            
            } else if (strncmp(commands, "/chatbot logout", strlen("/chatbot logout")) == 0  && client->chatbot_active==1 && client->chatbot_v2==0 ) {
                printf("Server : Chatbot deactivated for Client %d\n", client->id);
                client->chatbot_active = 0;
                send(client->sockfd, "stupidbot> Bye! Have a nice day and do not complain about me\n", strlen("stupidbot> Bye! Have a nice day and do not complain about me\n"), 0); 
                log_message("stupidbot : Bye! Have a nice day and do not complain about me\n");
            } else if (strncmp(commands, "/chatbot_v2 login", strlen("/chatbot_v2 login")) == 0 && client->chatbot_active==0 && client->chatbot_v2==0){  
               printf("Hi, I am updated bot, I am able to answer any question be it correct or incorrect");
               client->chatbot_v2=1;
               send(client->sockfd, "gpt2bot>Hi, I am updated bot, I am able to answer any question be it correct or incorrect\n",strlen("gpt2bot>Hi, I am updated bot, I am able to answer any question be it correct or incorrect\n"), 0);
               log_message("gpt2bot>Hi, I am updated bot, I am able to answer any question be it correct or incorrect\n");
             } else if (strncmp(commands, "/chatbot_v2 logout", strlen("/chatbot_v2 logout")) == 0 && client->chatbot_active==0 && client->chatbot_v2==1) {    
                printf("Server : Client %d requested logout\n", client->id);
                client->chatbot_v2=0;
                send(client->sockfd, "gpt2bot : Bye! Have a nice day and hope you do not have any complaints about me", strlen("Bye! Have a nice day and hope you do not have any complaints about me"), 0);
                log_message("gpt2bot : Bye! Have a nice day and hope you do not have any complaints about me\n");
            }else if (strncmp(commands, "/logout", strlen("/logout")) == 0 && client->chatbot_active==0 && client->chatbot_v2==0) {    
                printf("Server : Client %d requested logout\n", client->id);
                send(client->sockfd, "Bye!! Have a nice day", strlen("Bye!! Have a nice day"), 0);
                log_message("Server : Bye!! Have a nice day\n");
                close(client->sockfd);
                FD_CLR(client->sockfd, readfds);
                client->sockfd = 0;
            }else if(strncmp(commands, "/history_delete", strlen("/history_delete")) == 0 && client->chatbot_active==0 && client->chatbot_v2==0 ){
                    char * recipient_id = strtok(commands + strlen("/history_delete") + 1, "");
                    //printf(" R =%s",recipient_id);
                    if (recipient_id != NULL) {
                        handle_history_delete_request(client, recipient_id);
                    } else {
                        char error_msg[] = "Invalid request format. Usage: /history_delete <client_id>\n";
                        send(client->sockfd, error_msg, strlen(error_msg), 0);
                        log_message("Invalid request format. Usage: /history_delete <client_id>\n");
                    }
                } else if (strncmp(commands, "/history", strlen("/history")) == 0 && client->chatbot_active==0 && client->chatbot_v2==0) {
                char * recipient_id = strtok(commands + strlen("/history") + 1, "");
                //printf(" R =%s",recipient_id);
                if (recipient_id != NULL) {
                    handle_history_request(client, recipient_id);
                } else {
                    char error_msg[] = "Invalid request format. Usage: /history <client_id>\n";
                    send(client->sockfd, error_msg, strlen(error_msg), 0);
                    log_message("Invalid request format. Usage: /history <client_id>\n");
                }
                }else if(strncmp(commands, "/delete_all", strlen("/delete_all")) == 0 && client->chatbot_active==0 && client->chatbot_v2==0){
                    //printf(" R =%s",recipient_id);
                        handle_delete_request(client);
                        
                }else {
                if (client->chatbot_active == 1) {
                    // Check if the command matches any FAQ
                    int found = 0;

                    printf("user : %s\n",commands);
                    
                    //printf("%s",faq[0].question);
                    for (int i = 0; i < 231; i++) {
                        char lowercase_question[100];
                        strcpy(lowercase_question, faq[i].question);
                        for (int j = 0; lowercase_question[j] != '\0'; j++) {
                            lowercase_question[j] = tolower(lowercase_question[j]);
                        }
                        remove_trailing_spaces(lowercase_question);

                        // Convert commands to lowercase and remove trailing spaces
                        char lowercase_commands[100];
                        strcpy(lowercase_commands, commands);
                        for (int j = 0; lowercase_commands[j] != '\0'; j++) {
                            lowercase_commands[j] = tolower(lowercase_commands[j]);
                        }
                        remove_trailing_spaces(lowercase_commands);
                        
                        if (strcmp(lowercase_commands, lowercase_question) == 0) {
                            found = 1;
                            printf("Server : %s\n",faq[i].answer);
                            send(client->sockfd, faq[i].answer, strlen(faq[i].answer), 0);
                            // Update chat history
                            //update_chat_history(client, command);
                            break;
                        }
                    }
                    if (found == 0) {
                        // No matching FAQ found
                        printf("Server : %s","system Malfunction");
                        send(client->sockfd, "stupidbot> System Malfunction, I couldn't understand your query.\n", strlen("stupidbot> System Malfunction, I couldn't understand your query.\n"), 0);
                        log_message("stupidbot : System Malfunction, I couldn't understand your query.\n");
                        
                        // Update chat history
                        //update_chat_history(client, command);
                    }
                } else if(client->chatbot_v2==1){
                 	int pipe_fd[2];
   
					// Create a pipe for communication
					if (pipe(pipe_fd) == -1) {
						perror("Pipe creation failed");
						exit(EXIT_FAILURE);
					}

					pid_t pid = fork();

					if (pid == -1) {
						perror("Fork failed");
						exit(EXIT_FAILURE);
					} else if (pid == 0) {
						// Child process (Python script execution)
						close(pipe_fd[READ_END]);  // Close the unused read end of the pipe
						dup2(pipe_fd[WRITE_END], STDOUT_FILENO);  // Redirect stdout to the pipe write end
						close(pipe_fd[WRITE_END]);  // Close the original write end of the pipe
						
						execlp("python3", "python3", "./socket/gpt_2_gen.py",commands,NULL);  // Execute the Python script
						perror("execlp failed");
						exit(EXIT_FAILURE);
					} else {
						// Parent process (C program)
						close(pipe_fd[WRITE_END]);  // Close the unused write end of the pipe
						char buffer[4096];
						ssize_t bytes_read = read(pipe_fd[READ_END], buffer, sizeof(buffer));
						if (bytes_read == -1) {
							perror("Read from pipe failed");
							exit(EXIT_FAILURE);
						}
						buffer[bytes_read] = '\0';  // Null-terminate the string
						//printf("Output from Python script: %s\n", buffer);
						send(client->sockfd,buffer,strlen(buffer),0);
                        log_message(buffer);
						close(pipe_fd[READ_END]);  // Close the read end of the pipe
						wait(NULL);  // Wait for the child process to complete
						
					}
                 }else{
            
                    // Chatbot is inactive, handle as a regular message
                    printf("Server : Unknown command received from Client %d\n", client->id);
                    send(client->sockfd,"Unknown command received from Client\n",strlen("Unknown command received from Client"),0);
                    log_message("Unknown command received from Client\n");
                    //update_chat_history(client, command);
                }
            }
            
    }

int main() {
    char *ip = "127.0.0.1";
    int port = 5566;
    Client clients[MAX_CLIENTS];
    int num_clients = 0;
    char buffer[BUFFER_SIZE];
    char log_buffer[BUFFER_SIZE+20];
    initialize_faq();

    int server_sock, client_sock;
    struct sockaddr_in server_addr,client_addr;
    socklen_t addr_size;
    fd_set readfds;
    int max_sd;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");
    
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Bind to the port number: %d\n", port);
    if (listen(server_sock, 10) < 0) {
        perror("[-]Listen error");
        exit(EXIT_FAILURE);
    }
    printf("Listening...\n");

    FD_ZERO(&readfds);
    FD_SET(server_sock, &readfds);
    max_sd = server_sock;

    while(1) {
        fd_set temp_fds = readfds; 
        int activity = select(max_sd + 1, &temp_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("[-]Select error");
            if (errno != EINTR) {
                fprintf(stderr, "Error : Select returned with errno %d\n", errno);
                exit(EXIT_FAILURE);
            } else {
                fprintf(stderr, "Warning : Select interrupted\n");
            }
        }
        
        if (FD_ISSET(server_sock, &temp_fds)) {
        handle_new_connection(server_sock, clients, &num_clients, &readfds, &max_sd);
        continue; // Skip to the next iteration to avoid unnecessary checks
        }
        
            for (int i = 0; i < num_clients; ++i) {
                        if (FD_ISSET(clients[i].sockfd, &temp_fds)) {
                            // Clear the buffer before reading
                            bzero(buffer,BUFFER_SIZE);
                            ssize_t bytes_received = recv(clients[i].sockfd, buffer, BUFFER_SIZE, 0);
                            if (bytes_received <= 0) {
                                printf("[+]Client %d disconnected.\n", clients[i].id);
                                bzero(log_buffer,BUFFER_SIZE);
                                snprintf(log_buffer,sizeof(log_buffer),"[+]Client %d disconnected\n", clients[i].id);
                                log_message(log_buffer);
                                close(clients[i].sockfd);
                                FD_CLR(clients[i].sockfd, &readfds);
                                // Remove the client from the array
                                clients[i].sockfd = 0;
                                clients[i].id=0;
                                clients[i].chatbot_active=0;
                                clients[i].chatbot_v2=0;
                                strcpy(clients[i].uuid,"NULL");
                                num_clients--;
                            } else {
                                // Handle client command
                                handle_client_command(clients, &clients[i], buffer, &num_clients, &readfds);
                            }
                        }
                    }
        
        
    }
    return 0;
 }
