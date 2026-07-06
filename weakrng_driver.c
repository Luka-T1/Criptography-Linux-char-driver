#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/types.h>

#define DEVICE_NAME "weakrng"
#define CLASS_NAME  "weakrng_class"
#define SEED_BUF_SIZE 64

static int    major_number;
static struct class  *weakrng_class  = NULL;
static struct device *weakrng_device = NULL;

static u64 rng_state;

static u64 weakrng_next_u64(void)
{
    u64 z;

    rng_state += 0x9E3779B97F4A7C15ULL;
    z = rng_state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);

    return z;
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{s
    return 0;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer,
                          size_t len, loff_t *offset)
{
    char seed_text[SEED_BUF_SIZE];
    size_t copy_len;

    if (len < SEED_BUF_SIZE - 1)
        copy_len = len;
    else
        copy_len = SEED_BUF_SIZE - 1;

    if (copy_from_user(seed_text, buffer, copy_len))
        return -EFAULT;

    seed_text[copy_len] = '\0';

    rng_state = simple_strtoull(seed_text, NULL, 0);

    return len;
}

static ssize_t dev_read(struct file *filep, char __user *buffer,
                         size_t len, loff_t *offset)
{
    unsigned char *keystream;
    u64 value = 0;
    size_t i;

    keystream = kmalloc(len, GFP_KERNEL);
    if (!keystream)
        return -ENOMEM;

    for (i = 0; i < len; i++) {
        if (i % 8 == 0)
            value = weakrng_next_u64();

        keystream[i] = (unsigned char)((value >> (8 * (i % 8))) & 0xFF);
    }

    if (copy_to_user(buffer, keystream, len)) {
        kfree(keystream);
        return -EFAULT;
    }

    kfree(keystream);
    return len;
}

