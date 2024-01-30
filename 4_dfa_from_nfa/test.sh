#!/bin/bash


make cgrep

tests=('{' '.*' '^{' '^{$' '..;' ';$' '.*;$' 'edge. ' 'str(str)*')

passed=0
for regex in "${tests[@]}"; do
  echo "Testing: '$regex'"
  if valgrind --errors-for-leak-kinds=all --leak-check=full \
    --error-exitcode=1 ./cgrep "$regex" cgrep.c &>/dev/null; then
    echo "Valgrind check found no memory leaks"
  else
    echo "Valgrind detected memory problems"
    echo "Run this for more details:"
    echo "valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all"\
      "./cgrep '$regex' cgrep.c"
    exit 1
  fi
  ./cgrep "$regex" cgrep.c > cgrepout
  regexgrep="${regex/\(/\\\(}"
  regexgrep="${regexgrep/\)/\\\)}"
  grep "${regexgrep}" cgrep.c > grepout
  if [[ -n "$(diff cgrepout grepout)" ]]; then
    echo "Failed on this regex: '$regex'"
    if [[ $1 == "-v" ]]; then
      echo "cgrep output:"
      cat cgrepout
      echo "---------------------------"
      echo "grep output:"
      cat grepout
    else
      echo "run with -v for more information"
      echo "or see outputs in 'cgrepout' and 'grepout' files in local directory"
    fi
    exit 1
  else
    echo "Passed this regex test: '$regex'"
    ((passed=passed+1))
  fi
done

echo "---------------------------------"
echo "Passed $passed/${#tests[@]} tests"

rm cgrepout
rm grepout
