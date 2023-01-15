#!/bin/bash

. scripts/res/conf.sh

name=qlat-clean-build

{

echo "!!!! build $name !!!!"

rm -rfv "$prefix/build-qlat"*

echo "!!!! $name build !!!!"

rm -rf $temp_dir || true

} 2>&1 | tee $prefix/log.$name.txt
