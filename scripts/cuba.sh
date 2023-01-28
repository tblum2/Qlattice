#!/bin/bash

name=Cuba

source qcore/set-prefix.sh $name

{ time {

    echo "!!!! build $name !!!!"

    source qcore/conf.sh ..

    mkdir -p $src_dir
    cd $src_dir
    tar xzf $distfiles/$name-*.tar.*

    export CFLAGS="$CFLAGS -fPIC"
    export CXXFLAGS="$CXXFLAGS -fPIC"

    cd $name-*
    ./configure \
        --build="$(arch)" \
        --prefix=$prefix

    make -j$num_proc
    make install

    cd "$wd"

    mk-setenv.sh

    echo "!!!! $name build !!!!"

    rm -rf "$temp_dir" || true

} } 2>&1 | tee $prefix/log.$name.txt
