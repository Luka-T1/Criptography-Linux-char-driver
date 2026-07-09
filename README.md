# WeakRNG — demonstracija slabog generatora slučajnih brojeva

Cilj ovog projekta je da pokaže zašto je za kriptografiju bitno koristiti
kriptografski siguran generator slučajnih brojeva.

Ideja: isti proces žrtve može da radi u dva režima — normalnom (siguran) 
i napadnutom (slab). U napadnutom režimu se koristi predvidiv RNG, pa 
napadač koji zna seed može da rekonstruiše ceo keystream i dešifruje poruku.

## Delovi projekta

- **weakrng_driver.c** — kernel modul (char device drajver) koji pravi
  `/dev/weakrng`. Korisnik prvo upiše seed, pa čita bajtove. Namerno koristi
  slab, predvidiv generator.

- **victim_crypto_process.c** — proces žrtve. Čita podešavanja iz
  `crypto.conf` i šifruje poruku. U normalnom režimu koristi AES-256-GCM sa
  `/dev/urandom`, a u napadnutom XOR sa slabim keystream-om iz `/dev/weakrng`.

- **attacker_decrypt_weak.c** — proces napadača. Ako zna algoritam i seed,
  izračuna isti keystream i dešifruje ciphertext.

- **predict_weakrng.c** — predviđa izlaz drajvera samo na osnovu seeda i
  algoritma, bez pristupa samom drajveru.

- **test_weakrng.c** — poredi prave bajtove iz drajvera sa nezavisno
  izračunatim bajtovima. Ako se poklapaju, dokazano je da je RNG predvidiv.

- **frequency_test.c** — statistički test: pokazuje da izlaz izgleda
  slučajno, iako je u stvari potpuno predvidiv.

- **crypto.conf** — konfiguracioni fajl (algoritam, RNG, seed).

- **attack_change_algorithm.sh** — menja `crypto.conf` u napadnuti režim
  (WEAK_STREAM + `/dev/weakrng`).

- **normal_mode.sh** — vraća `crypto.conf` u normalan, bezbedan režim.

## Pokretanje

```bash
make            # kompajlira drajver i sve programe
make load       # učitava modul i pravi /dev/weakrng

./normal_mode.sh              # bezbedan režim
./attack_change_algorithm.sh  # napadnuti režim

make unload     # uklanja modul
make clean      # briše sve izgrađeno
```

## Poenta

Slab RNG može da prođe statističke testove i da izgleda slučajno, ali
ako se zna seed, izlaz je u potpunosti predvidiv. Zato se za značajne procese koriste bezbedniji,
sigurni sistemski RNG generatori koji nemaju predstavljene mane.