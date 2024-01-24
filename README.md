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

## Stage 2

Implement the same using a Non-Deterministic Finite Automaton, i.e. a finite state
machine.

I used a graph structure with each node containing the character that would allow
progression to the next node as well as a type which would determine whether it
is also allowed to stay at the current node (e.g. for a `*` we can stay in the
same state for an arbitrary number of repetitions of the previous character)
