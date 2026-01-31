/*
 * crypto_driver.c - Linux Character Device Driver for AES and MD5 cryptographic operations
 * 
 * This driver provides cryptographic services (AES encryption/decryption and MD5 hashing)
 * using the Linux Kernel Crypto API. It implements a character device that can be accessed
 * from user space through standard file operations (open, close, ioctl, read, write).
 * 
 * Architecture:
 * - The driver operates in kernel space and provides mechanism (crypto operations)
 * - Policy decisions (when/what to encrypt) are handled by user space applications
 * - Communication is done via ioctl for control and read/write for data transfer
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <crypto/hash.h>
#include <crypto/skcipher.h>
#include <linux/scatterlist.h>
#include <linux/random.h>

#include "crypto_driver.h"

#define DEVICE_NAME "crypto_dev"
#define CLASS_NAME "crypto"

/* Module information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chat System Developer");
MODULE_DESCRIPTION("Crypto Device Driver for AES and MD5 operations");
MODULE_VERSION("1.0");

/* Device variables */
static int major_number;
static struct class *crypto_class = NULL;
static struct device *crypto_device = NULL;
static struct cdev crypto_cdev;

/* Crypto context for each open file */
struct crypto_context {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[AES_BLOCK_SIZE];
    int key_set;
    unsigned char buffer[CRYPTO_BUFFER_SIZE];
    size_t buffer_len;
    int last_operation; /* 0: none, 1: encrypt, 2: decrypt, 3: hash */
};

/*
 * MD5 Hash Function
 * Uses the Linux Kernel Crypto API to compute MD5 hash
 */
static int do_md5_hash(const unsigned char *data, size_t data_len,
                       unsigned char *hash_out)
{
    struct crypto_shash *tfm;
    struct shash_desc *desc;
    int ret;

    /* Allocate transformation object */
    tfm = crypto_alloc_shash("md5", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("crypto_driver: Failed to allocate MD5 transform\n");
        return PTR_ERR(tfm);
    }

    /* Allocate hash descriptor */
    desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!desc) {
        crypto_free_shash(tfm);
        return -ENOMEM;
    }

    desc->tfm = tfm;

    /* Compute hash */
    ret = crypto_shash_digest(desc, data, data_len, hash_out);
    if (ret)
        pr_err("crypto_driver: MD5 hashing failed\n");

    kfree(desc);
    crypto_free_shash(tfm);

    return ret;
}

/*
 * AES Encryption/Decryption Function
 * Uses the Linux Kernel Crypto API with CBC mode
 */
static int do_aes_crypt(const unsigned char *key, const unsigned char *iv,
                        const unsigned char *in, size_t in_len,
                        unsigned char *out, int encrypt)
{
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct scatterlist sg_in, sg_out;
    DECLARE_CRYPTO_WAIT(wait);
    unsigned char *iv_copy;
    int ret;

    /* Input length must be multiple of AES block size */
    if (in_len % AES_BLOCK_SIZE != 0) {
        pr_err("crypto_driver: Input length must be multiple of %d\n", AES_BLOCK_SIZE);
        return -EINVAL;
    }

    /* Allocate transformation object */
    tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("crypto_driver: Failed to allocate AES transform\n");
        return PTR_ERR(tfm);
    }

    /* Set the key */
    ret = crypto_skcipher_setkey(tfm, key, AES_KEY_SIZE);
    if (ret) {
        pr_err("crypto_driver: Failed to set AES key\n");
        crypto_free_skcipher(tfm);
        return ret;
    }

    /* Allocate request */
    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        crypto_free_skcipher(tfm);
        return -ENOMEM;
    }

    /* Copy IV because it will be modified */
    iv_copy = kmalloc(AES_BLOCK_SIZE, GFP_KERNEL);
    if (!iv_copy) {
        skcipher_request_free(req);
        crypto_free_skcipher(tfm);
        return -ENOMEM;
    }
    memcpy(iv_copy, iv, AES_BLOCK_SIZE);

    /* Setup scatterlists */
    sg_init_one(&sg_in, in, in_len);
    sg_init_one(&sg_out, out, in_len);

    /* Setup request */
    skcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                                  crypto_req_done, &wait);
    skcipher_request_set_crypt(req, &sg_in, &sg_out, in_len, iv_copy);

    /* Perform encryption or decryption */
    if (encrypt)
        ret = crypto_wait_req(crypto_skcipher_encrypt(req), &wait);
    else
        ret = crypto_wait_req(crypto_skcipher_decrypt(req), &wait);

    if (ret)
        pr_err("crypto_driver: AES %s failed\n", encrypt ? "encryption" : "decryption");

    kfree(iv_copy);
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);

    return ret;
}

/*
 * PKCS7 Padding Functions
 */
static size_t add_pkcs7_padding(unsigned char *data, size_t data_len, size_t buf_size)
{
    size_t padding_len = AES_BLOCK_SIZE - (data_len % AES_BLOCK_SIZE);
    size_t padded_len = data_len + padding_len;
    size_t i;

    if (padded_len > buf_size)
        return 0;

    for (i = 0; i < padding_len; i++)
        data[data_len + i] = (unsigned char)padding_len;

    return padded_len;
}

static size_t remove_pkcs7_padding(unsigned char *data, size_t data_len)
{
    unsigned char padding_len;
    size_t i;

    if (data_len == 0 || data_len % AES_BLOCK_SIZE != 0)
        return 0;

    padding_len = data[data_len - 1];
    if (padding_len == 0 || padding_len > AES_BLOCK_SIZE)
        return 0;

    /* Verify padding */
    for (i = 0; i < padding_len; i++) {
        if (data[data_len - 1 - i] != padding_len)
            return 0;
    }

    return data_len - padding_len;
}

/*
 * File Operations
 */

static int crypto_open(struct inode *inode, struct file *file)
{
    struct crypto_context *ctx;

    ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    ctx->key_set = 0;
    ctx->buffer_len = 0;
    ctx->last_operation = 0;

    file->private_data = ctx;

    pr_info("crypto_driver: Device opened\n");
    return 0;
}

static int crypto_release(struct inode *inode, struct file *file)
{
    struct crypto_context *ctx = file->private_data;

    if (ctx) {
        /* Clear sensitive data */
        memset(ctx->aes_key, 0, AES_KEY_SIZE);
        memset(ctx->iv, 0, AES_BLOCK_SIZE);
        memset(ctx->buffer, 0, ctx->buffer_len);
        kfree(ctx);
    }

    pr_info("crypto_driver: Device closed\n");
    return 0;
}

static ssize_t crypto_read(struct file *file, char __user *buf,
                           size_t count, loff_t *offset)
{
    struct crypto_context *ctx = file->private_data;
    size_t to_copy;

    if (!ctx || ctx->buffer_len == 0)
        return 0;

    to_copy = min(count, ctx->buffer_len);

    if (copy_to_user(buf, ctx->buffer, to_copy))
        return -EFAULT;

    ctx->buffer_len = 0;

    return to_copy;
}

static ssize_t crypto_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *offset)
{
    struct crypto_context *ctx = file->private_data;

    if (!ctx)
        return -EINVAL;

    if (count > CRYPTO_BUFFER_SIZE)
        count = CRYPTO_BUFFER_SIZE;

    if (copy_from_user(ctx->buffer, buf, count))
        return -EFAULT;

    ctx->buffer_len = count;

    return count;
}

static long crypto_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct crypto_context *ctx = file->private_data;
    struct crypto_op_data op_data;
    unsigned char *input = NULL;
    unsigned char *output = NULL;
    unsigned char *padded = NULL;
    size_t padded_len;
    int ret = 0;

    if (!ctx)
        return -EINVAL;

    switch (cmd) {
    case IOCTL_SET_KEY:
        /* Set AES key and IV */
        if (copy_from_user(&op_data, (void __user *)arg, sizeof(op_data)))
            return -EFAULT;

        if (op_data.in_len != AES_KEY_SIZE)
            return -EINVAL;

        if (copy_from_user(ctx->aes_key, op_data.in_data, AES_KEY_SIZE))
            return -EFAULT;

        /* Generate random IV or use provided */
        get_random_bytes(ctx->iv, AES_BLOCK_SIZE);
        ctx->key_set = 1;

        pr_info("crypto_driver: AES key set\n");
        break;

    case IOCTL_GET_IV:
        /* Return current IV */
        if (!ctx->key_set)
            return -EINVAL;

        if (copy_from_user(&op_data, (void __user *)arg, sizeof(op_data)))
            return -EFAULT;

        if (op_data.out_len < AES_BLOCK_SIZE)
            return -EINVAL;

        if (copy_to_user(op_data.out_data, ctx->iv, AES_BLOCK_SIZE))
            return -EFAULT;

        op_data.out_len = AES_BLOCK_SIZE;
        if (copy_to_user((void __user *)arg, &op_data, sizeof(op_data)))
            return -EFAULT;
        break;

    case IOCTL_SET_IV:
        /* Set IV */
        if (copy_from_user(&op_data, (void __user *)arg, sizeof(op_data)))
            return -EFAULT;

        if (op_data.in_len != AES_BLOCK_SIZE)
            return -EINVAL;

        if (copy_from_user(ctx->iv, op_data.in_data, AES_BLOCK_SIZE))
            return -EFAULT;

        pr_info("crypto_driver: IV set\n");
        break;

    case IOCTL_ENCRYPT:
        /* AES encryption */
        if (!ctx->key_set)
            return -EINVAL;

        if (copy_from_user(&op_data, (void __user *)arg, sizeof(op_data)))
            return -EFAULT;

        if (op_data.in_len == 0 || op_data.in_len > CRYPTO_BUFFER_SIZE - AES_BLOCK_SIZE)
            return -EINVAL;

        input = kmalloc(CRYPTO_BUFFER_SIZE, GFP_KERNEL);
        output = kmalloc(CRYPTO_BUFFER_SIZE, GFP_KERNEL);
        if (!input || !output) {
            ret = -ENOMEM;
            goto encrypt_cleanup;
        }

        if (copy_from_user(input, op_data.in_data, op_data.in_len)) {
            ret = -EFAULT;
            goto encrypt_cleanup;
        }

        /* Add PKCS7 padding */
        padded_len = add_pkcs7_padding(input, op_data.in_len, CRYPTO_BUFFER_SIZE);
        if (padded_len == 0) {
            ret = -EINVAL;
            goto encrypt_cleanup;
        }

        ret = do_aes_crypt(ctx->aes_key, ctx->iv, input, padded_len, output, 1);
        if (ret)
            goto encrypt_cleanup;

        if (op_data.out_len < padded_len) {
            ret = -EINVAL;
            goto encrypt_cleanup;
        }

        if (copy_to_user(op_data.out_data, output, padded_len)) {
            ret = -EFAULT;
            goto encrypt_cleanup;
        }

        op_data.out_len = padded_len;
        if (copy_to_user((void __user *)arg, &op_data, sizeof(op_data)))
            ret = -EFAULT;

        ctx->last_operation = 1;

encrypt_cleanup:
        kfree(input);
        kfree(output);
        break;

    case IOCTL_DECRYPT:
        /* AES decryption */
        if (!ctx->key_set)
            return -EINVAL;

        if (copy_from_user(&op_data, (void __user *)arg, sizeof(op_data)))
            return -EFAULT;

        if (op_data.in_len == 0 || op_data.in_len > CRYPTO_BUFFER_SIZE ||
            op_data.in_len % AES_BLOCK_SIZE != 0)
            return -EINVAL;

        input = kmalloc(CRYPTO_BUFFER_SIZE, GFP_KERNEL);
        output = kmalloc(CRYPTO_BUFFER_SIZE, GFP_KERNEL);
        if (!input || !output) {
            ret = -ENOMEM;
            goto decrypt_cleanup;
        }

        if (copy_from_user(input, op_data.in_data, op_data.in_len)) {
            ret = -EFAULT;
            goto decrypt_cleanup;
        }

        ret = do_aes_crypt(ctx->aes_key, ctx->iv, input, op_data.in_len, output, 0);
        if (ret)
            goto decrypt_cleanup;

        /* Remove PKCS7 padding */
        padded_len = remove_pkcs7_padding(output, op_data.in_len);
        if (padded_len == 0) {
            ret = -EINVAL;
            goto decrypt_cleanup;
        }

        if (op_data.out_len < padded_len) {
            ret = -EINVAL;
            goto decrypt_cleanup;
        }

        if (copy_to_user(op_data.out_data, output, padded_len)) {
            ret = -EFAULT;
            goto decrypt_cleanup;
        }

        op_data.out_len = padded_len;
        if (copy_to_user((void __user *)arg, &op_data, sizeof(op_data)))
            ret = -EFAULT;

        ctx->last_operation = 2;

decrypt_cleanup:
        kfree(input);
        kfree(output);
        break;

    case IOCTL_MD5_HASH:
        /* MD5 hashing */
        if (copy_from_user(&op_data, (void __user *)arg, sizeof(op_data)))
            return -EFAULT;

        if (op_data.in_len == 0 || op_data.in_len > CRYPTO_BUFFER_SIZE)
            return -EINVAL;

        if (op_data.out_len < MD5_HASH_SIZE)
            return -EINVAL;

        input = kmalloc(op_data.in_len, GFP_KERNEL);
        output = kmalloc(MD5_HASH_SIZE, GFP_KERNEL);
        if (!input || !output) {
            ret = -ENOMEM;
            goto hash_cleanup;
        }

        if (copy_from_user(input, op_data.in_data, op_data.in_len)) {
            ret = -EFAULT;
            goto hash_cleanup;
        }

        ret = do_md5_hash(input, op_data.in_len, output);
        if (ret)
            goto hash_cleanup;

        if (copy_to_user(op_data.out_data, output, MD5_HASH_SIZE)) {
            ret = -EFAULT;
            goto hash_cleanup;
        }

        op_data.out_len = MD5_HASH_SIZE;
        if (copy_to_user((void __user *)arg, &op_data, sizeof(op_data)))
            ret = -EFAULT;

        ctx->last_operation = 3;

hash_cleanup:
        kfree(input);
        kfree(output);
        break;

    default:
        return -EINVAL;
    }

    return ret;
}

static const struct file_operations crypto_fops = {
    .owner = THIS_MODULE,
    .open = crypto_open,
    .release = crypto_release,
    .read = crypto_read,
    .write = crypto_write,
    .unlocked_ioctl = crypto_ioctl,
};

/*
 * Module Initialization
 */
static int __init crypto_driver_init(void)
{
    dev_t dev;
    int ret;

    pr_info("crypto_driver: Initializing\n");

    /* Allocate major number dynamically */
    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("crypto_driver: Failed to allocate major number\n");
        return ret;
    }
    major_number = MAJOR(dev);

    /* Initialize cdev */
    cdev_init(&crypto_cdev, &crypto_fops);
    crypto_cdev.owner = THIS_MODULE;

    ret = cdev_add(&crypto_cdev, dev, 1);
    if (ret < 0) {
        pr_err("crypto_driver: Failed to add cdev\n");
        goto err_cdev;
    }

    /* Create device class */
    crypto_class = class_create(CLASS_NAME);
    if (IS_ERR(crypto_class)) {
        pr_err("crypto_driver: Failed to create class\n");
        ret = PTR_ERR(crypto_class);
        goto err_class;
    }

    /* Create device */
    crypto_device = device_create(crypto_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(crypto_device)) {
        pr_err("crypto_driver: Failed to create device\n");
        ret = PTR_ERR(crypto_device);
        goto err_device;
    }

    pr_info("crypto_driver: Initialized with major number %d\n", major_number);
    return 0;

err_device:
    class_destroy(crypto_class);
err_class:
    cdev_del(&crypto_cdev);
err_cdev:
    unregister_chrdev_region(dev, 1);
    return ret;
}

/*
 * Module Cleanup
 */
static void __exit crypto_driver_exit(void)
{
    device_destroy(crypto_class, MKDEV(major_number, 0));
    class_destroy(crypto_class);
    cdev_del(&crypto_cdev);
    unregister_chrdev_region(MKDEV(major_number, 0), 1);

    pr_info("crypto_driver: Removed\n");
}

module_init(crypto_driver_init);
module_exit(crypto_driver_exit);
