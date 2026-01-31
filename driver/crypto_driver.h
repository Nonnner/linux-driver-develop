/*
 * crypto_driver.h - Header file for Crypto Device Driver
 * 
 * This header defines the ioctl commands and data structures used for
 * communication between user space applications and the crypto driver.
 */

#ifndef _CRYPTO_DRIVER_H
#define _CRYPTO_DRIVER_H

#include <linux/ioctl.h>

/* Constants */
#define AES_KEY_SIZE     16      /* AES-128 key size in bytes */
#define AES_BLOCK_SIZE   16      /* AES block size in bytes */
#define MD5_HASH_SIZE    16      /* MD5 hash output size in bytes */
#define CRYPTO_BUFFER_SIZE 4096  /* Maximum buffer size for crypto operations */

/* IOCTL magic number */
#define CRYPTO_IOC_MAGIC 'C'

/* 
 * Data structure for crypto operations
 * Used to pass data between user space and kernel space
 */
struct crypto_op_data {
    unsigned char __user *in_data;   /* Input data buffer (user space pointer) */
    size_t in_len;                   /* Length of input data */
    unsigned char __user *out_data;  /* Output data buffer (user space pointer) */
    size_t out_len;                  /* Length of output buffer / actual output length */
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

#endif /* _CRYPTO_DRIVER_H */
