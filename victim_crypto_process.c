/*
 * Ovo je proces zrtve - obican program koji zeli da sifruje
 * poruku. On ne zna i ne mora da zna da li trenutno koristi
 * bezbedan ili napadnut RNG - samo cita podesavanja iz crypto.conf
 * i radi ono sto pise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/evp.h>

#define CONFIG_FILE "crypto.conf"
#define MAX_TEXT 1024
#define MAX_LINE 256
#define KEY_SIZE 32
#define IV_SIZE 12
#define TAG_SIZE 16

typedef struct {
    char algorithm[64];
    char rng_path[128];
    char seed[64];
} CryptoConfig;

/* Ispisuje niz bajtova u hex formatu */
void print_hex(const char *label, const unsigned char *data, int len)
{
    int i;

    printf("%s", label);
    for (i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

/* Ucitava podesavanja iz crypto.conf, ili koristi podrazumevane vrednosti */
void load_config(CryptoConfig *cfg)
{
    FILE *file;
    char line[MAX_LINE];

    strcpy(cfg->algorithm, "AES_GCM");
    strcpy(cfg->rng_path, "/dev/urandom");
    strcpy(cfg->seed, "12345");

    file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        printf("Upozorenje: crypto.conf nije pronadjen, koristim default AES_GCM.\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (strncmp(line, "algorithm=", 10) == 0) {
            strncpy(cfg->algorithm, line + 10, sizeof(cfg->algorithm) - 1);
        } else if (strncmp(line, "rng=", 4) == 0) {
            strncpy(cfg->rng_path, line + 4, sizeof(cfg->rng_path) - 1);
        } else if (strncmp(line, "seed=", 5) == 0) {
            strncpy(cfg->seed, line + 5, sizeof(cfg->seed) - 1);
        }
    }

    fclose(file);
}

/* Cita "len" bajtova iz zadatog RNG uredjaja */
int get_random_bytes(const CryptoConfig *cfg, unsigned char *buffer, int len)
{
    int fd;
    int total;
    int r;

    fd = open(cfg->rng_path, O_RDWR);
    if (fd < 0) {
        perror("open rng");
        return -1;
    }

    if (strcmp(cfg->rng_path, "/dev/weakrng") == 0) {
        write(fd, cfg->seed, strlen(cfg->seed));
    }

    total = 0;
    while (total < len) {
        r = read(fd, buffer + total, len - total);
        if (r <= 0) {
            perror("read rng");
            close(fd);
            return -1;
        }
        total += r;
    }

    close(fd);
    return 0;
}

/* Sifrovanje AES-256-GCM algoritmom, koristeci OpenSSL biblioteku */
int aes_gcm_encrypt(const unsigned char *plaintext, int plaintext_len,
                     const unsigned char *key, const unsigned char *iv,
                     unsigned char *ciphertext, unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        return -1;
    }

    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);

    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag);

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

/* "Sifrovanje" prostim XOR-om sa slabim keystream-om */
void weak_stream_encrypt(const CryptoConfig *cfg, const unsigned char *plaintext,
                          int plaintext_len, unsigned char *ciphertext)
{
    unsigned char *keystream;
    int i;

    keystream = malloc(plaintext_len);
    if (keystream == NULL) {
        printf("Greska: malloc nije uspeo. \n");
        exit(1);
    }

    if (get_random_bytes(cfg, keystream, plaintext_len) != 0) {
        printf("Greska: ne mogu da citam iz weakrng. \n");
        free(keystream);
        exit(1);
    }

    for (i = 0; i < plaintext_len; i++) {
        ciphertext[i] = plaintext[i] ^ keystream[i];
    }

    free(keystream);
}

int main(int argc, char *argv[])
{
    CryptoConfig cfg;
    unsigned char plaintext[MAX_TEXT];
    unsigned char ciphertext[MAX_TEXT];
    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char tag[TAG_SIZE];
    int plaintext_len;
    int ciphertext_len;

    if (argc != 2) {
        printf("Upotreba: %s \"poruka\"\n", argv[0]);
        return 1;
    }

    load_config(&cfg);

    strncpy((char *)plaintext, argv[1], MAX_TEXT - 1);
    plaintext[MAX_TEXT - 1] = '\0';
    plaintext_len = strlen((char *)plaintext);

    printf(" Victim crypto process  \n");
    printf("Algorithm: %s \n", cfg.algorithm);
    printf("RNG: %s \n", cfg.rng_path);
    printf("Plaintext: %s \n\n", plaintext);

    if (strcmp(cfg.algorithm, "AES_GCM") == 0) {
        get_random_bytes(&cfg, key, KEY_SIZE);
        get_random_bytes(&cfg, iv, IV_SIZE);

        ciphertext_len = aes_gcm_encrypt(plaintext, plaintext_len, key, iv, ciphertext, tag);
        if (ciphertext_len < 0) {
            printf("AES-GCM encryption failed. \n");
            return 1;
        }

        printf("MODE=AES_GCM \n");
        print_hex("IV=", iv, IV_SIZE);
        print_hex("TAG=", tag, TAG_SIZE);
        print_hex("CIPHERTEXT=", ciphertext, ciphertext_len);

        printf("Objasnjenje: \n");
        printf("Normalan rezim: proveren algoritam AES-GCM i sistemski RNG. \n");
    } else if (strcmp(cfg.algorithm, "WEAK_STREAM") == 0) {
        weak_stream_encrypt(&cfg, plaintext, plaintext_len, ciphertext);

        printf("MODE=WEAK_STREAM \n");
        printf("SEED=%s \n", cfg.seed);
        print_hex("CIPHERTEXT=", ciphertext, plaintext_len);

        printf("Objasnjenje: \n");
        printf("Napadnut rezim: algoritam je zamenjen slabim WEAK_STREAM rezimom. \n");
        printf("Ciphertext postoji, ali je niz predvidiv ako se zna seed. \n");
    } else {
        printf("Nepoznat algoritam u crypto.conf: %s \n", cfg.algorithm);
        return 1;
    }

    return 0;
}
