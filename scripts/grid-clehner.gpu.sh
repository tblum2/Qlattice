#!/usr/bin/env bash

name=Grid-clehner

source qcore/set-prefix.sh $name

{ time {
    echo "!!!! build $name !!!!"
    source qcore/conf.sh ..

    mkdir -p "$src_dir" || true
    time-run rsync -a --delete $distfiles/$name "$src_dir"/
    cd "$src_dir/$name"

    INITDIR="$(pwd)"
    rm -rfv "${INITDIR}/Eigen/Eigen/unsupported"
    rm -rfv "${INITDIR}/Grid/Eigen"
    if [ -n "$(find-header.sh Eigen/Eigen)" ] ; then
        eigen_path="$(find-header.sh Eigen/Eigen)"
        rsync -av --delete "$eigen_path/include/Eigen/" ${INITDIR}/Grid/Eigen/
        rsync -av --delete "$eigen_path/include/unsupported/Eigen/" ${INITDIR}/Grid/Eigen/unsupported/
        cd ${INITDIR}/Grid
        echo 'eigen_files =\' > ${INITDIR}/Grid/Eigen.inc
        find -L Eigen -type f -print | sed 's/^/  /;$q;s/$/ \\/' >> ${INITDIR}/Grid/Eigen.inc
        cd ${INITDIR}
    else
        ln -vs "${INITDIR}/Eigen/Eigen" "${INITDIR}/Grid/Eigen"
        ln -vs "${INITDIR}/Eigen/unsupported/Eigen" "${INITDIR}/Grid/Eigen/unsupported"
    fi

    export CC=
    export CXX=nvcc
    export CFLAGS=
    export CXXFLAGS="-Xcompiler -fPIC -ccbin MPICXX.sh -arch=$NVCC_ARCH -w"
    export LDFLAGS="-Xcompiler -fopenmp -ccbin MPICXX.sh"
    export LIBS=
    export MPICXX=
    export MPICC=

    opts=""
    if [ -n "$(find-library.sh libgmp.a)" ] ; then
        opts+=" --with-gmp=$(find-library.sh libgmp.a)"
    fi
    if [ -n "$(find-library.sh libmpfr.a)" ] ; then
        opts+=" --with-mpfr=$(find-library.sh libmpfr.a)"
    fi
    if [ -n "$(find-library.sh libfftw3.a)" ] ; then
        opts+=" --with-fftw=$(find-library.sh libfftw3.a)"
    fi
    if [ -n "$(find-library.sh liblime.a)" ] ; then
        opts+=" --with-lime=$(find-library.sh liblime.a)"
    fi
    if [ -n "$(find-library.sh libcrypto.a)" ] ; then
        opts+=" --with-openssl=$(find-library.sh libcrypto.a)"
    fi
    if [ -n "$(find-library.sh libhdf5_hl_cpp.a)" ] ; then
        opts+=" --with-hdf5=$(find-library.sh libhdf5_hl_cpp.a)"
    fi

    if [ -n "$(find-library.sh libz.a)" ] ; then
        LDFLAGS+=" -L$(find-library.sh libz.a)/lib"
        CXXFLAGS+=" -I$(find-library.sh libz.a)/include"
        export LDFLAGS
        export CXXFLAGS
    fi

    echo "CXXFLAGS='$CXXFLAGS'"
    echo "LDFLAGS='$LDFLAGS'"

    mkdir build
    cd build
    time-run ../configure \
        --enable-simd=GPU \
        --enable-gen-simd-width=32 \
        --enable-alloc-align=4k \
        --enable-comms=mpi-auto \
        --enable-unified=no \
        --enable-accelerator=cuda \
        --enable-accelerator-cshift \
        --enable-gparity=no \
        --disable-fermion-reps \
        $opts \
        --prefix="$prefix"

    time-run make -j$num_proc -C Grid
    time-run make install -C Grid
    time-run install -D -m755 grid-config "$prefix"/bin/grid-config

    mk-setenv.sh
    echo "!!!! $name build !!!!"
    rm -rf "$temp_dir" || true
    touch "$prefix"/build-successfully.txt
} } 2>&1 | tee "$prefix/log.$name.txt"
