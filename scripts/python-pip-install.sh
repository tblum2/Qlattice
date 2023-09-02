#!/bin/bash

name=python-pip-install

source qcore/set-prefix.sh $name

{ time {
    echo "!!!! build $name !!!!"
    source qcore/conf.sh ..

    time-run pip3 install -vU meson

    ( :
    unset CC
    unset CXX
    unset MPICC
    unset MPICXX
    unset LD_RUN_PATH
    unset LIBRARY_PATH
    unset CPATH
    time-run pip3 install -vU mpi4py
    : )

    time-run pip3 install -vU psutil
    time-run pip3 install -vU sympy
    time-run pip3 install -vU cython
    time-run pip3 install -vU pybind11
    time-run pip3 install -vU numpy
    time-run pip3 install -vU pythran
    time-run pip3 install -vU scipy
    time-run pip3 install -vU poetry_core
    time-run pip3 install -vU pkgconfig

    mk-setenv.sh
    echo "!!!! $name build !!!!"
    rm -rf $temp_dir || true
} } 2>&1 | tee $prefix/log.$name.txt
