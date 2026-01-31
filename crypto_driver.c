/*
 * crypto_driver.c - Linux Character Device Driver for Crypto Operations
 * Provides AES encryption/decryption and MD5 hashing using Kernel Crypto API
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <crypto/hash.h>
#include <crypto/skcipher.h>
#include <linux/scatterlist.h>
#include <linux/string.h>

#define DEVICE_NAME "crypto_dev"
#define CLASS_NAME "crypto"

/* IOCTL commands */
#define CRYPTO_IOC_MAGIC 'c'
#define CRYPTO_MD5_HASH _IOWR(CRYPTO_IOC_MAGIC, 1, struct crypto_data)
#define CRYPTO_AES_ENCRYPT _IOWR(CRYPTO_IOC_MAGIC, 2, struct crypto_data)
#define CRYPTO_AES_DECRYPT _IOWR(CRYPTO_IOC_MAGIC, 3, struct crypto_data)

#define MAX_DATA_SIZE 4096
#define AES_KEY_SIZE 16
#define MD5_DIGEST_SIZE 16

/* Data structure for IOCTL */
struct crypto_data {
    unsigned char input[MAX_DATA_SIZE];
    unsigned char output[MAX_DATA_SIZE];
    unsigned char key[AES_KEY_SIZE];
    unsigned int input_len;
    unsigned int output_len;
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chat System");
MODULE_DESCRIPTION("Crypto Character Device Driver for AES and MD5");
MODULE_VERSION("1.0");

static int major_number;
static struct class *crypto_class = NULL;
static struct device *crypto_device = NULL;

/* Function prototypes */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static long device_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

/* MD5 hashing function */
static int compute_md5(const unsigned char *data, unsigned int len, unsigned char *hash)
{
    struct crypto_shash *tfm;
    struct shash_desc *desc;
    int ret;

    tfm = crypto_alloc_shash("md5", 0, 0);
    if (IS_ERR(tfm)) {
        printk(KERN_ERR "crypto_dev: Failed to allocate MD5 transform\n");
        return PTR_ERR(tfm);
    }

    desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!desc) {
        crypto_free_shash(tfm);
        return -ENOMEM;
    }

    desc->tfm = tfm;

    ret = crypto_shash_init(desc);
    if (ret) {
        printk(KERN_ERR "crypto_dev: MD5 init failed\n");
        goto out;
    }

    ret = crypto_shash_update(desc, data, len);
    if (ret) {
        printk(KERN_ERR "crypto_dev: MD5 update failed\n");
        goto out;
    }

    ret = crypto_shash_final(desc, hash);
    if (ret) {
        printk(KERN_ERR "crypto_dev: MD5 final failed\n");
    }

out:
    kfree(desc);
    crypto_free_shash(tfm);
    return ret;
}

/* AES encryption function
 * WARNING: This implementation uses ECB mode for simplicity.
 * ECB mode is NOT SECURE for production use because:
 * - Identical plaintext blocks produce identical ciphertext blocks
 * - Patterns in the plaintext are visible in the ciphertext
 * - No initialization vector means deterministic encryption
 * 
 * For production systems, use CBC, CTR, or GCM mode with proper IV.
 */
static int aes_encrypt(const unsigned char *plaintext, unsigned int len,
                      const unsigned char *key, unsigned char *ciphertext)
{
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct scatterlist sg_in, sg_out;
    int ret;

    tfm = crypto_alloc_skcipher("ecb(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        printk(KERN_ERR "crypto_dev: Failed to allocate AES transform\n");
        return PTR_ERR(tfm);
    }

    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        crypto_free_skcipher(tfm);
        return -ENOMEM;
    }

    ret = crypto_skcipher_setkey(tfm, key, AES_KEY_SIZE);
    if (ret) {
        printk(KERN_ERR "crypto_dev: Failed to set AES key\n");
        goto out;
    }

    sg_init_one(&sg_in, plaintext, len);
    sg_init_one(&sg_out, ciphertext, len);

    /* Note: ECB mode doesn't use IV, NULL is passed */
    skcipher_request_set_crypt(req, &sg_in, &sg_out, len, NULL);

    ret = crypto_skcipher_encrypt(req);
    if (ret) {
        printk(KERN_ERR "crypto_dev: AES encryption failed\n");
    }

out:
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);
    return ret;
}

/* AES decryption function
 * WARNING: This implementation uses ECB mode for simplicity.
 * See aes_encrypt() for security warnings about ECB mode.
 */
static int aes_decrypt(const unsigned char *ciphertext, unsigned int len,
                      const unsigned char *key, unsigned char *plaintext)
{
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct scatterlist sg_in, sg_out;
    int ret;

    tfm = crypto_alloc_skcipher("ecb(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        printk(KERN_ERR "crypto_dev: Failed to allocate AES transform\n");
        return PTR_ERR(tfm);
    }

    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        crypto_free_skcipher(tfm);
        return -ENOMEM;
    }

    ret = crypto_skcipher_setkey(tfm, key, AES_KEY_SIZE);
    if (ret) {
        printk(KERN_ERR "crypto_dev: Failed to set AES key\n");
        goto out;
    }

    sg_init_one(&sg_in, ciphertext, len);
    sg_init_one(&sg_out, plaintext, len);

    /* Note: ECB mode doesn't use IV, NULL is passed */
    skcipher_request_set_crypt(req, &sg_in, &sg_out, len, NULL);

    ret = crypto_skcipher_decrypt(req);
    if (ret) {
        printk(KERN_ERR "crypto_dev: AES decryption failed\n");
    }

out:
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);
    return ret;
}

/* Device open */
static int device_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "crypto_dev: Device opened\n");
    return 0;
}

/* Device release */
static int device_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "crypto_dev: Device closed\n");
    return 0;
}

/* IOCTL handler */
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct crypto_data *kdata;
    struct crypto_data __user *udata = (struct crypto_data __user *)arg;
    int ret = 0;

    kdata = kmalloc(sizeof(struct crypto_data), GFP_KERNEL);
    if (!kdata) {
        return -ENOMEM;
    }

    if (copy_from_user(kdata, udata, sizeof(struct crypto_data))) {
        kfree(kdata);
        return -EFAULT;
    }

    switch (cmd) {
    case CRYPTO_MD5_HASH:
        printk(KERN_INFO "crypto_dev: MD5 hash request\n");
        ret = compute_md5(kdata->input, kdata->input_len, kdata->output);
        if (!ret) {
            kdata->output_len = MD5_DIGEST_SIZE;
        }
        break;

    case CRYPTO_AES_ENCRYPT:
        printk(KERN_INFO "crypto_dev: AES encrypt request\n");
        /* Pad length to multiple of 16 bytes */
        kdata->output_len = ((kdata->input_len + 15) / 16) * 16;
        ret = aes_encrypt(kdata->input, kdata->output_len, kdata->key, kdata->output);
        break;

    case CRYPTO_AES_DECRYPT:
        printk(KERN_INFO "crypto_dev: AES decrypt request\n");
        ret = aes_decrypt(kdata->input, kdata->input_len, kdata->key, kdata->output);
        if (!ret) {
            kdata->output_len = kdata->input_len;
        }
        break;

    default:
        printk(KERN_ERR "crypto_dev: Invalid IOCTL command\n");
        ret = -EINVAL;
        goto out;
    }

    if (!ret) {
        if (copy_to_user(udata, kdata, sizeof(struct crypto_data))) {
            ret = -EFAULT;
        }
    }

out:
    kfree(kdata);
    return ret;
}

/* Module initialization */
static int __init crypto_driver_init(void)
{
    printk(KERN_INFO "crypto_dev: Initializing crypto driver\n");

    /* Register character device */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "crypto_dev: Failed to register major number\n");
        return major_number;
    }
    printk(KERN_INFO "crypto_dev: Registered with major number %d\n", major_number);

    /* Register device class */
    crypto_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(crypto_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "crypto_dev: Failed to register device class\n");
        return PTR_ERR(crypto_class);
    }
    printk(KERN_INFO "crypto_dev: Device class registered\n");

    /* Register device driver */
    crypto_device = device_create(crypto_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(crypto_device)) {
        class_destroy(crypto_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "crypto_dev: Failed to create device\n");
        return PTR_ERR(crypto_device);
    }
    printk(KERN_INFO "crypto_dev: Device created successfully\n");

    return 0;
}

/* Module cleanup */
static void __exit crypto_driver_exit(void)
{
    device_destroy(crypto_class, MKDEV(major_number, 0));
    class_unregister(crypto_class);
    class_destroy(crypto_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "crypto_dev: Driver unloaded\n");
}

module_init(crypto_driver_init);
module_exit(crypto_driver_exit);
