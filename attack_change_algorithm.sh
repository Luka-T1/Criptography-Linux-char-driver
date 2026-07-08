#!/bin/bash
# Simulira napad: menja crypto.conf tako da victim proces
# pocne da koristi slab, predvidiv algoritam.

cat > crypto.conf << EOF
algorithm=WEAK_STREAM
rng=/dev/weakrng
seed=12345
EOF

echo "crypto.conf promenjen:"
cat crypto.conf
echo
echo "Proces je sada u napadnutom rezimu: WEAK_STREAM + /dev/weakrng"
