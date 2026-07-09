/*
 * Program koji predvidja izlaz weakrng drajvera.
 * Ako znamo seed i algoritam, mozemo unapred da izracunamo
 * potpuno isti niz bajtova koji ce drajver da vrati.
 *
 * Ovaj isti algoritam mora da bude identican onom u
 * weakrng_driver.c da bi predikcija bila tacna.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

/* Generise "count" bajtova pocevsi od datog seeda */
static void predict_bytes(uint64_t seed, unsigned char *out, int count)
{
    uint64_t value = 0;
    int i;

    state = seed;

    for (i = 0; i < count; i++) {
        if (i % 8 == 0) {
            value = weakrng_next_u64();
        }
        out[i] = (unsigned char)((value >> (8 * (i % 8))) & 0xFF);
    }
}

int main(int argc, char *argv[])
{
    uint64_t seed;
    int count;
    unsigned char *keystream;
    int i;

    if (argc != 3) {
        printf("Upotreba: \n");
        printf("%s <seed> <broj_bajtova>\n", argv[0]);
        printf("Primer: \n");
        printf("%s 12345 16 \n", argv[0]);
        return 1;
    }

    seed = strtoull(argv[1], NULL, 0);
    count = atoi(argv[2]);

    if (count <= 0) {
        printf("Greska: broj bajtova mora biti pozitivan. \n");
        return 1;
    }

    keystream = malloc(count);
    if (keystream == NULL) {
        printf("Greska: malloc nije uspeo.\n");
        return 1;
    }

    predict_bytes(seed, keystream, count);

    printf(" Predikcija weakrng izlaza \n");
    printf("Seed: %llu \n", (unsigned long long)seed);

    printf("Bajtovi: ");
    for (i = 0; i < count; i++) {
        printf("%02X", keystream[i]);
    }
    printf("\n");

    printf("Cifre: ");
    for (i = 0; i < count; i++) {
        printf("%d", keystream[i] % 10);
    }
    printf("\n");

    printf("Objasnjenje: \n");
    printf("Ovi bajtovi su izracunati bez ikakvog pristupa drajveru, samo na osnovu poznatog seeda i algoritma. \n");

    free(keystream);
    return 0;
}
