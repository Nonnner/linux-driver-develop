/* crypto_user.h - User space interface for crypto driver
 * This header defines the interface between user space and the crypto device driver
 */
#ifndef _CRYPTO_USER_H
#define _CRYPTO_USER_H

#include <stdint.h>
#include <sys/ioctl.h>

/* Device path */
#define CRYPTO_DEV_PATH "/dev/crypto_dev"

/* IOCTL magic number */
#define CRYPTO_IOC_MAGIC 'c'

/* Maximum data sizes */
#define CRYPTO_MAX_DATA_SIZE 4096
#define CRYPTO_AES_KEY_SIZE  16
#define CRYPTO_AES_IV_SIZE   16
#define CRYPTO_MD5_SIZE      16

/* Data structure for IOCTL */
struct crypto_data {
    unsigned char input[CRYPTO_MAX_DATA_SIZE];
    unsigned char output[CRYPTO_MAX_DATA_SIZE];
    unsigned char key[CRYPTO_AES_KEY_SIZE];
    unsigned char iv[CRYPTO_AES_IV_SIZE];
    unsigned int input_len;
    unsigned int output_len;
};

/* IOCTL commands */
#define IOCTL_SET_KEY      _IOW(CRYPTO_IOC_MAGIC, 1, unsigned char[CRYPTO_AES_KEY_SIZE])
#define IOCTL_SET_IV       _IOW(CRYPTO_IOC_MAGIC, 2, unsigned char[CRYPTO_AES_IV_SIZE])
#define IOCTL_GET_IV       _IOR(CRYPTO_IOC_MAGIC, 3, unsigned char[CRYPTO_AES_IV_SIZE])
#define IOCTL_ENCRYPT      _IOWR(CRYPTO_IOC_MAGIC, 4, struct crypto_data)
#define IOCTL_DECRYPT      _IOWR(CRYPTO_IOC_MAGIC, 5, struct crypto_data)
#define IOCTL_MD5_HASH     _IOWR(CRYPTO_IOC_MAGIC, 6, struct crypto_data)

/* Helper functions for user space applications */

/* Open crypto device */
static inline int crypto_open(void) {
    return open(CRYPTO_DEV_PATH, O_RDWR);
}

/* Close crypto device */
static inline void crypto_close(int fd) {
    if (fd >= 0) close(fd);
}

#endif /* _CRYPTO_USER_H */
