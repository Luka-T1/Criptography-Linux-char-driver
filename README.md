# WeakRNG — demonstracija slabog generatora slučajnih brojeva

## Ideja projekta

Cilj projekta je da pokaže zašto je za kriptografiju bitno koristiti
kriptografski siguran generator slučajnih brojeva umesto jednostavnog i
predvidivog algoritma.

Zamisao je da isti proces "žrtve" može da radi u dva režima:

- **normalan (bezbedan)** režim — koristi proveren algoritam i sistemski RNG,
- **napadnut (slab)** režim — koristi predvidiv generator, gde napadač koji
  zna seed može da rekonstruiše keystream i dešifruje poruku.

Time se demonstrira da nešto može statistički da *izgleda* slučajno, a da je u
stvari potpuno predvidivo.

## Planirane komponente

- **weakrng_driver.c** — kernel modul (char device) koji pravi `/dev/weakrng`.
  Treba da primi seed, pa da na čitanje vraća niz bajtova iz slabog generatora.

- **victim_crypto_process.c** — proces "žrtve". Čita podešavanja iz
  `crypto.conf` i šifruje poruku. U normalnom režimu treba da koristi siguran
  algoritam i sistemski RNG, a u napadnutom slab keystream iz drajvera.

- **attacker_decrypt_weak.c** — proces "napadača" koji, ako zna seed i
  algoritam, treba da rekonstruiše isti keystream i dešifruje ciphertext.

- **predict_weakrng.c** — treba da predvidi izlaz drajvera samo na osnovu
  seeda, bez pristupa drajveru.

- **test_weakrng.c** — poredi izlaz drajvera sa nezavisno izračunatim bajtovima
  da bi dokazao predvidivost.

- **frequency_test.c** — statistički test koji pokazuje da izlaz izgleda
  ravnomerno raspoređen.

- **crypto.conf** — konfiguracioni fajl (algoritam, RNG, seed).

- **attack_change_algorithm.sh / normal_mode.sh** — skripte koje prebacuju
  sistem između napadnutog i normalnog režima.

## Grubi plan rada (faze)

1. **Koncept i struktura** — definisati ideju i napraviti fajlove 
2. **Drajver** — implementirati `/dev/weakrng` i osnovne operacije (read/write)
3. **Žrtva** — proces koji šifruje poruku i čita `crypto.conf`
4. **Napad** — napadnuti režim, attacker proces i test predvidivosti
5. **Provere i demonstracija** — statistički test i skripte za prebacivanje režima

## Pokretanje (planirano)

```bash
make            # kompajlira drajver i programe
make load       # učitava modul i pravi /dev/weakrng
make unload     # uklanja modul
make clean      # briše izgrađeno
```
