/*
 * attacker_decrypt_weak.c
 *
 * Ovo je proces "napadaca". On zna dve stvari koje ne bi trebalo
 * da zna: algoritam koji se koristi (WEAK_STREAM) i seed koji
 * je upotrebljen. Sa tim znanjem moze da izracuna potpuno isti
 * keystream kao zrtva i da dekriptuje ciphertext.
 *
 * Ovo dokazuje zasto je vazno koristiti kriptografski siguran
 * generator slucajnih brojeva (npr. /dev/urandom), a ne
 * jednostavan, predvidiv algoritam.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_DATA 2048

static uint64_t state;

/* Isti generator kao u weakrng_driver.c */
static uint64_t weakrng_next_u64(void)
{
    uint64_t z;

    state += 0x9E3779B97F4A7C15ULL;
    z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);

    return z;
}

/* Pretvara jedan hex karakter u broj 0-15 */
int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/* Pretvara hex string u niz bajtova */
int parse_hex(const char *hex, unsigned char *out)
{
    int len = strlen(hex);
    int i;

    if (len % 2 != 0) {
        return -1;
    }

    for (i = 0; i < len; i += 2) {
        int high = hex_value(hex[i]);
        int low = hex_value(hex[i + 1]);

        if (high < 0 || low < 0) {
            return -1;
        }

        out[i / 2] = (unsigned char)((high << 4) | low);
    }

    return len / 2;
}

/* Generise keystream na osnovu seeda - identicno kao kod zrtve */
void generate_keystream(uint64_t seed, unsigned char *keystream, int len)
{
    uint64_t value = 0;
    int i;

    state = seed;

    for (i = 0; i < len; i++) {
        if (i % 8 == 0) {
            value = weakrng_next_u64();
        }
        keystream[i] = (value >> (8 * (i % 8))) & 0xFF;
    }
}

int main(int argc, char *argv[])
{
    uint64_t seed;
    unsigned char ciphertext[MAX_DATA];
    unsigned char keystream[MAX_DATA];
    unsigned char plaintext[MAX_DATA];
    int ciphertext_len;
    int i;

    if (argc != 3) {
        printf("Upotreba:\n");
        printf("%s <seed> <ciphertext_hex>\n", argv[0]);
        printf("\nPrimer:\n");
        printf("%s 12345 AABBCC\n", argv[0]);
        return 1;
    }

    seed = strtoull(argv[1], NULL, 0);
    ciphertext_len = parse_hex(argv[2], ciphertext);

    if (ciphertext_len < 0) {
        printf("Greska: ciphertext mora biti validan hex string.\n");
        return 1;
    }

    generate_keystream(seed, keystream, ciphertext_len);

    for (i = 0; i < ciphertext_len; i++) {
        plaintext[i] = ciphertext[i] ^ keystream[i];
    }
    plaintext[ciphertext_len] = '\0';

    printf("=== Attacker decrypt ===\n");
    printf("Seed: %llu\n", (unsigned long long)seed);
    printf("Predicted plaintext:\n%s\n", plaintext);

    printf("\nObjasnjenje:\n");
    printf("Posto je algoritam WEAK_STREAM, a napadac zna seed,\n");
    printf("moze da rekonstruise isti keystream i dekriptuje poruku.\n");

    return 0;
}
