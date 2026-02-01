/*
 * crypto_driver.c - Enhanced Linux Character Device Driver for Crypto Operations
 * Provides AES-CBC encryption/decryption and MD5 hashing using Kernel Crypto API
 * 
 * Enhancements over v1.0:
 * - AES-128-CBC mode with initialization vector (IV) support (was ECB)
 * - Separate IOCTL commands for setting key and IV
 * - Per-file session context for better security
 * - Random IV generation
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
#include <linux/random.h>

#define DEVICE_NAME "crypto_dev"
#define CLASS_NAME "crypto"

/* IOCTL magic number */
#define CRYPTO_IOC_MAGIC 'c'

/* Maximum data sizes */
#define MAX_DATA_SIZE 4096
#define AES_KEY_SIZE 16
#define AES_IV_SIZE 16
#define MD5_DIGEST_SIZE 16

/* Data structure for IOCTL */
struct crypto_data {
    unsigned char input[MAX_DATA_SIZE];
    unsigned char output[MAX_DATA_SIZE];
    unsigned char key[AES_KEY_SIZE];
    unsigned char iv[AES_IV_SIZE];
    unsigned int input_len;
    unsigned int output_len;
};

/* IOCTL commands - Enhanced with key/IV management */
#define IOCTL_SET_KEY      _IOW(CRYPTO_IOC_MAGIC, 1, unsigned char[AES_KEY_SIZE])
#define IOCTL_SET_IV       _IOW(CRYPTO_IOC_MAGIC, 2, unsigned char[AES_IV_SIZE])
#define IOCTL_GET_IV       _IOR(CRYPTO_IOC_MAGIC, 3, unsigned char[AES_IV_SIZE])
#define IOCTL_ENCRYPT      _IOWR(CRYPTO_IOC_MAGIC, 4, struct crypto_data)
#define IOCTL_DECRYPT      _IOWR(CRYPTO_IOC_MAGIC, 5, struct crypto_data)
#define IOCTL_MD5_HASH     _IOWR(CRYPTO_IOC_MAGIC, 6, struct crypto_data)

/* Per-file context for session management */
struct crypto_session {
    unsigned char aes_key[AES_KEY_SIZE];
    unsigned char iv[AES_IV_SIZE];
    int key_set;
    int iv_set;
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chat System Enhanced");
MODULE_DESCRIPTION("Enhanced Crypto Character Device Driver with AES-CBC and MD5");
MODULE_VERSION("2.0");

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

/* AES-CBC encryption - Enhanced with CBC mode and IV */
static int aes_cbc_encrypt(const unsigned char *plaintext, unsigned int len,
                          const unsigned char *key, const unsigned char *iv,
                          unsigned char *ciphertext)
{
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct scatterlist sg_in, sg_out;
    unsigned char *iv_copy;
    int ret;

    tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        printk(KERN_ERR "crypto_dev: Failed to allocate AES-CBC transform\n");
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

    iv_copy = kmalloc(AES_IV_SIZE, GFP_KERNEL);
    if (!iv_copy) {
        ret = -ENOMEM;
        goto out;
    }
    memcpy(iv_copy, iv, AES_IV_SIZE);

    sg_init_one(&sg_in, plaintext, len);
    sg_init_one(&sg_out, ciphertext, len);

    skcipher_request_set_crypt(req, &sg_in, &sg_out, len, iv_copy);

    ret = crypto_skcipher_encrypt(req);
    if (ret) {
        printk(KERN_ERR "crypto_dev: AES-CBC encryption failed\n");
    }

    kfree(iv_copy);
out:
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);
    return ret;
}

/* AES-CBC decryption */
static int aes_cbc_decrypt(const unsigned char *ciphertext, unsigned int len,
                          const unsigned char *key, const unsigned char *iv,
                          unsigned char *plaintext)
{
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct scatterlist sg_in, sg_out;
    unsigned char *iv_copy;
    int ret;

    tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        printk(KERN_ERR "crypto_dev: Failed to allocate AES-CBC transform\n");
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

    iv_copy = kmalloc(AES_IV_SIZE, GFP_KERNEL);
    if (!iv_copy) {
        ret = -ENOMEM;
        goto out;
    }
    memcpy(iv_copy, iv, AES_IV_SIZE);

    sg_init_one(&sg_in, ciphertext, len);
    sg_init_one(&sg_out, plaintext, len);

    skcipher_request_set_crypt(req, &sg_in, &sg_out, len, iv_copy);

    ret = crypto_skcipher_decrypt(req);
    if (ret) {
        printk(KERN_ERR "crypto_dev: AES-CBC decryption failed\n");
    }

    kfree(iv_copy);
out:
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);
    return ret;
}

/* Device open - Create session context */
static int device_open(struct inode *inodep, struct file *filep)
{
    struct crypto_session *session;
    
    session = kmalloc(sizeof(struct crypto_session), GFP_KERNEL);
    if (!session) {
        return -ENOMEM;
    }
    
    memset(session, 0, sizeof(struct crypto_session));
    session->key_set = 0;
    session->iv_set = 0;
    
    filep->private_data = session;
    
    printk(KERN_INFO "crypto_dev: Device opened, session created\n");
    return 0;
}

/* Device release - Free session context */
static int device_release(struct inode *inodep, struct file *filep)
{
    struct crypto_session *session = filep->private_data;
    
    if (session) {
        memset(session, 0, sizeof(struct crypto_session));
        kfree(session);
        filep->private_data = NULL;
    }
    
    printk(KERN_INFO "crypto_dev: Device closed, session destroyed\n");
    return 0;
}

/* IOCTL handler - Enhanced with new commands */
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct crypto_session *session = file->private_data;
    struct crypto_data *kdata;
    unsigned char key_buf[AES_KEY_SIZE];
    unsigned char iv_buf[AES_IV_SIZE];
    int ret = 0;

    if (!session) {
        return -EINVAL;
    }

    switch (cmd) {
    case IOCTL_SET_KEY:
        if (copy_from_user(session->aes_key, (void __user *)arg, AES_KEY_SIZE)) {
            return -EFAULT;
        }
        session->key_set = 1;
        break;

    case IOCTL_SET_IV:
        if (copy_from_user(session->iv, (void __user *)arg, AES_IV_SIZE)) {
            return -EFAULT;
        }
        session->iv_set = 1;
        break;

    case IOCTL_GET_IV:
        if (!session->iv_set) {
            get_random_bytes(session->iv, AES_IV_SIZE);
            session->iv_set = 1;
        }
        if (copy_to_user((void __user *)arg, session->iv, AES_IV_SIZE)) {
            return -EFAULT;
        }
        break;

    case IOCTL_MD5_HASH:
        kdata = kmalloc(sizeof(struct crypto_data), GFP_KERNEL);
        if (!kdata) {
            return -ENOMEM;
        }

        if (copy_from_user(kdata, (void __user *)arg, sizeof(struct crypto_data))) {
            kfree(kdata);
            return -EFAULT;
        }

        ret = compute_md5(kdata->input, kdata->input_len, kdata->output);
        if (!ret) {
            kdata->output_len = MD5_DIGEST_SIZE;
            if (copy_to_user((void __user *)arg, kdata, sizeof(struct crypto_data))) {
                ret = -EFAULT;
            }
        }
        kfree(kdata);
        break;

    case IOCTL_ENCRYPT:
        if (!session->key_set) {
            return -EINVAL;
        }

        kdata = kmalloc(sizeof(struct crypto_data), GFP_KERNEL);
        if (!kdata) {
            return -ENOMEM;
        }

        if (copy_from_user(kdata, (void __user *)arg, sizeof(struct crypto_data))) {
            kfree(kdata);
            return -EFAULT;
        }

        if (kdata->iv[0] != 0 || kdata->iv[1] != 0) {
            memcpy(iv_buf, kdata->iv, AES_IV_SIZE);
        } else if (session->iv_set) {
            memcpy(iv_buf, session->iv, AES_IV_SIZE);
            memcpy(kdata->iv, session->iv, AES_IV_SIZE);
        } else {
            get_random_bytes(iv_buf, AES_IV_SIZE);
            memcpy(kdata->iv, iv_buf, AES_IV_SIZE);
            memcpy(session->iv, iv_buf, AES_IV_SIZE);
            session->iv_set = 1;
        }

        if (kdata->key[0] != 0 || kdata->key[1] != 0) {
            memcpy(key_buf, kdata->key, AES_KEY_SIZE);
        } else {
            memcpy(key_buf, session->aes_key, AES_KEY_SIZE);
        }

        kdata->output_len = ((kdata->input_len + 15) / 16) * 16;
        ret = aes_cbc_encrypt(kdata->input, kdata->output_len, key_buf, iv_buf, kdata->output);
        
        if (!ret) {
            if (copy_to_user((void __user *)arg, kdata, sizeof(struct crypto_data))) {
                ret = -EFAULT;
            }
        }
        kfree(kdata);
        break;

    case IOCTL_DECRYPT:
        if (!session->key_set) {
            return -EINVAL;
        }

        kdata = kmalloc(sizeof(struct crypto_data), GFP_KERNEL);
        if (!kdata) {
            return -ENOMEM;
        }

        if (copy_from_user(kdata, (void __user *)arg, sizeof(struct crypto_data))) {
            kfree(kdata);
            return -EFAULT;
        }

        if (kdata->iv[0] != 0 || kdata->iv[1] != 0) {
            memcpy(iv_buf, kdata->iv, AES_IV_SIZE);
        } else if (session->iv_set) {
            memcpy(iv_buf, session->iv, AES_IV_SIZE);
        } else {
            kfree(kdata);
            return -EINVAL;
        }

        if (kdata->key[0] != 0 || kdata->key[1] != 0) {
            memcpy(key_buf, kdata->key, AES_KEY_SIZE);
        } else {
            memcpy(key_buf, session->aes_key, AES_KEY_SIZE);
        }

        ret = aes_cbc_decrypt(kdata->input, kdata->input_len, key_buf, iv_buf, kdata->output);
        if (!ret) {
            kdata->output_len = kdata->input_len;
            if (copy_to_user((void __user *)arg, kdata, sizeof(struct crypto_data))) {
                ret = -EFAULT;
            }
        }
        kfree(kdata);
        break;

    default:
        ret = -EINVAL;
    }

    return ret;
}

/* Module initialization */
static int __init crypto_driver_init(void)
{
    printk(KERN_INFO "crypto_dev: Initializing enhanced crypto driver v2.0\n");

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "crypto_dev: Failed to register major number\n");
        return major_number;
    }

    crypto_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(crypto_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(crypto_class);
    }

    crypto_device = device_create(crypto_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(crypto_device)) {
        class_destroy(crypto_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(crypto_device);
    }
    
    printk(KERN_INFO "crypto_dev: Device /dev/%s created (AES-128-CBC + MD5)\n", DEVICE_NAME);
    return 0;
}

/* Module cleanup */
static void __exit crypto_driver_exit(void)
{
    device_destroy(crypto_class, MKDEV(major_number, 0));
    class_unregister(crypto_class);
    class_destroy(crypto_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "crypto_dev: Enhanced driver v2.0 unloaded\n");
}

module_init(crypto_driver_init);
module_exit(crypto_driver_exit);
