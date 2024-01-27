#!/bin/bash

set -e

make cgrep

tests=('{' '.*' '^{' '^{$' '..;' ';$' '.*;$' 'edge. ' 'str(str)*')

for regex in "${tests[@]}"; do
  echo "Testing: '$regex'"
  ./cgrep "$regex" cgrep.c > cgrepout
  regexgrep="${regex/\(/\\\(}"
  regexgrep="${regexgrep/\)/\\\)}"
  grep "${regexgrep}" cgrep.c > grepout
  if [[ -n "$(diff cgrepout grepout)" ]]; then
    echo "Failed on this regex: '$regex'"
    echo "cgrep output:"
    cat cgrepout
    echo "---------------------------"
    echo "grep output:"
    cat grepout
  else
    echo "Passed this regex test: '$regex'"
  fi
done

rm cgrepout
rm grepout
