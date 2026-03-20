/* protocol.h - Chat Protocol Definitions
 * This header defines the message formats and protocol for the multi-user chat system
 */
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include <stdint.h>

/* Protocol constants */
#define MAX_USERNAME_LEN     32     
#define MAX_PASSWORD_LEN     32     
#define MAX_MESSAGE_LEN      512    
#define MAX_CLIENTS          100    
#define AES_KEY_LEN          16     
#define AES_BLOCK_SIZE       16     
#define MD5_DIGEST_SIZE      16     
#define BUFFER_SIZE          4096

/* Message types */
typedef enum {
    LOGIN_REQ = 1,
    LOGIN_RESP,
    LOGOUT_REQ,
    LOGOUT_RESP,
    CHAT_MSG,
    CHAT_ACK,
    USER_LIST_REQ,
    USER_LIST_RESP,
    PRIVATE_MSG,
    BROADCAST_MSG,
    ERROR_MSG,
    PING,
    PONG
} MsgType;

/* Error codes */
typedef enum {
    SUCCESS = 0,
    INVALID_CREDENTIALS = 1,
    ADMIN_ACTION_REQUIRED = 2,
    USER_ALREADY_LOGGED_IN = 3,
    USER_NOT_LOGGED_IN = 4,
    USER_NOT_FOUND = 5,
    MESSAGE_TOO_LONG = 6,
    INVALID_MESSAGE_FORMAT = 7
} ErrorCode;

/* Message header */
typedef struct {
    uint8_t msg_type;
    uint16_t payload_len;
    uint32_t timestamp;
} __attribute__((packed)) MsgHeader;

/* Login request */
typedef struct {
    MsgHeader header;
    char username[MAX_USERNAME_LEN];
    uint8_t password_hash[MD5_DIGEST_SIZE];
} __attribute__((packed)) LoginRequest;

/* Login response */
typedef struct {
    MsgHeader header;
    uint8_t status;                       /* ErrorCode */
    uint8_t session_key[AES_KEY_LEN];     /* Session key for encryption */
    uint8_t iv[AES_BLOCK_SIZE];           /* Initialization vector */
} __attribute__((packed)) LoginResponse;

/* Chat message */
typedef struct {
    MsgHeader header;
    char from_username[MAX_USERNAME_LEN];
    char to_username[MAX_USERNAME_LEN];
    uint16_t message_len;
    uint8_t iv[AES_BLOCK_SIZE];
    char message[MAX_MESSAGE_LEN];
} __attribute__((packed)) ChatMessage;

/* User list request */
typedef struct {
    MsgHeader header;
} __attribute__((packed)) UserListRequest;

/* User info in list */
typedef struct {
    char username[MAX_USERNAME_LEN];
    uint8_t online;
} __attribute__((packed)) UserInfo;

/* User list response */
typedef struct {
    MsgHeader header;
    uint16_t user_count;
    UserInfo users[0];  /* Variable length array */
} __attribute__((packed)) UserListResponse;

/* Error message */
typedef struct {
    MsgHeader header;
    uint8_t error_code;
    char error_message[256];
} __attribute__((packed)) ErrorMessage;

#endif /* _PROTOCOL_H */
