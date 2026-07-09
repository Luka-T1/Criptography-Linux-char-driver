/*
 * Ovaj program proverava da li izlaz weakrng drajvera izgleda
 * slucajno, iako u stvari nije. Cita veliki broj bajtova, svaki
 * bajt pretvara u cifru i broji koliko puta se pojavila svaka cifra od 0 do 9.
 *
 * Ako je algoritam dobar generator, cifre
 * bi trebalo da budu skoro ravnomerno rasporedjene - to je poenta
 * ovog testa: dokazuje da statisticki izgleda slucajno, iako je
 * napadac moze tacno predvideti.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/weakrng"
#define DEFAULT_COUNT 100000

int main(int argc, char *argv[])
{
    int fd;
    char seed_text[32];
    unsigned char byte;
    int digit;
    int count;
    int i;
    long histogram[10];
    unsigned long long seed;

    if (argc != 2 && argc != 3) {
        printf("Upotreba:\n");
        printf("%s <seed> [broj_uzoraka]\n", argv[0]);
        printf("\nPrimer:\n");
        printf("%s 12345 100000\n", argv[0]);
        return 1;
    }

    seed = strtoull(argv[1], NULL, 0);
    if (argc == 3) {
        count = atoi(argv[2]);
    } else {
        count = DEFAULT_COUNT;
    }

    if (count <= 0) {
        printf("Greska: broj uzoraka mora biti pozitivan. \n");
        return 1;
    }

    for (i = 0; i < 10; i++) {
        histogram[i] = 0;
    }

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open /dev/weakrng");
        return 1;
    }

    snprintf(seed_text, sizeof(seed_text), "%llu", seed);
    if (write(fd, seed_text, strlen(seed_text)) < 0) {
        perror("write seed");
        close(fd);
        return 1;
    }

    for (i = 0; i < count; i++) {
        if (read(fd, &byte, 1) != 1) {
            perror("read /dev/weakrng");
            close(fd);
            return 1;
        }

        digit = byte % 10;
        histogram[digit]++;
    }

    close(fd);

    printf(" Frequency test \n");
    printf("Seed: %llu\n", seed);
    printf("Broj uzoraka: %d\n\n", count);

    printf("Cifra | Broj pojavljivanja | Procenat \n");
    printf("------------------------------------- \n");
    for (i = 0; i < 10; i++) {
        double percent = (100.0 * histogram[i]) / count;
        printf("  %d   |       %8ld       |  %5.2f%%\n", i, histogram[i], percent);
    }

    printf("Objasnjenje:\n");
    printf("Ocekivano je da svaka cifra ima udeo blizu 10 %%.\n");
    printf("Ako su procenti blizu ravnomerni, izlaz izgleda slucajno, iako je zapravo u potpunosti predvidiv.\n");

    return 0;
}
