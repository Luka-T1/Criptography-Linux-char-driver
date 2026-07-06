# Makefile za weakrng simulaciju
#
# "make"        - kompajlira kernel modul i sve userspace programe
# "make clean"  - brise sve sto je izgradjeno
# "make load"   - ucitava kernel modul i pravi /dev/weakrng
# "make unload" - uklanja kernel modul

obj-m += weakrng_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc test_weakrng.c -o test_weakrng
	gcc predict_weakrng.c -o predict_weakrng
	gcc frequency_test.c -o frequency_test
	gcc victim_crypto_process.c -o victim_crypto_process -lcrypto
	gcc attacker_decrypt_weak.c -o attacker_decrypt_weak

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f test_weakrng predict_weakrng frequency_test victim_crypto_process attacker_decrypt_weak

load:
	sudo insmod weakrng_driver.ko

unload:
	sudo rmmod weakrng_driver
