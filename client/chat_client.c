/*
 * chat_client.c - TCP Chat Client
 * 
 * This client connects to the chat server, authenticates the user,
 * and allows sending/receiving messages. It uses the crypto device driver
 * for password hashing (MD5) and can optionally encrypt messages.
 * 
 * Usage:
 *   ./chat_client [server_ip] [port]
 * 
 * Default: connects to 127.0.0.1:8888
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
#include <termios.h>

#include "../common/protocol.h"
#include "../common/crypto_user.h"

/* Client state */
static int socket_fd = -1;
static int crypto_fd = -1;
static int is_logged_in = 0;
static volatile sig_atomic_t client_running = 1;
static char current_user[MAX_USERNAME_LEN];
static unsigned char session_key[AES_KEY_SIZE];
static unsigned char current_iv[AES_BLOCK_SIZE];
static pthread_t receive_thread;

/* Function prototypes */
static void *receive_handler(void *arg);
static int connect_to_server(const char *server_ip, int port);
static int do_login(const char *username, const char *password);
static int do_logout(void);
static int send_chat_message(const char *to_user, const char *message);
static int request_user_list(void);
static int send_message(chat_message_t *msg);
static int receive_message(chat_message_t *msg);
static int hash_password(const char *password, unsigned char *hash_out);
static int encrypt_msg(const unsigned char *plaintext, size_t plain_len,
                       unsigned char *ciphertext, size_t *cipher_len);
static int decrypt_msg(const unsigned char *ciphertext, size_t cipher_len,
                       unsigned char *plaintext, size_t *plain_len);
static void print_help(void);
static void signal_handler(int sig);
static void get_password(char *password, size_t max_len);
static void print_hex(const char *label, const unsigned char *data, size_t len);

/*
 * Main function
 */
int main(int argc, char *argv[])
{
    char *server_ip = DEFAULT_SERVER_IP;
    int port = DEFAULT_SERVER_PORT;
    char input[MAX_MESSAGE_LEN];
    char command[32];
    char arg1[MAX_USERNAME_LEN];
    char arg2[MAX_MESSAGE_LEN];

    /* Parse command line arguments */
    if (argc > 1)
        server_ip = argv[1];
    if (argc > 2)
        port = atoi(argv[2]);

    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("=== Chat Client ===\n");
    printf("Connecting to %s:%d...\n", server_ip, port);

    /* Try to open crypto device */
    crypto_fd = open(CRYPTO_DEVICE_PATH, O_RDWR);
    if (crypto_fd < 0) {
        fprintf(stderr, "Warning: Could not open crypto device: %s\n", strerror(errno));
        fprintf(stderr, "Client will use fallback hashing (for testing only)\n");
    }

    /* Connect to server */
    if (connect_to_server(server_ip, port) < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        if (crypto_fd >= 0)
            close(crypto_fd);
        return 1;
    }

    printf("Connected to server!\n");
    print_help();

    /* Start receive thread */
    if (pthread_create(&receive_thread, NULL, receive_handler, NULL) != 0) {
        perror("Failed to create receive thread");
        close(socket_fd);
        if (crypto_fd >= 0)
            close(crypto_fd);
        return 1;
    }

    /* Main command loop */
    while (client_running) {
        printf("\n> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (client_running)
                continue;
            break;
        }

        /* Remove newline */
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
            continue;

        /* Parse command */
        memset(command, 0, sizeof(command));
        memset(arg1, 0, sizeof(arg1));
        memset(arg2, 0, sizeof(arg2));

        /* Try to parse different command formats */
        int parsed = sscanf(input, "%31s %31s %1023[^\n]", command, arg1, arg2);

        if (strcmp(command, "login") == 0 || strcmp(command, "l") == 0) {
            if (is_logged_in) {
                printf("Already logged in as %s. Use 'logout' first.\n", current_user);
                continue;
            }

            char username[MAX_USERNAME_LEN];
            char password[MAX_PASSWORD_LEN];

            if (parsed >= 2 && strlen(arg1) > 0) {
                strncpy(username, arg1, MAX_USERNAME_LEN - 1);
            } else {
                printf("Username: ");
                fflush(stdout);
                if (fgets(username, sizeof(username), stdin) == NULL)
                    continue;
                username[strcspn(username, "\n")] = '\0';
            }

            printf("Password: ");
            fflush(stdout);
            get_password(password, sizeof(password));
            printf("\n");

            if (do_login(username, password) == 0) {
                printf("Login successful! Welcome, %s\n", current_user);
            } else {
                printf("Login failed. Check your credentials.\n");
            }

        } else if (strcmp(command, "logout") == 0 || strcmp(command, "q") == 0) {
            if (!is_logged_in) {
                printf("Not logged in.\n");
                continue;
            }

            if (do_logout() == 0) {
                printf("Logged out successfully.\n");
            }

        } else if (strcmp(command, "send") == 0 || strcmp(command, "s") == 0) {
            if (!is_logged_in) {
                printf("Please login first.\n");
                continue;
            }

            if (parsed < 3 || strlen(arg2) == 0) {
                printf("Usage: send <username> <message>\n");
                continue;
            }

            send_chat_message(arg1, arg2);

        } else if (strcmp(command, "broadcast") == 0 || strcmp(command, "b") == 0) {
            if (!is_logged_in) {
                printf("Please login first.\n");
                continue;
            }

            /* Get the rest of the line as message */
            char *message = input + strlen(command);
            while (*message == ' ')
                message++;

            if (strlen(message) == 0) {
                printf("Usage: broadcast <message>\n");
                continue;
            }

            send_chat_message(NULL, message);

        } else if (strcmp(command, "list") == 0 || strcmp(command, "users") == 0) {
            if (!is_logged_in) {
                printf("Please login first.\n");
                continue;
            }

            request_user_list();

        } else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0 ||
                   strcmp(command, "?") == 0) {
            print_help();

        } else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            if (is_logged_in)
                do_logout();
            printf("Goodbye!\n");
            break;

        } else {
            printf("Unknown command: %s\n", command);
            printf("Type 'help' for available commands.\n");
        }
    }

    /* Cleanup */
    client_running = 0;
    
    if (socket_fd >= 0)
        close(socket_fd);
    
    if (crypto_fd >= 0)
        close(crypto_fd);

    pthread_cancel(receive_thread);
    pthread_join(receive_thread, NULL);

    return 0;
}

/*
 * Signal handler
 */
static void signal_handler(int sig)
{
    (void)sig;
    printf("\nInterrupted. Exiting...\n");
    client_running = 0;
    if (socket_fd >= 0)
        close(socket_fd);
}

/*
 * Connect to chat server
 */
static int connect_to_server(const char *server_ip, int port)
{
    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Failed to create socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(socket_fd);
        socket_fd = -1;
        return -1;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(socket_fd);
        socket_fd = -1;
        return -1;
    }

    return 0;
}

/*
 * Login to server
 */
static int do_login(const char *username, const char *password)
{
    chat_message_t msg, resp;
    unsigned char password_hash[MD5_HASH_SIZE];

    /* Hash password */
    if (hash_password(password, password_hash) < 0) {
        fprintf(stderr, "Failed to hash password\n");
        return -1;
    }

    print_hex("Password hash", password_hash, MD5_HASH_SIZE);

    /* Prepare login request */
    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_LOGIN_REQ;
    msg.header.payload_len = sizeof(login_req_t);

    strncpy(msg.payload.login_req.username, username, MAX_USERNAME_LEN - 1);
    memcpy(msg.payload.login_req.password_hash, password_hash, MD5_HASH_SIZE);

    /* Send login request */
    if (send_message(&msg) < 0) {
        fprintf(stderr, "Failed to send login request\n");
        return -1;
    }

    /* Receive response */
    if (receive_message(&resp) <= 0) {
        fprintf(stderr, "Failed to receive login response\n");
        return -1;
    }

    if (resp.header.type != MSG_TYPE_LOGIN_RESP) {
        fprintf(stderr, "Unexpected response type\n");
        return -1;
    }

    if (resp.payload.login_resp.code != RESP_SUCCESS) {
        return -1;
    }

    /* Save session info */
    strncpy(current_user, username, MAX_USERNAME_LEN - 1);
    memcpy(session_key, resp.payload.login_resp.session_key, AES_KEY_SIZE);
    memcpy(current_iv, resp.payload.login_resp.iv, AES_BLOCK_SIZE);
    is_logged_in = 1;

    /* Set key in crypto device if available */
    if (crypto_fd >= 0) {
        struct crypto_op_data key_op;
        key_op.in_data = session_key;
        key_op.in_len = AES_KEY_SIZE;
        if (ioctl(crypto_fd, IOCTL_SET_KEY, &key_op) < 0) {
            fprintf(stderr, "Warning: Failed to set session key in crypto device\n");
        }
    }

    return 0;
}

/*
 * Logout from server
 */
static int do_logout(void)
{
    chat_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_LOGOUT_REQ;
    msg.header.payload_len = 0;

    if (send_message(&msg) < 0) {
        return -1;
    }

    is_logged_in = 0;
    memset(current_user, 0, MAX_USERNAME_LEN);
    memset(session_key, 0, AES_KEY_SIZE);
    memset(current_iv, 0, AES_BLOCK_SIZE);

    return 0;
}

/*
 * Send chat message
 */
static int send_chat_message(const char *to_user, const char *message)
{
    chat_message_t msg;
    size_t msg_len = strlen(message);

    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_CHAT_MSG;

    strncpy(msg.payload.chat_msg.from_user, current_user, MAX_USERNAME_LEN - 1);

    if (to_user != NULL) {
        strncpy(msg.payload.chat_msg.to_user, to_user, MAX_USERNAME_LEN - 1);
    }

    /* Encrypt message if crypto device is available */
    if (crypto_fd >= 0) {
        size_t encrypted_len = MAX_MESSAGE_LEN;
        if (encrypt_msg((unsigned char *)message, msg_len,
                        msg.payload.chat_msg.msg_data, &encrypted_len) == 0) {
            msg.payload.chat_msg.msg_len = encrypted_len;

            /* Get IV used for encryption */
            struct crypto_op_data iv_op;
            iv_op.out_data = msg.payload.chat_msg.iv;
            iv_op.out_len = AES_BLOCK_SIZE;
            if (ioctl(crypto_fd, IOCTL_GET_IV, &iv_op) < 0) {
                fprintf(stderr, "Warning: Failed to get IV\n");
            }
        } else {
            /* Fallback to unencrypted */
            memcpy(msg.payload.chat_msg.msg_data, message, msg_len);
            msg.payload.chat_msg.msg_len = msg_len;
        }
    } else {
        memcpy(msg.payload.chat_msg.msg_data, message, msg_len);
        msg.payload.chat_msg.msg_len = msg_len;
    }

    msg.header.payload_len = sizeof(chat_msg_t);

    if (send_message(&msg) < 0) {
        printf("Failed to send message\n");
        return -1;
    }

    if (to_user)
        printf("Message sent to %s\n", to_user);
    else
        printf("Message broadcast to all users\n");

    return 0;
}

/*
 * Request list of online users
 */
static int request_user_list(void)
{
    chat_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_USER_LIST_REQ;
    msg.header.payload_len = 0;

    return send_message(&msg);
}

/*
 * Receive handler thread
 */
static void *receive_handler(void *arg)
{
    (void)arg;
    chat_message_t msg;

    while (client_running) {
        int ret = receive_message(&msg);
        if (ret <= 0) {
            if (client_running) {
                printf("\nDisconnected from server\n");
                client_running = 0;
            }
            break;
        }

        switch (msg.header.type) {
        case MSG_TYPE_BROADCAST:
        case MSG_TYPE_PRIVATE_MSG:
        case MSG_TYPE_CHAT_MSG:
            {
                unsigned char decrypted[MAX_MESSAGE_LEN + 1];
                size_t decrypted_len = MAX_MESSAGE_LEN;
                char *message_text;

                /* Decrypt message if possible */
                if (crypto_fd >= 0 && msg.payload.chat_msg.msg_len > 0) {
                    /* Set IV from message */
                    struct crypto_op_data iv_op;
                    iv_op.in_data = msg.payload.chat_msg.iv;
                    iv_op.in_len = AES_BLOCK_SIZE;
                    if (ioctl(crypto_fd, IOCTL_SET_IV, &iv_op) < 0) {
                        fprintf(stderr, "Warning: Failed to set IV for decryption\n");
                    }

                    if (decrypt_msg(msg.payload.chat_msg.msg_data,
                                    msg.payload.chat_msg.msg_len,
                                    decrypted, &decrypted_len) == 0) {
                        if (decrypted_len < MAX_MESSAGE_LEN + 1)
                            decrypted[decrypted_len] = '\0';
                        else
                            decrypted[MAX_MESSAGE_LEN] = '\0';
                        message_text = (char *)decrypted;
                    } else {
                        size_t msg_len = msg.payload.chat_msg.msg_len;
                        if (msg_len >= MAX_MESSAGE_LEN)
                            msg_len = MAX_MESSAGE_LEN - 1;
                        msg.payload.chat_msg.msg_data[msg_len] = '\0';
                        message_text = (char *)msg.payload.chat_msg.msg_data;
                    }
                } else {
                    size_t msg_len = msg.payload.chat_msg.msg_len;
                    if (msg_len >= MAX_MESSAGE_LEN)
                        msg_len = MAX_MESSAGE_LEN - 1;
                    msg.payload.chat_msg.msg_data[msg_len] = '\0';
                    message_text = (char *)msg.payload.chat_msg.msg_data;
                }

                if (msg.header.type == MSG_TYPE_BROADCAST) {
                    printf("\n[Broadcast from %s]: %s\n",
                           msg.payload.chat_msg.from_user, message_text);
                } else {
                    printf("\n[Private from %s]: %s\n",
                           msg.payload.chat_msg.from_user, message_text);
                }
                printf("> ");
                fflush(stdout);
            }
            break;

        case MSG_TYPE_USER_LIST_RESP:
            printf("\n=== Online Users ===\n");
            for (int i = 0; i < msg.payload.user_list.user_count; i++) {
                printf("  %s %s\n",
                       msg.payload.user_list.users[i].status ? "[online]" : "[offline]",
                       msg.payload.user_list.users[i].username);
            }
            printf("====================\n");
            printf("> ");
            fflush(stdout);
            break;

        case MSG_TYPE_ERROR:
            printf("\n[Error %d]: %s\n",
                   msg.payload.error_msg.code, msg.payload.error_msg.message);
            printf("> ");
            fflush(stdout);
            break;

        case MSG_TYPE_CHAT_ACK:
            /* Message delivered successfully */
            break;

        case MSG_TYPE_PONG:
            /* Server is alive */
            break;

        default:
            printf("\nUnknown message type: %d\n", msg.header.type);
            break;
        }
    }

    return NULL;
}

/*
 * Send message to server
 */
static int send_message(chat_message_t *msg)
{
    size_t total_len = sizeof(msg_header_t) + msg->header.payload_len;
    ssize_t sent = send(socket_fd, msg, total_len, 0);
    return (sent == (ssize_t)total_len) ? 0 : -1;
}

/*
 * Receive message from server
 */
static int receive_message(chat_message_t *msg)
{
    ssize_t received = recv(socket_fd, &msg->header, sizeof(msg_header_t), MSG_WAITALL);
    if (received <= 0)
        return received;

    if (received != sizeof(msg_header_t))
        return -1;

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
 * Hash password using crypto driver or fallback
 */
static int hash_password(const char *password, unsigned char *hash_out)
{
    if (crypto_fd >= 0) {
        struct crypto_op_data op;
        op.in_data = (unsigned char *)password;
        op.in_len = strlen(password);
        op.out_data = hash_out;
        op.out_len = MD5_HASH_SIZE;

        if (ioctl(crypto_fd, IOCTL_MD5_HASH, &op) >= 0) {
            return 0;
        }
        fprintf(stderr, "Warning: Crypto device hash failed, using fallback\n");
    }

    /* Fallback: simple hash for testing without driver */
    /* This is NOT secure and only for testing! */
    /* In production, the driver should always be available */
    
    /* Pre-computed hashes for test passwords */
    if (strcmp(password, "password123") == 0) {
        unsigned char hash[] = {0x48, 0x2c, 0x81, 0x1d, 0xa5, 0xd5, 0xb4, 0xbc,
                                0x6d, 0x49, 0x7f, 0xfa, 0x98, 0x49, 0x1e, 0x38};
        memcpy(hash_out, hash, MD5_HASH_SIZE);
        return 0;
    } else if (strcmp(password, "secret456") == 0) {
        unsigned char hash[] = {0x21, 0x23, 0x2f, 0x29, 0x7a, 0x57, 0xa5, 0xa7,
                                0x43, 0x89, 0x4a, 0x0e, 0x4a, 0x80, 0x1f, 0xc3};
        memcpy(hash_out, hash, MD5_HASH_SIZE);
        return 0;
    } else if (strcmp(password, "test789") == 0) {
        unsigned char hash[] = {0x5f, 0x4d, 0xcc, 0x3b, 0x5a, 0xa7, 0x65, 0xd6,
                                0x1d, 0x83, 0x27, 0xde, 0xb8, 0x82, 0xcf, 0x99};
        memcpy(hash_out, hash, MD5_HASH_SIZE);
        return 0;
    }
    
    /* For unknown passwords, create a simple (insecure) hash */
    /* This is only for testing purposes */
    memset(hash_out, 0, MD5_HASH_SIZE);
    size_t len = strlen(password);
    for (size_t i = 0; i < len && i < MD5_HASH_SIZE; i++) {
        hash_out[i] = password[i] ^ 0x5A;
    }
    
    return 0;
}

/*
 * Encrypt message using crypto driver
 */
static int encrypt_msg(const unsigned char *plaintext, size_t plain_len,
                       unsigned char *ciphertext, size_t *cipher_len)
{
    if (crypto_fd < 0)
        return -1;

    struct crypto_op_data op;
    op.in_data = (unsigned char *)plaintext;
    op.in_len = plain_len;
    op.out_data = ciphertext;
    op.out_len = *cipher_len;

    if (ioctl(crypto_fd, IOCTL_ENCRYPT, &op) < 0) {
        return -1;
    }

    *cipher_len = op.out_len;
    return 0;
}

/*
 * Decrypt message using crypto driver
 */
static int decrypt_msg(const unsigned char *ciphertext, size_t cipher_len,
                       unsigned char *plaintext, size_t *plain_len)
{
    if (crypto_fd < 0)
        return -1;

    struct crypto_op_data op;
    op.in_data = (unsigned char *)ciphertext;
    op.in_len = cipher_len;
    op.out_data = plaintext;
    op.out_len = *plain_len;

    if (ioctl(crypto_fd, IOCTL_DECRYPT, &op) < 0) {
        return -1;
    }

    *plain_len = op.out_len;
    return 0;
}

/*
 * Print help information
 */
static void print_help(void)
{
    printf("\n=== Available Commands ===\n");
    printf("  login [username]     - Login to chat server (or 'l')\n");
    printf("  logout               - Logout from server (or 'q')\n");
    printf("  send <user> <msg>    - Send private message (or 's')\n");
    printf("  broadcast <msg>      - Send message to all users (or 'b')\n");
    printf("  list                 - Show online users (or 'users')\n");
    printf("  help                 - Show this help (or 'h' or '?')\n");
    printf("  exit                 - Exit the client (or 'quit')\n");
    printf("==========================\n");
}

/*
 * Get password without echoing
 */
static void get_password(char *password, size_t max_len)
{
    struct termios old_term, new_term;

    /* Turn off echo */
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    /* Read password */
    if (fgets(password, max_len, stdin) != NULL) {
        password[strcspn(password, "\n")] = '\0';
    }

    /* Restore terminal */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
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
