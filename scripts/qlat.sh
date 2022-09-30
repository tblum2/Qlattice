#!/bin/bash

./scripts/qlat-utils.sh

. scripts/conf.sh

name=qlat

{

    time {

    echo "!!!! build $name !!!!"

    build="$prefix/build-qlat"
    mkdir -p "$build"

    cd "$build"

    rm -rfv "$prefix"/include/qlat
    rm -rfv "$prefix"/include/qlat-setup.h
    rm -rfv "$prefix"/lib/python3*/*-packages/cqlat.*
    rm -rfv "$prefix"/lib/python3*/*-packages/qlat
    rm -rfv "$prefix"/lib/python3*/*-packages/qlat_gpt.py
    rm -rfv "$prefix"/lib/python3*/*-packages/rbc_ukqcd*
    rm -rfv "$prefix"/lib/python3*/*-packages/auto_contractor*

    option=
    if [ -n "$QLAT_MPICXX" ] ; then
        export CXX="$QLAT_MPICXX"
        export MPICXX=false
        option="-Duse_cxx=true"
    fi
    if [ -n "$QLAT_CXXFLAGS" ] ; then
        export CXXFLAGS="$QLAT_CXXFLAGS"
    fi
    if [ -n "$QLAT_LDFLAGS" ] ; then
        export LDFLAGS="$QLAT_LDFLAGS"
    fi
    if [ -n "$QLAT_LIBS" ] ; then
        export LIBS="$QLAT_LIBS"
    fi

    touch "$wd"/qlat/meson.build

    meson "$wd/qlat" --prefix="$prefix" $option
    ninja -j$num_proc
    ninja install

    echo "!!!! $name build !!!!"

    rm -rf $temp_dir || true

}

} |& tee $prefix/log.$name.txt
