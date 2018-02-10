#!/bin/sh
ldd bin/HexTile | grep '=>' \
    | grep -v '\s./bin/libs/' \
    | sed 's/=>//' \
    | sed  's/[(][^)]*[)]//' \
    | awk '{ if ($2 != "") { print $2 }}' \
    | xargs --no-run-if-empty -l1 cp -L -v -t bin/libs 
