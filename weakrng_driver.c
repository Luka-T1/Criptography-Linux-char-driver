/*
 * weakrng_driver.c
 *
 * Jednostavan Linux character device drajver koji simulira
 * namerno slab (predvidiv) generator pseudoslucajnih brojeva.
 *
 * Ideja projekta:
 *  - Korisnik prvo upise (write) seed u /dev/weakrng
 *  - Nakon toga cita (read) niz bajtova koji izgledaju "slucajno"
 *  - Posto se koristi jednostavan algoritam i poznat seed, napadac
 *    moze u korisnickom prostoru (attacker_decrypt_weak.c) da
 *    izracuna potpuno isti niz bajtova.
 *
 * Namerno je implementirano samo sa osnovne 4 operacije:
 * open, release, read i write - bez ioctl-a i slicnih dodataka,
 * da bi drajver ostao jednostavan i pregledan za citanje.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "weakrng"
#define CLASS_NAME  "weakrng_class"
#define SEED_BUF_SIZE 64

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("Jednostavan drajver koji simulira slab (predvidiv) RNG");

static int    major_number;
static struct class  *weakrng_class  = NULL;
static struct device *weakrng_device = NULL;

/* Interno stanje generatora - isti princip kao u napadacevom kodu */
static u64 rng_state;

/*
 * Generator brojeva - identican algoritam kao weakrng_next_u64()
 * iz attacker_decrypt_weak.c. Bitno je da se ova dva algoritma
 * poklapaju bajt po bajt, inace napadac ne moze da predvidi izlaz.
 */
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
    /* Ne treba nam nikakva posebna priprema pri otvaranju */
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    /* Ni ovde nema nista posebno da se oslobodi */
    return 0;
}

/*
 * write() - korisnik salje seed kao tekstualni broj (npr. "12345").
 * Taj broj postaje pocetno stanje generatora, isto kao sto
 * attacker_decrypt_weak.c postavlja state = seed pre generisanja.
 */
static ssize_t dev_write(struct file *filep, const char __user *buffer,
                          size_t len, loff_t *offset)
{
    char seed_text[SEED_BUF_SIZE];
    size_t copy_len;

    copy_len = (len < SEED_BUF_SIZE - 1) ? len : SEED_BUF_SIZE - 1;

    if (copy_from_user(seed_text, buffer, copy_len))
        return -EFAULT;

    seed_text[copy_len] = '\0';

    /* simple_strtoull parsira decimalni broj iz stringa */
    rng_state = simple_strtoull(seed_text, NULL, 0);

    return len;
}

/*
 * read() - generise trazeni broj bajtova koriscenjem gornjeg
 * generatora i kopira ih korisniku. Redosled bajtova (8 po pozivu,
 * little-endian) mora da odgovara generate_keystream() kod napadaca.
 */
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

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

static int __init weakrng_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "weakrng: registracija drajvera nije uspela\n");
        return major_number;
    }

    weakrng_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(weakrng_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(weakrng_class);
    }

    weakrng_device = device_create(weakrng_class, NULL,
                                    MKDEV(major_number, 0),
                                    NULL, DEVICE_NAME);
    if (IS_ERR(weakrng_device)) {
        class_destroy(weakrng_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(weakrng_device);
    }

    rng_state = 0;

    printk(KERN_INFO "weakrng: drajver ucitan, major broj = %d\n", major_number);
    return 0;
}

static void __exit weakrng_exit(void)
{
    device_destroy(weakrng_class, MKDEV(major_number, 0));
    class_destroy(weakrng_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "weakrng: drajver uklonjen\n");
}

module_init(weakrng_init);
module_exit(weakrng_exit);
