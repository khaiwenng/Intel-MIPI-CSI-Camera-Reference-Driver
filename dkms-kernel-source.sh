#!/bin/bash

vers=(${kernelver//./ })
major="${vers[0]}"
minor="${vers[1]}"

makefile=/usr/src/linux-headers-${kernelver}/Makefile
if [ $(wc -l < $makefile) -eq 1 ] && grep -q "^include " $makefile ; then
  makefile=$(tr -s " " < $makefile | cut -d " " -f 2)
fi
subver=$(grep "SUBLEVEL =" $makefile | tr -d " " | cut -d "=" -f 2)

echo "Downloading kernel source $major.$minor.$subver for $kernelver"
if [ "$subver" -ne 0 ]; then
	kernelprefix=linux-$major.$minor.$subver
else
	kernelprefix=linux-$major.$minor
fi
wget --no-check-certificate https://mirrors.edge.kernel.org/pub/linux/kernel/v$major.x/$kernelprefix.tar.xz -O $kernelprefix.tar.xz

for arg in "$@"; do
    echo "Extracting: $kernelprefix/$arg"
    tar -xvf "$kernelprefix.tar.xz" "$kernelprefix/$arg" \
      --xform="s,^${kernelprefix//./\\.}/,$major.$minor.$subver/,"
done
