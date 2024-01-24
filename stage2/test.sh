#!/bin/bash

set -e

make cgrep

tests=('{' '.*' '^{' '^{$' '..;' ';$')

for regex in "${tests[@]}"; do
  ./cgrep "$regex" cgrep.c > cgrepout
  grep "$regex" cgrep.c > grepout
  if [[ -n "$(diff cgrepout grepout)" ]]; then
    echo "Failed on this regex: $regex"
    echo "cgrep output:"
    cat cgrepout
    echo "---------------------------"
    echo "grep output:"
    cat grepout
  else
    echo "Passed this regex test: $regex"
  fi
done

rm cgrepout
rm grepout
