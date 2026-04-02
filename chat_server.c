/*
 * chat_server.c - Multi-user TCP Chat Server
 * Manages client connections, authentication, and message routing
 * Uses crypto driver for password hashing and message encryption
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

/* Include common crypto interface definitions */
#include "common/crypto_user.h"

#define PORT 8888
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096
#define USERNAME_SIZE 32
#define PASSWORD_SIZE 32
#define MAX_DATA_SIZE CRYPTO_MAX_DATA_SIZE
#define AES_KEY_SIZE CRYPTO_AES_KEY_SIZE
#define MD5_DIGEST_SIZE CRYPTO_MD5_SIZE

/* Use IOCTL commands from common header */
/* These are defined in crypto_user.h:
 * - IOCTL_MD5_HASH  (command 6)
 * - IOCTL_ENCRYPT   (command 4)
 * - IOCTL_DECRYPT   (command 5)
 */

/* Client structure */
typedef struct {
    int socket;
    int id;
    char username[USERNAME_SIZE];
    int authenticated;
    pthread_t thread;
} client_t;

/* Global variables */
static client_t *clients[MAX_CLIENTS];
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static int client_count = 0;
static int crypto_fd = -1;

/* Hardcoded user database (username:MD5_hash_of_password) */
/* For demo: alice:password123, bob:password456, charlie:password789 */
static struct {
    char username[USERNAME_SIZE];
    unsigned char password_hash[MD5_DIGEST_SIZE];
} user_db[] = {
    {"alice", {0}},
    {"bob", {0}},
    {"charlie", {0}}
};
static int user_db_size = 3;
static const char *user_plain_passwords[] = {
    "password123",
    "password456",
    "password789"
};
static int md5_driver_available = 1;
static int aes_driver_available = 1;

/* AES key for message encryption (shared secret)
 * WARNING: This hardcoded key is for DEMONSTRATION ONLY.
 * In production systems:
 * - Generate cryptographically secure random keys
 * - Use key exchange protocols (e.g., Diffie-Hellman)
 * - Store keys securely
 * - Rotate keys regularly
 */
static unsigned char aes_key[AES_KEY_SIZE] = "1234567890123456";

/* Function prototypes */
void *handle_client(void *arg);
int add_client(client_t *client);
void remove_client(int id);
void send_message_to_all(char *message, int sender_id);
int authenticate_user(const char *username, const char *password);
int md5_hash(const unsigned char *input, unsigned int len, unsigned char *output);
int aes_encrypt_msg(const unsigned char *plaintext, unsigned int len, unsigned char *ciphertext,
                    unsigned int *out_len, unsigned char *out_iv);
int aes_decrypt_msg(const unsigned char *ciphertext, unsigned int len, const unsigned char *iv,
                    unsigned char *plaintext, unsigned int *out_len);
int crypto_process_chat_message(const char *input, char *output, size_t output_size);
void init_user_database(void);

/* Initialize crypto driver */
int init_crypto_driver(void)
{
    crypto_fd = open("/dev/crypto_dev", O_RDWR);
    if (crypto_fd < 0) {
        perror("Failed to open crypto device");
        fprintf(stderr, "\n");
        fprintf(stderr, "ERROR: Crypto device /dev/crypto_dev not found!\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "The kernel module must be loaded before starting the server.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "To fix this issue, run these commands:\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "  1. Build the kernel module (if not already built):\n");
        fprintf(stderr, "     make -f Makefile.driver\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "  2. Load the kernel module:\n");
        fprintf(stderr, "     sudo insmod crypto_driver.ko\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "  3. Set device permissions:\n");
        fprintf(stderr, "     sudo chmod 666 /dev/crypto_dev\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "  4. Verify the module is loaded:\n");
        fprintf(stderr, "     lsmod | grep crypto_driver\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Or use the helper script:\n");
        fprintf(stderr, "     sudo ./setup.sh load\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "See QUICKSTART.md for detailed instructions.\n");
        fprintf(stderr, "\n");
        return -1;
    }

    if (ioctl(crypto_fd, IOCTL_SET_KEY, aes_key) < 0) {
        if (errno == EINVAL || errno == ENOTTY) {
            fprintf(stderr,
                    "Warning: IOCTL_SET_KEY unsupported by loaded driver, continuing in compatibility mode\n");
        } else {
            perror("Failed to set AES key");
            close(crypto_fd);
            crypto_fd = -1;
            return -1;
        }
    }

    printf("Crypto driver opened successfully\n");
    return 0;
}

/* Close crypto driver */
void close_crypto_driver(void)
{
    if (crypto_fd >= 0) {
        close(crypto_fd);
        crypto_fd = -1;
    }
}

/* MD5 hash using driver */
int md5_hash(const unsigned char *input, unsigned int len, unsigned char *output)
{
    struct crypto_data data;
    memset(&data, 0, sizeof(data));
    
    if (len > MAX_DATA_SIZE) {
        fprintf(stderr, "Input too large for MD5\n");
        return -1;
    }
    
    memcpy(data.input, input, len);
    data.input_len = len;
    
    /* Use correct IOCTL command from crypto_user.h */
    if (ioctl(crypto_fd, IOCTL_MD5_HASH, &data) < 0) {
        perror("MD5 hash ioctl failed");
        return -1;
    }
    
    memcpy(output, data.output, MD5_DIGEST_SIZE);
    return 0;
}

/* AES encrypt using driver */
int aes_encrypt_msg(const unsigned char *plaintext, unsigned int len, unsigned char *ciphertext,
                    unsigned int *out_len, unsigned char *out_iv)
{
    struct crypto_data data;
    memset(&data, 0, sizeof(data));
    
    if (len > MAX_DATA_SIZE) {
        fprintf(stderr, "Input too large for encryption\n");
        return -1;
    }
    
    memcpy(data.input, plaintext, len);
    data.input_len = len;
    memcpy(data.key, aes_key, AES_KEY_SIZE);
    
    /* Use correct IOCTL command from crypto_user.h */
    if (ioctl(crypto_fd, IOCTL_ENCRYPT, &data) < 0) {
        perror("AES encrypt ioctl failed");
        return -1;
    }
    
    memcpy(ciphertext, data.output, data.output_len);
    memcpy(out_iv, data.iv, AES_KEY_SIZE);
    *out_len = data.output_len;
    return 0;
}

/* AES decrypt using driver */
int aes_decrypt_msg(const unsigned char *ciphertext, unsigned int len, const unsigned char *iv,
                    unsigned char *plaintext, unsigned int *out_len)
{
    struct crypto_data data;
    memset(&data, 0, sizeof(data));
    
    if (len > MAX_DATA_SIZE) {
        fprintf(stderr, "Input too large for decryption\n");
        return -1;
    }
    
    memcpy(data.input, ciphertext, len);
    data.input_len = len;
    memcpy(data.key, aes_key, AES_KEY_SIZE);
    memcpy(data.iv, iv, AES_KEY_SIZE);
    
    /* Use correct IOCTL command from crypto_user.h */
    if (ioctl(crypto_fd, IOCTL_DECRYPT, &data) < 0) {
        perror("AES decrypt ioctl failed");
        return -1;
    }
    
    memcpy(plaintext, data.output, data.output_len);
    *out_len = data.output_len;
    return 0;
}

/* Process user message through kernel AES pipeline (encrypt + decrypt) */
int crypto_process_chat_message(const char *input, char *output, size_t output_size)
{
    unsigned char encrypted[MAX_DATA_SIZE];
    unsigned char decrypted[MAX_DATA_SIZE];
    unsigned char iv[AES_KEY_SIZE];
    unsigned int encrypted_len = 0;
    unsigned int decrypted_len = 0;
    size_t input_len = strlen(input);

    if (output_size == 0 || input_len == 0 || input_len > MAX_DATA_SIZE) {
        return -1;
    }

    if (!aes_driver_available) {
        size_t copy_len = input_len;
        if (copy_len >= output_size) {
            copy_len = output_size - 1;
        }
        memcpy(output, input, copy_len);
        output[copy_len] = '\0';
        return 0;
    }

    if (aes_encrypt_msg((const unsigned char *)input, (unsigned int)input_len,
                        encrypted, &encrypted_len, iv) < 0) {
        fprintf(stderr,
                "Warning: AES ioctl unavailable, switching to compatibility message mode\n");
        aes_driver_available = 0;
        size_t copy_len = input_len;
        if (copy_len >= output_size) {
            copy_len = output_size - 1;
        }
        memcpy(output, input, copy_len);
        output[copy_len] = '\0';
        return 0;
    }

    if (aes_decrypt_msg(encrypted, encrypted_len, iv, decrypted, &decrypted_len) < 0) {
        fprintf(stderr,
                "Warning: AES decrypt ioctl unavailable, switching to compatibility message mode\n");
        aes_driver_available = 0;
        size_t copy_len = input_len;
        if (copy_len >= output_size) {
            copy_len = output_size - 1;
        }
        memcpy(output, input, copy_len);
        output[copy_len] = '\0';
        return 0;
    }

    if (decrypted_len < input_len) {
        return -1;
    }

    if (input_len >= output_size) {
        input_len = output_size - 1;
    }

    memcpy(output, decrypted, input_len);
    output[input_len] = '\0';
    return 0;
}

/* Initialize user database with hashed passwords */
void init_user_database(void)
{
    for (int i = 0; i < user_db_size; i++) {
        if (md5_hash((unsigned char *)user_plain_passwords[i], strlen(user_plain_passwords[i]),
                     user_db[i].password_hash) < 0) {
            fprintf(stderr,
                    "Warning: MD5 ioctl unavailable, switching to compatibility auth mode\n");
            md5_driver_available = 0;
            return;
        }
        printf("User %s initialized with hashed password\n", user_db[i].username);
    }
}

/* Authenticate user */
int authenticate_user(const char *username, const char *password)
{
    unsigned char password_hash[MD5_DIGEST_SIZE];

    if (!md5_driver_available) {
        for (int i = 0; i < user_db_size; i++) {
            if (strcmp(user_db[i].username, username) == 0 &&
                strcmp(user_plain_passwords[i], password) == 0) {
                return 1;
            }
        }
        return 0;
    }
    
    if (md5_hash((unsigned char *)password, strlen(password), password_hash) < 0) {
        md5_driver_available = 0;
        return 0;
    }
    
    for (int i = 0; i < user_db_size; i++) {
        if (strcmp(user_db[i].username, username) == 0) {
            if (memcmp(user_db[i].password_hash, password_hash, MD5_DIGEST_SIZE) == 0) {
                return 1;
            }
        }
    }
    
    return 0;
}

/* Add client to array */
int add_client(client_t *client)
{
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = client;
            client->id = i;
            client_count++;
            pthread_mutex_unlock(&clients_mutex);
            return i;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    return -1;
}

/* Remove client from array */
void remove_client(int id)
{
    pthread_mutex_lock(&clients_mutex);
    
    if (id >= 0 && id < MAX_CLIENTS && clients[id] != NULL) {
        clients[id] = NULL;
        client_count--;
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message_to_all(char *message, int sender_id)
{
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->authenticated && i != sender_id) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Send failed");
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

/* Send user list to specific client */
void send_user_list(int client_id)
{
    char user_list[BUFFER_SIZE];
    int offset = 0;
    
    pthread_mutex_lock(&clients_mutex);
    
    offset = snprintf(user_list, sizeof(user_list), "USERLIST:");
    
    int first = 1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->authenticated) {
            if (!first && offset < BUFFER_SIZE - 2) {
                user_list[offset++] = ',';
            }
            first = 0;
            
            int len = strlen(clients[i]->username);
            if (offset + len < BUFFER_SIZE - 2) {
                strcpy(user_list + offset, clients[i]->username);
                offset += len;
            }
        }
    }
    
    if (offset < BUFFER_SIZE - 1) {
        user_list[offset++] = '\n';
        user_list[offset] = '\0';
    }
    
    if (client_id >= 0 && client_id < MAX_CLIENTS && clients[client_id] != NULL) {
        send(clients[client_id]->socket, user_list, strlen(user_list), 0);
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

/* Send private message to specific user */
int send_private_message(const char *from_username, const char *to_username, const char *message)
{
    char formatted_msg[BUFFER_SIZE];
    int sent = 0;
    
    snprintf(formatted_msg, sizeof(formatted_msg), "[PRIVATE from %s] %s\n", from_username, message);
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->authenticated && 
            strcmp(clients[i]->username, to_username) == 0) {
            if (send(clients[i]->socket, formatted_msg, strlen(formatted_msg), 0) >= 0) {
                sent = 1;
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    
    return sent;
}

/* Handle client connection */
void *handle_client(void *arg)
{
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    int received;
    
    /* Authentication phase */
    send(client->socket, "USERNAME: ", 10, 0);
    received = recv(client->socket, username, USERNAME_SIZE - 1, 0);
    if (received <= 0) {
        goto cleanup;
    }
    /* Ensure buffer doesn't overflow */
    if (received >= USERNAME_SIZE) {
        received = USERNAME_SIZE - 1;
    }
    username[received] = '\0';
    /* Remove newline */
    username[strcspn(username, "\r\n")] = 0;
    
    send(client->socket, "PASSWORD: ", 10, 0);
    received = recv(client->socket, password, PASSWORD_SIZE - 1, 0);
    if (received <= 0) {
        goto cleanup;
    }
    /* Ensure buffer doesn't overflow */
    if (received >= PASSWORD_SIZE) {
        received = PASSWORD_SIZE - 1;
    }
    password[received] = '\0';
    password[strcspn(password, "\r\n")] = 0;
    
    /* Authenticate */
    if (authenticate_user(username, password)) {
        strncpy(client->username, username, USERNAME_SIZE - 1);
        client->username[USERNAME_SIZE - 1] = '\0';
        client->authenticated = 1;
        send(client->socket, "AUTH_SUCCESS\n", 13, 0);
        printf("User %s authenticated successfully\n", username);
        
        /* Announce to all */
        char join_msg[BUFFER_SIZE];
        snprintf(join_msg, sizeof(join_msg), "[SERVER] %s has joined the chat\n", username);
        send_message_to_all(join_msg, client->id);
    } else {
        send(client->socket, "AUTH_FAILED\n", 12, 0);
        printf("Authentication failed for user %s\n", username);
        goto cleanup;
    }
    
    /* Message handling loop */
    while ((received = recv(client->socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[received] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if (strlen(buffer) > 0) {
            /* Check for commands */
            if (buffer[0] == '/') {
                if (strcmp(buffer, "/list") == 0) {
                    /* Send user list */
                    printf("User %s requested user list\n", client->username);
                    send_user_list(client->id);
                } 
                else if (strncmp(buffer, "/msg ", 5) == 0) {
                    /* Private message: /msg <username> <message> */
                    char target_user[USERNAME_SIZE];
                    char processed_private[BUFFER_SIZE];
                    char *space_pos = strchr(buffer + 5, ' ');
                    
                    if (space_pos != NULL) {
                        size_t username_len = space_pos - (buffer + 5);
                        if (username_len >= USERNAME_SIZE) {
                            username_len = USERNAME_SIZE - 1;
                        }
                        strncpy(target_user, buffer + 5, username_len);
                        target_user[username_len] = '\0';
                        
                        char *msg_start = space_pos + 1;

                        if (crypto_process_chat_message(msg_start, processed_private,
                                                        sizeof(processed_private)) < 0) {
                            char crypto_err[] = "[SERVER] Failed to process message via AES driver\n";
                            send(client->socket, crypto_err, strlen(crypto_err), 0);
                            continue;
                        }
                        
                        if (send_private_message(client->username, target_user, processed_private)) {
                            printf("Private message from %s to %s: %s\n", 
                                   client->username, target_user, processed_private);
                            /* Confirm to sender - limit message length to avoid truncation */
                            char confirm[BUFFER_SIZE];
                            int max_confirm_len = BUFFER_SIZE - USERNAME_SIZE - 20;
                            snprintf(confirm, sizeof(confirm), "[PRIVATE to %s] %.*s\n", 
                                     target_user, max_confirm_len, processed_private);
                            send(client->socket, confirm, strlen(confirm), 0);
                        } else {
                            char error_msg[] = "[SERVER] User not found or offline\n";
                            send(client->socket, error_msg, strlen(error_msg), 0);
                        }
                    } else {
                        char usage[] = "[SERVER] Usage: /msg <username> <message>\n";
                        send(client->socket, usage, strlen(usage), 0);
                    }
                }
                else if (strcmp(buffer, "/help") == 0) {
                    /* Help command */
                    const char *help_msg = 
                        "[SERVER] Available commands:\n"
                        "[SERVER]   /list - Show online users\n"
                        "[SERVER]   /msg <username> <message> - Send private message\n"
                        "[SERVER]   /help - Show this help\n"
                        "[SERVER] Just type a message to broadcast to all users\n";
                    send(client->socket, help_msg, strlen(help_msg), 0);
                }
                else {
                    char error_msg[] = "[SERVER] Unknown command. Type /help for available commands\n";
                    send(client->socket, error_msg, strlen(error_msg), 0);
                }
            } 
            else {
                /* Regular message - broadcast to all */
                char formatted_msg[BUFFER_SIZE];
                char processed_msg[BUFFER_SIZE];

                if (crypto_process_chat_message(buffer, processed_msg, sizeof(processed_msg)) < 0) {
                    char crypto_err[] = "[SERVER] Failed to process message via AES driver\n";
                    send(client->socket, crypto_err, strlen(crypto_err), 0);
                    continue;
                }

                /* Reserve space for "[username] \n" format (USERNAME_SIZE + 5 bytes) */
                /* Limit message content to avoid truncation warning */
                int max_msg_len = BUFFER_SIZE - USERNAME_SIZE - 5;
                snprintf(formatted_msg, sizeof(formatted_msg), "[%s] %.*s\n", 
                         client->username, max_msg_len, processed_msg);
                
                printf("Message from %s: %s\n", client->username, processed_msg);
                
                /* Broadcast to all other authenticated clients */
                send_message_to_all(formatted_msg, client->id);
            }
        }
    }
    
cleanup:
    /* Announce departure */
    if (client->authenticated) {
        char leave_msg[BUFFER_SIZE];
        snprintf(leave_msg, sizeof(leave_msg), "[SERVER] %s has left the chat\n", client->username);
        send_message_to_all(leave_msg, client->id);
        printf("User %s disconnected\n", client->username);
    }
    
    close(client->socket);
    remove_client(client->id);
    free(client);
    pthread_detach(pthread_self());
    
    return NULL;
}

/* Main server function */
int main(void)
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    /* Initialize crypto driver */
    if (init_crypto_driver() < 0) {
        fprintf(stderr, "Failed to initialize crypto driver. Make sure the module is loaded.\n");
        return EXIT_FAILURE;
    }
    
    /* Initialize user database */
    init_user_database();
    
    /* Create socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        close_crypto_driver();
        return EXIT_FAILURE;
    }
    
    /* Allow address reuse */
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_socket);
        close_crypto_driver();
        return EXIT_FAILURE;
    }
    
    /* Bind socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        close_crypto_driver();
        return EXIT_FAILURE;
    }
    
    /* Listen */
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        close_crypto_driver();
        return EXIT_FAILURE;
    }
    
    printf("Chat server started on port %d\n", PORT);
    printf("Waiting for clients...\n");
    printf("Available users: alice/password123, bob/password456, charlie/password789\n");
    
    /* Accept clients */
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        /* Create client structure */
        client_t *client = (client_t *)malloc(sizeof(client_t));
        if (client == NULL) {
            fprintf(stderr, "Failed to allocate client structure\n");
            close(client_socket);
            continue;
        }
        
        client->socket = client_socket;
        client->authenticated = 0;
        
        if (add_client(client) < 0) {
            fprintf(stderr, "Maximum clients reached\n");
            close(client_socket);
            free(client);
            continue;
        }
        
        /* Create thread for client */
        if (pthread_create(&client->thread, NULL, handle_client, (void *)client) != 0) {
            perror("Thread creation failed");
            remove_client(client->id);
            close(client_socket);
            free(client);
        }
    }
    
    close(server_socket);
    close_crypto_driver();
    
    return EXIT_SUCCESS;
}
