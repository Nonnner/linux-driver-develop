/*
 * protocol.h - Chat Protocol Definitions
 * 
 * This header defines the message protocol used for communication
 * between chat clients and the chat server.
 */

#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include <stdint.h>

/* Protocol constants */
#define MAX_USERNAME_LEN    32
#define MAX_PASSWORD_LEN    32
#define MAX_MESSAGE_LEN     1024
#define MAX_CLIENTS         100

/* Default server configuration */
#define DEFAULT_SERVER_PORT 8888
#define DEFAULT_SERVER_IP   "127.0.0.1"

/* Message types */
typedef enum {
    MSG_TYPE_LOGIN_REQ = 1,     /* Client login request */
    MSG_TYPE_LOGIN_RESP,        /* Server login response */
    MSG_TYPE_LOGOUT_REQ,        /* Client logout request */
    MSG_TYPE_LOGOUT_RESP,       /* Server logout response */
    MSG_TYPE_CHAT_MSG,          /* Chat message */
    MSG_TYPE_CHAT_ACK,          /* Chat message acknowledgment */
    MSG_TYPE_USER_LIST_REQ,     /* Request online user list */
    MSG_TYPE_USER_LIST_RESP,    /* Online user list response */
    MSG_TYPE_BROADCAST,         /* Broadcast message to all users */
    MSG_TYPE_PRIVATE_MSG,       /* Private message to specific user */
    MSG_TYPE_ERROR,             /* Error message */
    MSG_TYPE_PING,              /* Keep-alive ping */
    MSG_TYPE_PONG,              /* Keep-alive pong */
    MSG_TYPE_REGISTER_REQ,      /* Client registration request */
    MSG_TYPE_REGISTER_RESP      /* Server registration response */
} message_type_t;

/* Response codes */
typedef enum {
    RESP_SUCCESS = 0,
    RESP_ERR_INVALID_CREDENTIALS,
    RESP_ERR_USER_ALREADY_LOGGED,
    RESP_ERR_USER_NOT_FOUND,
    RESP_ERR_SERVER_FULL,
    RESP_ERR_ENCRYPTION_FAILED,
    RESP_ERR_INTERNAL_ERROR,
    RESP_ERR_USER_ALREADY_EXISTS,    /* Username already registered */
    RESP_ERR_INVALID_USERNAME,       /* Invalid username format */
    RESP_ERR_INVALID_PASSWORD        /* Invalid password format */
} response_code_t;

/* 
 * Message header structure
 * All messages start with this header
 */
typedef struct {
    uint8_t  type;              /* Message type (message_type_t) */
    uint8_t  flags;             /* Message flags */
    uint16_t payload_len;       /* Length of payload data */
    uint32_t sequence;          /* Message sequence number */
} __attribute__((packed)) msg_header_t;

/*
 * Login request payload
 */
typedef struct {
    char username[MAX_USERNAME_LEN];
    unsigned char password_hash[16];  /* MD5 hash of password */
} __attribute__((packed)) login_req_t;

/*
 * Login response payload
 */
typedef struct {
    uint8_t  code;              /* Response code */
    unsigned char session_key[16];     /* AES session key (encrypted) */
    unsigned char iv[16];              /* Initial IV for encryption */
} __attribute__((packed)) login_resp_t;

/*
 * Registration request payload (same as login)
 */
typedef struct {
    char username[MAX_USERNAME_LEN];
    unsigned char password_hash[16];  /* MD5 hash of password */
} __attribute__((packed)) register_req_t;

/*
 * Registration response payload
 */
typedef struct {
    uint8_t code;               /* Response code */
    char message[64];           /* Status message */
} __attribute__((packed)) register_resp_t;

/*
 * Chat message payload
 * The actual message content is encrypted
 */
typedef struct {
    char from_user[MAX_USERNAME_LEN];
    char to_user[MAX_USERNAME_LEN];   /* Empty for broadcast */
    uint16_t msg_len;
    unsigned char msg_data[MAX_MESSAGE_LEN];  /* Encrypted message */
    unsigned char iv[16];                      /* IV used for this message */
} __attribute__((packed)) chat_msg_t;

/*
 * User list entry
 */
typedef struct {
    char username[MAX_USERNAME_LEN];
    uint8_t status;             /* 0: offline, 1: online */
} __attribute__((packed)) user_entry_t;

/*
 * User list response payload
 */
typedef struct {
    uint8_t user_count;
    user_entry_t users[MAX_CLIENTS];
} __attribute__((packed)) user_list_t;

/*
 * Error message payload
 */
typedef struct {
    uint8_t code;
    char message[256];
} __attribute__((packed)) error_msg_t;

/*
 * Complete message structure
 */
typedef struct {
    msg_header_t header;
    union {
        login_req_t     login_req;
        login_resp_t    login_resp;
        register_req_t  register_req;
        register_resp_t register_resp;
        chat_msg_t      chat_msg;
        user_list_t     user_list;
        error_msg_t     error_msg;
        unsigned char   raw[4096];
    } payload;
} chat_message_t;

#endif /* _PROTOCOL_H */
