#!/bin/bash

. conf.sh

name=setenv

echo "!!!! build $name !!!!"

mkdir -p "$prefix"

cat - setenv.sh >"$prefix/setenv.sh" << EOF
echo "Sourcing '$prefix/setenv.sh'"
if [ -z "\$prefix" ] ; then
    prefix="$prefix"
fi
if [ -z "\$num_proc" ] ; then
    num_proc=2
fi
EOF

./scripts/compiler-wrappers.sh

. "$prefix/setenv.sh" >"$prefix/log.setenv.txt" 2>&1

echo "!!!! $name build !!!!"

rm -rf $temp_dir || true
