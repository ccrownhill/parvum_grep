# cgrep - a simplified grep implementation

## Stage 1

Only accept these regular expressions:

* `^`: matches the beginning of the input string
* `$`: matches the end of the input string
* `.`: matches any character
* `c`: matches the character `c`
* `*`: matches 0 or more occurrences of the previous character

This was implemented without converting the regexes to a different data structure
like a **finite automaton**.
