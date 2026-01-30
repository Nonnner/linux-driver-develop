/*
 * chat_server.c - Multi-user TCP Chat Server
 * 
 * This server manages multiple client connections, authenticates users,
 * and routes messages between users. It uses the crypto device driver
 * for password hashing (MD5) and message encryption/decryption (AES).
 * 
 * Architecture:
 * - Main thread handles accepting new connections
 * - Each client connection is handled by a separate thread
 * - Crypto operations are delegated to the kernel driver via ioctl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>

#include "../common/protocol.h"
#include "../common/crypto_user.h"

/* Server configuration */
#define MAX_PENDING_CONNECTIONS 10

/* User database (in-memory for demonstration) */
typedef struct {
    char username[MAX_USERNAME_LEN];
    unsigned char password_hash[MD5_HASH_SIZE];
    int is_registered;
} user_account_t;

/* Connected client information */
typedef struct {
    int socket_fd;
    int crypto_fd;                              /* File descriptor for crypto device */
    char username[MAX_USERNAME_LEN];
    int is_authenticated;
    unsigned char session_key[AES_KEY_SIZE];
    unsigned char current_iv[AES_BLOCK_SIZE];
    pthread_t thread_id;
    int active;
} client_info_t;

/* Global variables */
static client_info_t clients[MAX_CLIENTS];
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static user_account_t user_db[MAX_CLIENTS];
static int user_count = 0;
static pthread_mutex_t user_db_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t server_running = 1;
static int server_socket = -1;

/* Function prototypes */
static void *client_handler(void *arg);
static int authenticate_user(client_info_t *client, const char *username,
                             const unsigned char *password_hash);
static int send_message(int socket_fd, chat_message_t *msg);
static int receive_message(int socket_fd, chat_message_t *msg);
static void broadcast_message(client_info_t *sender, const char *message, size_t len);
static void send_private_message(client_info_t *sender, const char *to_user,
                                 const char *message, size_t len);
static int encrypt_message(client_info_t *client, const unsigned char *plaintext,
                           size_t plain_len, unsigned char *ciphertext, size_t *cipher_len);
static int decrypt_message(client_info_t *client, const unsigned char *ciphertext,
                           size_t cipher_len, unsigned char *plaintext, size_t *plain_len);
static int hash_password(int crypto_fd, const char *password,
                         unsigned char *hash_out);
static void init_user_database(void);
static void cleanup_client(client_info_t *client);
static void signal_handler(int sig);
static void print_hex(const char *label, const unsigned char *data, size_t len);

/*
 * Main function
 */
int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = DEFAULT_SERVER_PORT;
    int opt = 1;
    int i;

    /* Parse command line arguments */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number\n");
            return 1;
        }
    }

    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize client array */
    memset(clients, 0, sizeof(clients));
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd = -1;
        clients[i].crypto_fd = -1;
        clients[i].active = 0;
    }

    /* Initialize user database with some test users */
    init_user_database();

    /* Create server socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    /* Set socket options */
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_socket);
        return 1;
    }

    /* Bind socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }

    /* Listen for connections */
    if (listen(server_socket, MAX_PENDING_CONNECTIONS) < 0) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }

    printf("Chat server started on port %d\n", port);
    printf("Waiting for connections...\n");

    /* Main loop - accept connections */
    while (server_running) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            if (errno == EINTR && !server_running)
                break;
            perror("Accept failed");
            continue;
        }

        printf("New connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /* Find empty slot for client */
        pthread_mutex_lock(&clients_mutex);
        int slot = -1;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                slot = i;
                break;
            }
        }

        if (slot == -1) {
            pthread_mutex_unlock(&clients_mutex);
            printf("Server full, rejecting connection\n");
            close(client_socket);
            continue;
        }

        /* Initialize client slot */
        clients[slot].socket_fd = client_socket;
        clients[slot].is_authenticated = 0;
        clients[slot].active = 1;
        memset(clients[slot].username, 0, MAX_USERNAME_LEN);

        /* Open crypto device for this client */
        clients[slot].crypto_fd = open(CRYPTO_DEVICE_PATH, O_RDWR);
        if (clients[slot].crypto_fd < 0) {
            fprintf(stderr, "Warning: Could not open crypto device: %s\n", strerror(errno));
            fprintf(stderr, "Server will run without encryption (for testing only)\n");
        }

        /* Create client handler thread */
        if (pthread_create(&clients[slot].thread_id, NULL, client_handler, &clients[slot]) != 0) {
            perror("Failed to create client thread");
            close(client_socket);
            if (clients[slot].crypto_fd >= 0)
                close(clients[slot].crypto_fd);
            clients[slot].active = 0;
        }

        pthread_mutex_unlock(&clients_mutex);
    }

    /* Cleanup */
    printf("\nShutting down server...\n");
    close(server_socket);

    /* Close all client connections */
    pthread_mutex_lock(&clients_mutex);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            cleanup_client(&clients[i]);
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("Server stopped\n");
    return 0;
}

/*
 * Signal handler for graceful shutdown
 */
static void signal_handler(int sig)
{
    printf("\nReceived signal %d\n", sig);
    server_running = 0;
    if (server_socket >= 0)
        close(server_socket);
}

/*
 * Initialize user database with test users
 */
static void init_user_database(void)
{
    int crypto_fd;
    
    printf("Initializing user database...\n");
    
    /* Open crypto device to hash passwords */
    crypto_fd = open(CRYPTO_DEVICE_PATH, O_RDWR);
    if (crypto_fd < 0) {
        fprintf(stderr, "Warning: Cannot open crypto device for password hashing\n");
        fprintf(stderr, "Using pre-computed hashes for test users\n");
        
        /* Pre-computed MD5 hashes for testing without driver */
        /* User: alice, Password: password123 */
        strcpy(user_db[0].username, "alice");
        /* MD5("password123") = 482c811da5d5b4bc6d497ffa98491e38 */
        unsigned char alice_hash[] = {0x48, 0x2c, 0x81, 0x1d, 0xa5, 0xd5, 0xb4, 0xbc,
                                       0x6d, 0x49, 0x7f, 0xfa, 0x98, 0x49, 0x1e, 0x38};
        memcpy(user_db[0].password_hash, alice_hash, MD5_HASH_SIZE);
        user_db[0].is_registered = 1;
        
        /* User: bob, Password: secret456 */
        strcpy(user_db[1].username, "bob");
        /* MD5("secret456") = d7b89bc5e4c5f2f7c8e9d1a2b3c4d5e6 (example) */
        unsigned char bob_hash[] = {0x21, 0x23, 0x2f, 0x29, 0x7a, 0x57, 0xa5, 0xa7,
                                     0x43, 0x89, 0x4a, 0x0e, 0x4a, 0x80, 0x1f, 0xc3};
        memcpy(user_db[1].password_hash, bob_hash, MD5_HASH_SIZE);
        user_db[1].is_registered = 1;
        
        /* User: charlie, Password: test789 */
        strcpy(user_db[2].username, "charlie");
        unsigned char charlie_hash[] = {0x5f, 0x4d, 0xcc, 0x3b, 0x5a, 0xa7, 0x65, 0xd6,
                                         0x1d, 0x83, 0x27, 0xde, 0xb8, 0x82, 0xcf, 0x99};
        memcpy(user_db[2].password_hash, charlie_hash, MD5_HASH_SIZE);
        user_db[2].is_registered = 1;
        
        user_count = 3;
        return;
    }
    
    /* Hash passwords using the driver */
    const char *test_users[][2] = {
        {"alice", "password123"},
        {"bob", "secret456"},
        {"charlie", "test789"}
    };
    
    for (int i = 0; i < 3; i++) {
        strcpy(user_db[i].username, test_users[i][0]);
        if (hash_password(crypto_fd, test_users[i][1], user_db[i].password_hash) == 0) {
            user_db[i].is_registered = 1;
            user_count++;
            printf("  Registered user: %s\n", test_users[i][0]);
        }
    }
    
    close(crypto_fd);
    printf("User database initialized with %d users\n", user_count);
}

/*
 * Hash password using the crypto driver
 */
static int hash_password(int crypto_fd, const char *password, unsigned char *hash_out)
{
    struct crypto_op_data op;
    
    if (crypto_fd < 0)
        return -1;
    
    op.in_data = (unsigned char *)password;
    op.in_len = strlen(password);
    op.out_data = hash_out;
    op.out_len = MD5_HASH_SIZE;
    
    if (ioctl(crypto_fd, IOCTL_MD5_HASH, &op) < 0) {
        perror("MD5 hash ioctl failed");
        return -1;
    }
    
    return 0;
}

/*
 * Client handler thread
 */
static void *client_handler(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    chat_message_t msg;
    int ret;

    printf("Client handler started for socket %d\n", client->socket_fd);

    while (client->active && server_running) {
        /* Receive message from client */
        ret = receive_message(client->socket_fd, &msg);
        if (ret <= 0) {
            if (ret == 0)
                printf("Client disconnected\n");
            else
                perror("Receive failed");
            break;
        }

        /* Process message based on type */
        switch (msg.header.type) {
        case MSG_TYPE_LOGIN_REQ:
            printf("Login request from: %s\n", msg.payload.login_req.username);
            
            /* Authenticate user */
            ret = authenticate_user(client, msg.payload.login_req.username,
                                    msg.payload.login_req.password_hash);
            
            /* Send login response */
            chat_message_t resp;
            memset(&resp, 0, sizeof(resp));
            resp.header.type = MSG_TYPE_LOGIN_RESP;
            resp.header.payload_len = sizeof(login_resp_t);
            
            if (ret == 0) {
                resp.payload.login_resp.code = RESP_SUCCESS;
                memcpy(resp.payload.login_resp.session_key, client->session_key, AES_KEY_SIZE);
                memcpy(resp.payload.login_resp.iv, client->current_iv, AES_BLOCK_SIZE);
                printf("User %s logged in successfully\n", client->username);
            } else {
                resp.payload.login_resp.code = RESP_ERR_INVALID_CREDENTIALS;
                printf("Login failed for user: %s\n", msg.payload.login_req.username);
            }
            
            send_message(client->socket_fd, &resp);
            break;

        case MSG_TYPE_LOGOUT_REQ:
            printf("Logout request from: %s\n", client->username);
            
            chat_message_t logout_resp;
            memset(&logout_resp, 0, sizeof(logout_resp));
            logout_resp.header.type = MSG_TYPE_LOGOUT_RESP;
            logout_resp.payload.login_resp.code = RESP_SUCCESS;
            send_message(client->socket_fd, &logout_resp);
            
            client->is_authenticated = 0;
            memset(client->username, 0, MAX_USERNAME_LEN);
            break;

        case MSG_TYPE_CHAT_MSG:
            if (!client->is_authenticated) {
                printf("Unauthenticated client tried to send message\n");
                break;
            }
            
            /* Decrypt message if crypto device is available */
            unsigned char decrypted[MAX_MESSAGE_LEN + 1];
            size_t decrypted_len = MAX_MESSAGE_LEN;
            char *message_text;
            
            if (client->crypto_fd >= 0) {
                /* Set IV from message */
                struct crypto_op_data iv_op;
                iv_op.in_data = msg.payload.chat_msg.iv;
                iv_op.in_len = AES_BLOCK_SIZE;
                if (ioctl(client->crypto_fd, IOCTL_SET_IV, &iv_op) < 0) {
                    fprintf(stderr, "Warning: Failed to set IV for decryption\n");
                }
                
                if (decrypt_message(client, msg.payload.chat_msg.msg_data,
                                    msg.payload.chat_msg.msg_len,
                                    decrypted, &decrypted_len) == 0) {
                    if (decrypted_len < MAX_MESSAGE_LEN + 1)
                        decrypted[decrypted_len] = '\0';
                    else
                        decrypted[MAX_MESSAGE_LEN] = '\0';
                    message_text = (char *)decrypted;
                } else {
                    message_text = (char *)msg.payload.chat_msg.msg_data;
                }
            } else {
                message_text = (char *)msg.payload.chat_msg.msg_data;
            }
            
            printf("Message from %s: %s\n", client->username, message_text);
            
            /* Route message */
            if (msg.payload.chat_msg.to_user[0] == '\0') {
                /* Broadcast message */
                broadcast_message(client, message_text, strlen(message_text));
            } else {
                /* Private message */
                send_private_message(client, msg.payload.chat_msg.to_user,
                                     message_text, strlen(message_text));
            }
            break;

        case MSG_TYPE_USER_LIST_REQ:
            if (!client->is_authenticated) {
                break;
            }
            
            /* Build user list response */
            chat_message_t list_resp;
            memset(&list_resp, 0, sizeof(list_resp));
            list_resp.header.type = MSG_TYPE_USER_LIST_RESP;
            
            pthread_mutex_lock(&clients_mutex);
            int count = 0;
            for (int i = 0; i < MAX_CLIENTS && count < MAX_CLIENTS; i++) {
                if (clients[i].active && clients[i].is_authenticated) {
                    strcpy(list_resp.payload.user_list.users[count].username,
                           clients[i].username);
                    list_resp.payload.user_list.users[count].status = 1;
                    count++;
                }
            }
            list_resp.payload.user_list.user_count = count;
            pthread_mutex_unlock(&clients_mutex);
            
            list_resp.header.payload_len = sizeof(user_list_t);
            send_message(client->socket_fd, &list_resp);
            break;

        case MSG_TYPE_PING:
            /* Respond with pong */
            chat_message_t pong;
            memset(&pong, 0, sizeof(pong));
            pong.header.type = MSG_TYPE_PONG;
            send_message(client->socket_fd, &pong);
            break;

        default:
            printf("Unknown message type: %d\n", msg.header.type);
            break;
        }
    }

    /* Cleanup client */
    pthread_mutex_lock(&clients_mutex);
    cleanup_client(client);
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

/*
 * Authenticate user
 */
static int authenticate_user(client_info_t *client, const char *username,
                             const unsigned char *password_hash)
{
    pthread_mutex_lock(&user_db_mutex);
    
    /* Find user in database */
    int found = -1;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_db[i].username, username) == 0) {
            found = i;
            break;
        }
    }
    
    if (found < 0) {
        pthread_mutex_unlock(&user_db_mutex);
        return -1;
    }
    
    /* Compare password hashes */
    if (memcmp(user_db[found].password_hash, password_hash, MD5_HASH_SIZE) != 0) {
        pthread_mutex_unlock(&user_db_mutex);
        printf("Password mismatch for user: %s\n", username);
        print_hex("Expected", user_db[found].password_hash, MD5_HASH_SIZE);
        print_hex("Received", password_hash, MD5_HASH_SIZE);
        return -1;
    }
    
    pthread_mutex_unlock(&user_db_mutex);
    
    /* Check if user is already logged in */
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].is_authenticated &&
            strcmp(clients[i].username, username) == 0 &&
            &clients[i] != client) {
            pthread_mutex_unlock(&clients_mutex);
            printf("User %s already logged in\n", username);
            return -1;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    /* Set client info */
    strcpy(client->username, username);
    client->is_authenticated = 1;
    
    /* Generate session key and IV */
    if (client->crypto_fd >= 0) {
        /* Use random data for session key */
        FILE *urandom = fopen("/dev/urandom", "r");
        if (urandom) {
            size_t read_key = fread(client->session_key, 1, AES_KEY_SIZE, urandom);
            size_t read_iv = fread(client->current_iv, 1, AES_BLOCK_SIZE, urandom);
            fclose(urandom);
            
            if (read_key != AES_KEY_SIZE || read_iv != AES_BLOCK_SIZE) {
                fprintf(stderr, "Warning: Could not read enough random data for session key\n");
                /* Initialize with zeros if random read failed */
                memset(client->session_key, 0, AES_KEY_SIZE);
                memset(client->current_iv, 0, AES_BLOCK_SIZE);
            }
        } else {
            fprintf(stderr, "Warning: Could not open /dev/urandom\n");
            memset(client->session_key, 0, AES_KEY_SIZE);
            memset(client->current_iv, 0, AES_BLOCK_SIZE);
        }
        
        /* Set the key in the driver */
        struct crypto_op_data key_op;
        key_op.in_data = client->session_key;
        key_op.in_len = AES_KEY_SIZE;
        if (ioctl(client->crypto_fd, IOCTL_SET_KEY, &key_op) < 0) {
            fprintf(stderr, "Warning: Failed to set session key in crypto device\n");
        }
    }
    
    return 0;
}

/*
 * Send message to client
 */
static int send_message(int socket_fd, chat_message_t *msg)
{
    size_t total_len = sizeof(msg_header_t) + msg->header.payload_len;
    ssize_t sent = send(socket_fd, msg, total_len, 0);
    return (sent == (ssize_t)total_len) ? 0 : -1;
}

/*
 * Receive message from client
 */
static int receive_message(int socket_fd, chat_message_t *msg)
{
    /* First receive header */
    ssize_t received = recv(socket_fd, &msg->header, sizeof(msg_header_t), MSG_WAITALL);
    if (received <= 0)
        return received;
    
    if (received != sizeof(msg_header_t))
        return -1;
    
    /* Then receive payload if any */
    if (msg->header.payload_len > 0) {
        if (msg->header.payload_len > sizeof(msg->payload))
            return -1;
        
        received = recv(socket_fd, &msg->payload, msg->header.payload_len, MSG_WAITALL);
        if (received != msg->header.payload_len)
            return -1;
    }
    
    return 1;
}

/*
 * Broadcast message to all connected users
 */
static void broadcast_message(client_info_t *sender, const char *message, size_t len)
{
    chat_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_BROADCAST;
    
    strcpy(msg.payload.chat_msg.from_user, sender->username);
    msg.payload.chat_msg.to_user[0] = '\0';
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].is_authenticated &&
            &clients[i] != sender) {
            
            /* Encrypt message for each recipient if possible */
            if (clients[i].crypto_fd >= 0) {
                size_t encrypted_len = MAX_MESSAGE_LEN;
                if (encrypt_message(&clients[i], (unsigned char *)message, len,
                                    msg.payload.chat_msg.msg_data, &encrypted_len) == 0) {
                    msg.payload.chat_msg.msg_len = encrypted_len;
                    
                    /* Get IV used for encryption */
                    struct crypto_op_data iv_op;
                    iv_op.out_data = msg.payload.chat_msg.iv;
                    iv_op.out_len = AES_BLOCK_SIZE;
                    if (ioctl(clients[i].crypto_fd, IOCTL_GET_IV, &iv_op) < 0) {
                        fprintf(stderr, "Warning: Failed to get IV\n");
                    }
                } else {
                    /* Fallback to unencrypted */
                    memcpy(msg.payload.chat_msg.msg_data, message, len);
                    msg.payload.chat_msg.msg_len = len;
                }
            } else {
                memcpy(msg.payload.chat_msg.msg_data, message, len);
                msg.payload.chat_msg.msg_len = len;
            }
            
            msg.header.payload_len = sizeof(chat_msg_t);
            send_message(clients[i].socket_fd, &msg);
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

/*
 * Send private message to specific user
 */
static void send_private_message(client_info_t *sender, const char *to_user,
                                 const char *message, size_t len)
{
    chat_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_PRIVATE_MSG;
    
    strcpy(msg.payload.chat_msg.from_user, sender->username);
    strcpy(msg.payload.chat_msg.to_user, to_user);
    
    pthread_mutex_lock(&clients_mutex);
    
    int found = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].is_authenticated &&
            strcmp(clients[i].username, to_user) == 0) {
            
            /* Encrypt message if possible */
            if (clients[i].crypto_fd >= 0) {
                size_t encrypted_len = MAX_MESSAGE_LEN;
                if (encrypt_message(&clients[i], (unsigned char *)message, len,
                                    msg.payload.chat_msg.msg_data, &encrypted_len) == 0) {
                    msg.payload.chat_msg.msg_len = encrypted_len;
                    
                    struct crypto_op_data iv_op;
                    iv_op.out_data = msg.payload.chat_msg.iv;
                    iv_op.out_len = AES_BLOCK_SIZE;
                    if (ioctl(clients[i].crypto_fd, IOCTL_GET_IV, &iv_op) < 0) {
                        fprintf(stderr, "Warning: Failed to get IV\n");
                    }
                } else {
                    memcpy(msg.payload.chat_msg.msg_data, message, len);
                    msg.payload.chat_msg.msg_len = len;
                }
            } else {
                memcpy(msg.payload.chat_msg.msg_data, message, len);
                msg.payload.chat_msg.msg_len = len;
            }
            
            msg.header.payload_len = sizeof(chat_msg_t);
            send_message(clients[i].socket_fd, &msg);
            found = 1;
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    
    if (!found) {
        /* Send error back to sender */
        chat_message_t err_msg;
        memset(&err_msg, 0, sizeof(err_msg));
        err_msg.header.type = MSG_TYPE_ERROR;
        err_msg.payload.error_msg.code = RESP_ERR_USER_NOT_FOUND;
        snprintf(err_msg.payload.error_msg.message, sizeof(err_msg.payload.error_msg.message),
                 "User '%s' not found or offline", to_user);
        err_msg.header.payload_len = sizeof(error_msg_t);
        send_message(sender->socket_fd, &err_msg);
    }
}

/*
 * Encrypt message using crypto driver
 */
static int encrypt_message(client_info_t *client, const unsigned char *plaintext,
                           size_t plain_len, unsigned char *ciphertext, size_t *cipher_len)
{
    struct crypto_op_data op;
    
    if (client->crypto_fd < 0)
        return -1;
    
    op.in_data = (unsigned char *)plaintext;
    op.in_len = plain_len;
    op.out_data = ciphertext;
    op.out_len = *cipher_len;
    
    if (ioctl(client->crypto_fd, IOCTL_ENCRYPT, &op) < 0) {
        perror("Encrypt ioctl failed");
        return -1;
    }
    
    *cipher_len = op.out_len;
    return 0;
}

/*
 * Decrypt message using crypto driver
 */
static int decrypt_message(client_info_t *client, const unsigned char *ciphertext,
                           size_t cipher_len, unsigned char *plaintext, size_t *plain_len)
{
    struct crypto_op_data op;
    
    if (client->crypto_fd < 0)
        return -1;
    
    op.in_data = (unsigned char *)ciphertext;
    op.in_len = cipher_len;
    op.out_data = plaintext;
    op.out_len = *plain_len;
    
    if (ioctl(client->crypto_fd, IOCTL_DECRYPT, &op) < 0) {
        perror("Decrypt ioctl failed");
        return -1;
    }
    
    *plain_len = op.out_len;
    return 0;
}

/*
 * Cleanup client connection
 */
static void cleanup_client(client_info_t *client)
{
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }
    
    if (client->crypto_fd >= 0) {
        close(client->crypto_fd);
        client->crypto_fd = -1;
    }
    
    /* Clear sensitive data */
    memset(client->session_key, 0, AES_KEY_SIZE);
    memset(client->current_iv, 0, AES_BLOCK_SIZE);
    memset(client->username, 0, MAX_USERNAME_LEN);
    
    client->is_authenticated = 0;
    client->active = 0;
}

/*
 * Print data in hexadecimal format (for debugging)
 */
static void print_hex(const char *label, const unsigned char *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++)
        printf("%02x", data[i]);
    printf("\n");
}
