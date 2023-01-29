#!/bin/bash

# Need to run on computing node for UCONN HPC
# You can use within interactive session obtained by the following command:
# fisbatch -n 2 -p generalsky

set -e

{

./scripts/setenv.uconn.sh

./scripts/gsl.sh
./scripts/cuba.sh
./scripts/zlib.sh
./scripts/eigen.sh
./scripts/perl.sh
./scripts/openssl.sh
./scripts/libffi.sh
./scripts/openblas.sh
./scripts/python.sh
./scripts/python-pip.sh
./scripts/re2c.sh
./scripts/ninja.sh
./scripts/ninja-script.sh
./scripts/python-meson.sh
./scripts/python-meson-py.sh
./scripts/python-packages.sh

./scripts/fftw.sh
./scripts/fftwf.sh

./scripts/qlat.sh

./scripts/qlat-examples-py.sh
./scripts/qlat-examples-cpp.sh

./scripts/c-lime.sh
./scripts/hdf5.sh
./scripts/gmp.sh
./scripts/mpfr.sh
./scripts/grid-clehner.avx512.sh
./scripts/qlat-grid.sh

./scripts/gpt.sh

./scripts/qlat-examples-py-gpt.sh

} 2>&1 | tee $prefix/log.build.txt
