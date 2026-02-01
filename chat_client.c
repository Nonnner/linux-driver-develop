/*
 * chat_client.c - TCP Chat Client
 * Connects to chat server, handles authentication and message exchange
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define SERVER_PORT 8888

/* Function prototypes */
void *receive_messages(void *arg);
void send_message(int socket);

/* Thread for receiving messages */
void *receive_messages(void *arg)
{
    int socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    int received;
    
    while ((received = recv(socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }
    
    if (received == 0) {
        printf("\nServer disconnected\n");
    } else {
        perror("\nReceive error");
    }
    
    exit(0);
    return NULL;
}

/* Main client function */
int main(int argc, char *argv[])
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t recv_thread;
    char server_ip[16] = "127.0.0.1";
    
    /* Parse command line arguments */
    if (argc > 1) {
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
    }
    
    /* Create socket */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }
    
    /* Setup server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client_socket);
        return EXIT_FAILURE;
    }
    
    /* Connect to server */
    printf("Connecting to server at %s:%d...\n", server_ip, SERVER_PORT);
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        return EXIT_FAILURE;
    }
    
    printf("Connected to chat server\n");
    printf("================================================\n");
    
    /* Authentication phase - receive username prompt */
    int received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
        fflush(stdout);
        
        /* Send username */
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }
    
    /* Receive password prompt */
    received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
        fflush(stdout);
        
        /* Send password */
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }
    
    /* Receive authentication result */
    received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
        
        if (strstr(buffer, "AUTH_FAILED") != NULL) {
            printf("Authentication failed. Disconnecting...\n");
            close(client_socket);
            return EXIT_FAILURE;
        } else if (strstr(buffer, "AUTH_SUCCESS") != NULL) {
            printf("Authentication successful!\n");
            printf("================================================\n");
            printf("Chat Commands:\n");
            printf("  /list              - Show online users\n");
            printf("  /msg <user> <msg>  - Send private message\n");
            printf("  /help              - Show help\n");
            printf("  <message>          - Broadcast to all\n");
            printf("  quit or exit       - Disconnect\n");
            printf("================================================\n");
            printf("You can start chatting now. Type /help for commands.\n");
        }
    }
    
    /* Create thread for receiving messages */
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)&client_socket) != 0) {
        perror("Thread creation failed");
        close(client_socket);
        return EXIT_FAILURE;
    }
    
    /* Main loop for sending messages */
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            /* Remove trailing newline */
            buffer[strcspn(buffer, "\n")] = 0;
            
            /* Check for quit command */
            if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                printf("Disconnecting...\n");
                break;
            }
            
            /* Send message to server */
            size_t len = strlen(buffer);
            if (len > 0 && len < BUFFER_SIZE - 1) {
                /* Safely append newline */
                buffer[len] = '\n';
                buffer[len + 1] = '\0';
                if (send(client_socket, buffer, len + 1, 0) < 0) {
                    perror("Send failed");
                    break;
                }
            }
        }
    }
    
    close(client_socket);
    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);
    
    return EXIT_SUCCESS;
}
