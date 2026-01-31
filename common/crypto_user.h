/*
 * crypto_user.h - User space header for Crypto Device Driver communication
 * 
 * This header provides user space applications with the interface to
 * communicate with the crypto device driver for AES and MD5 operations.
 */

#ifndef _CRYPTO_USER_H
#define _CRYPTO_USER_H

#include <sys/ioctl.h>
#include <stddef.h>

/* Constants */
#define AES_KEY_SIZE     16      /* AES-128 key size in bytes */
#define AES_BLOCK_SIZE   16      /* AES block size in bytes */
#define MD5_HASH_SIZE    16      /* MD5 hash output size in bytes */
#define CRYPTO_BUFFER_SIZE 4096  /* Maximum buffer size for crypto operations */

/* Device path */
#define CRYPTO_DEVICE_PATH "/dev/crypto_dev"

/* IOCTL magic number */
#define CRYPTO_IOC_MAGIC 'C'

/* 
 * Data structure for crypto operations
 * Used to pass data between user space and kernel space
 */
struct crypto_op_data {
    unsigned char *in_data;   /* Input data buffer */
    size_t in_len;            /* Length of input data */
    unsigned char *out_data;  /* Output data buffer */
    size_t out_len;           /* Length of output buffer / actual output length */
};

/* IOCTL Commands */

/* Set AES encryption key (input: AES_KEY_SIZE bytes key) */
#define IOCTL_SET_KEY    _IOW(CRYPTO_IOC_MAGIC, 1, struct crypto_op_data)

/* Get current IV (output: AES_BLOCK_SIZE bytes IV) */
#define IOCTL_GET_IV     _IOR(CRYPTO_IOC_MAGIC, 2, struct crypto_op_data)

/* Set IV (input: AES_BLOCK_SIZE bytes IV) */
#define IOCTL_SET_IV     _IOW(CRYPTO_IOC_MAGIC, 3, struct crypto_op_data)

/* Encrypt data using AES-CBC (input: plaintext, output: ciphertext) */
#define IOCTL_ENCRYPT    _IOWR(CRYPTO_IOC_MAGIC, 4, struct crypto_op_data)

/* Decrypt data using AES-CBC (input: ciphertext, output: plaintext) */
#define IOCTL_DECRYPT    _IOWR(CRYPTO_IOC_MAGIC, 5, struct crypto_op_data)

/* Compute MD5 hash (input: data, output: 16-byte hash) */
#define IOCTL_MD5_HASH   _IOWR(CRYPTO_IOC_MAGIC, 6, struct crypto_op_data)

#endif /* _CRYPTO_USER_H */
