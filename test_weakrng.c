/*
 * test_weakrng.c
 *
 * Program koji dokazuje da je weakrng zaista predvidiv.
 *
 * Postupak:
 *  1. Otvara /dev/weakrng, upisuje seed, cita "prave" bajtove
 *     direktno od drajvera (kernel strana).
 *  2. Nezavisno racuna "predvidjene" bajtove istim algoritmom
 *     u korisnickom prostoru (userspace).
 *  3. Poredi ta dva niza. Ako su identicni, dokazano je da
 *     napadac moze unapred da zna izlaz RNG-a.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/weakrng"

static uint64_t state;

/* Isti generator kao u weakrng_driver.c i predict_weakrng.c */
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

/* Cita "count" bajtova direktno iz weakrng drajvera */
static int read_real_bytes(uint64_t seed, unsigned char *out, int count)
{
    int fd;
    char seed_text[32];
    int total, r;

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open /dev/weakrng");
        return -1;
    }

    snprintf(seed_text, sizeof(seed_text), "%llu", (unsigned long long)seed);

    if (write(fd, seed_text, strlen(seed_text)) < 0) {
        perror("write seed");
        close(fd);
        return -1;
    }

    total = 0;
    while (total < count) {
        r = read(fd, out + total, count - total);
        if (r <= 0) {
            perror("read /dev/weakrng");
            close(fd);
            return -1;
        }
        total += r;
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    uint64_t seed;
    int count;
    unsigned char *real_bytes;
    unsigned char *predicted_bytes;
    int i, mismatch;

    if (argc != 3) {
        printf("Upotreba:\n");
        printf("%s <seed> <broj_bajtova>\n", argv[0]);
        printf("\nPrimer:\n");
        printf("%s 12345 16\n", argv[0]);
        return 1;
    }

    seed = strtoull(argv[1], NULL, 0);
    count = atoi(argv[2]);

    if (count <= 0) {
        printf("Greska: broj bajtova mora biti pozitivan.\n");
        return 1;
    }

    real_bytes = malloc(count);
    predicted_bytes = malloc(count);

    if (real_bytes == NULL || predicted_bytes == NULL) {
        printf("Greska: malloc nije uspeo.\n");
        return 1;
    }

    if (read_real_bytes(seed, real_bytes, count) != 0) {
        free(real_bytes);
        free(predicted_bytes);
        return 1;
    }

    predict_bytes(seed, predicted_bytes, count);

    printf("=== Test weakrng predvidivosti ===\n");
    printf("Seed: %llu\n", (unsigned long long)seed);

    printf("Pravi izlaz drajvera:   ");
    for (i = 0; i < count; i++) {
        printf("%02X", real_bytes[i]);
    }
    printf("\n");

    printf("Predvidjeni izlaz:      ");
    for (i = 0; i < count; i++) {
        printf("%02X", predicted_bytes[i]);
    }
    printf("\n");

    mismatch = 0;
    for (i = 0; i < count; i++) {
        if (real_bytes[i] != predicted_bytes[i]) {
            mismatch++;
        }
    }

    printf("\nRezultat: ");
    if (mismatch == 0) {
        printf("USPESNO - svi bajtovi su tacno predvidjeni (%d/%d)\n", count, count);
    } else {
        printf("NEUSPESNO - %d od %d bajtova se ne poklapa\n", mismatch, count);
    }

    free(real_bytes);
    free(predicted_bytes);
    return 0;
}
