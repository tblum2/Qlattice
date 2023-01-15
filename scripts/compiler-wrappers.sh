#!/bin/bash

# With the wrappers:
# 1) select compiler based on availability
# 2) support --wrapper-remove-arg=XXX option to remove unwanted flags

. scripts/res/conf.sh

mkdir -p "$prefix/bin"

cat - >"$prefix/bin/compiler-options.py" << EOF
#!/usr/bin/env python3
import sys
import subprocess as p
def process_remove_arg(argv):
    name = "--wrapper-remove-arg="
    name_len = len(name)
    args_to_remove = []
    for arg in argv:
        if arg[:name_len] == name:
            args_to_remove.append(arg)
            args_to_remove.append(arg[name_len:])
    for x in args_to_remove:
        while x in argv:
            argv.remove(x)
argv = sys.argv.copy()
process_remove_arg(argv)
# status = p.run(argv[1:])
# sys.exit(status.returncode)
status = p.call(argv[1:])
sys.exit(status)
EOF
chmod +x "$prefix/bin/compiler-options.py"

cat - >"$prefix/bin/CC.sh" << EOF
#!/bin/bash
if which python3 >/dev/null 2>&1 ; then
    run=compiler-options.py
else
    run=
fi
if [ intel = "\$USE_COMPILER" ] ; then
    \$run icc "\$@"
elif [ gcc = "\$USE_COMPILER" ] ; then
    \$run gcc "\$@"
elif [ clang = "\$USE_COMPILER" ] ; then
    \$run clang "\$@"
elif [ -f "$prefix/bin/clang" ] ; then
    \$run clang "\$@"
elif [ -f "$prefix/bin/gcc" ] ; then
    \$run gcc "\$@"
elif which icc >/dev/null 2>&1 ; then
    \$run icc "\$@"
elif [ -f "/usr/bin/clang" ] ; then
    \$run clang "\$@"
else
    \$run gcc "\$@"
fi
EOF
chmod +x "$prefix/bin/CC.sh"

cat - >"$prefix/bin/CXX.sh" << EOF
#!/bin/bash
if which python3 >/dev/null 2>&1 ; then
    run=compiler-options.py
else
    run=
fi
if [ intel = "\$USE_COMPILER" ] ; then
    \$run icpc "\$@"
elif [ gcc = "\$USE_COMPILER" ] ; then
    \$run g++ "\$@"
elif [ clang = "\$USE_COMPILER" ] ; then
    \$run clang++ "\$@"
elif [ -f "$prefix/bin/clang++" ] ; then
    \$run clang++ "\$@"
elif [ -f "$prefix/bin/g++" ] ; then
    \$run g++ "\$@"
elif which icpc >/dev/null 2>&1 ; then
    \$run icpc "\$@"
elif [ -f "/usr/bin/clang++" ] ; then
    \$run clang++ "\$@"
else
    \$run g++ "\$@"
fi
EOF
chmod +x "$prefix/bin/CXX.sh"

cat - >"$prefix/bin/MPICC.sh" << EOF
#!/bin/bash
if which python3 >/dev/null 2>&1 ; then
    run=compiler-options.py
else
    run=
fi
if [ intel = "\$USE_COMPILER" ] ; then
    \$run mpiicc "\$@"
elif which mpiicc >/dev/null 2>&1 ; then
    if [ gcc = "\$USE_COMPILER" ] ; then
        \$run mpiicc -cc=gcc "\$@"
    elif [ clang = "\$USE_COMPILER" ] ; then
        \$run mpiicc -cc=clang "\$@"
    elif [ -f "$prefix/bin/clang" ] ; then
        \$run mpiicc -cc=clang "\$@"
    elif [ -f "$prefix/bin/gcc" ] ; then
        \$run mpiicc -cc=gcc "\$@"
    elif [ -f "/usr/bin/clang" ] ; then
        \$run mpiicc -cc=clang "\$@"
    elif [ -f "/usr/bin/gcc" ] ; then
        \$run mpiicc -cc=gcc "\$@"
    else
        \$run mpiicc "\$@"
    fi
else
    if [ gcc = "\$USE_COMPILER" ] ; then
        OMPI_CC=gcc \$run mpicc "\$@"
    elif [ clang = "\$USE_COMPILER" ] ; then
        OMPI_CC=clang \$run mpicc "\$@"
    else
        \$run mpicc "\$@"
    fi
fi
EOF
chmod +x "$prefix/bin/MPICC.sh"

cat - >"$prefix/bin/MPICXX.sh" << EOF
#!/bin/bash
if which python3 >/dev/null 2>&1 ; then
    run=compiler-options.py
else
    run=
fi
if [ intel = "\$USE_COMPILER" ] ; then
    \$run mpiicpc "\$@"
elif which mpiicpc >/dev/null 2>&1 ; then
    if [ gcc = "\$USE_COMPILER" ] ; then
        \$run mpiicpc -cxx=g++ "\$@"
    elif [ clang = "\$USE_COMPILER" ] ; then
        \$run mpiicpc -cxx=clang++ "\$@"
    elif [ -f "$prefix/bin/clang++" ] ; then
        \$run mpiicpc -cxx=clang++ "\$@"
    elif [ -f "$prefix/bin/g++" ] ; then
        \$run mpiicpc -cxx=g++ "\$@"
    elif [ -f "/usr/bin/clang++" ] ; then
        \$run mpiicpc -cxx=clang++ "\$@"
    elif [ -f "/usr/bin/g++" ] ; then
        \$run mpiicpc -cxx=g++ "\$@"
    else
        \$run mpiicpc "\$@"
    fi
elif which mpicxx >/dev/null 2>&1 ; then
    if [ gcc = "\$USE_COMPILER" ] ; then
        OMPI_CXX=g++ \$run mpicxx "\$@"
    elif [ clang = "\$USE_COMPILER" ] ; then
        OMPI_CXX=clang++ \$run mpicxx "\$@"
    else
        \$run mpicxx "\$@"
    fi
else
    if [ gcc = "\$USE_COMPILER" ] ; then
        OMPI_CXX=g++ \$run mpic++ "\$@"
    elif [ clang = "\$USE_COMPILER" ] ; then
        OMPI_CXX=clang++ \$run mpic++ "\$@"
    else
        \$run mpic++ "\$@"
    fi
fi
EOF
chmod +x "$prefix/bin/MPICXX.sh"
