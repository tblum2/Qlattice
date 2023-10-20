#!/bin/bash

name=qlat-examples-py

source qcore/set-prefix.sh $name

{ time {
    echo "!!!! build $name !!!!"
    source qcore/conf.sh ..

    time-run rsync -a --delete "$wd"/examples-py "$prefix"/

    if [ -n "$QLAT_MPICXX" ] ; then
        export MPICXX="$QLAT_MPICXX"
    fi
    export CXX="$MPICXX"
    if [ -n "$QLAT_CXXFLAGS" ] ; then
        export CXXFLAGS="$QLAT_CXXFLAGS"
    fi
    if [ -n "$QLAT_LDFLAGS" ] ; then
        export LDFLAGS="$QLAT_LDFLAGS"
    fi
    if [ -n "$QLAT_LIBS" ] ; then
        export LIBS="$QLAT_LIBS"
    fi

    export mpi_options="--oversubscribe $mpi_options"

    q_verbose=1 time-run make -C "$prefix"/examples-py update-sources || true
    q_verbose=1 time-run make -C "$prefix"/examples-py run -j "$num_test" || true

    cd "$wd"

    for log in examples-py/*.log ; do
        echo diff "$prefix/$log" "$log"
        diff "$prefix/$log" "$log" | grep 'CHECK: ' && ( echo "$log" ; cat "$prefix/$log" || true )
        if diff "$prefix/$log" "$log" >/dev/null 2>&1 ; then
            :
        else
            cp -rpv "$prefix/$log" "$log".new || true
            cp -rpv "$prefix/${log%.log}.py.p/log.full.txt" "$log".full.txt || true
        fi
    done

    echo "!!!! $name build !!!!"
    rm -rf "$temp_dir" || true
} } 2>&1 | tee $prefix/log.$name.txt
