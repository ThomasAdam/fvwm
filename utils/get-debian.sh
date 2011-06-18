#!/bin/sh

mkdir debian && {
(
    cd debian
    git init
    git remote add -t fvwm -m fvwm -f origin \
        git://git.debian.org/~srivasta/debian/debian-dir.git
    git pull
    git submodule init
    git submodule update
)
} || exit 1
    
[ -f utils/changelog-debian-v-261.patch ] &&
    git apply utils/changelog-debian-v-261.patch
