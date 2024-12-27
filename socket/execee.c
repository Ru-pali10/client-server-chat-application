#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>
#include <sys/wait.h>

#define READ_END 0
#define WRITE_END 1

int main() {
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
        execlp("python3", "python3", "sample.py","world",NULL);  // Execute the Python script
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
        printf("Output from Python script: %s\n", buffer);
        close(pipe_fd[READ_END]);  // Close the read end of the pipe
        wait(NULL);  // Wait for the child process to complete
    }

    return 0;
}
