#!/bin/bash

for i in `ls include | egrep '\.hh$'`
do
    if grep -q newInstance "include/$i"; then
        echo "#include \"$i\"";
    fi
done

cat << EOF
#include "Reflect.hh"

namespace m3
{

StringKeyHashMap<Reflect::CreateMethod> Reflect::reflect =
{
EOF

for i in `ls include | egrep '\.hh$'`
do
    for res in `grep newInstance "include/$i"`
    do
        if [[ "$res" =~ "//@Reflect" ]]; then
            res=${res##*//@Reflect}
            echo "    { \"$res\", $res::newInstance },"
        fi
    done
done

cat << EOF
};


}

EOF