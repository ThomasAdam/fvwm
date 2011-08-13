#!/bin/sh

for i in *.txt
do
    a2x -vf manpage "$i"
done
