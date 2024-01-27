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

## Stage 2: Simple Non-deterministic Finite Automaton

Implement the same using a Non-Deterministic Finite Automaton, i.e. a finite state
machine.

I used a graph structure with each node containing the character that would allow
progression to the next node as well as a type which would determine whether it
is also allowed to stay at the current node (e.g. for a `*` we can stay in the
same state for an arbitrary number of repetitions of the previous character)

This however, was only a simplified NFA generation because it only allowed for
nodes connected from start to end with possible connections of one node to itself
but otherwise always to the next node in the chain.

## Stage 3: General Non-deterministic Finite Automaton

Here I allowed for arbitrary connections of nodes (also backwards to older ones).

### Implementation

Moreover, I use a more general recursive algorithm to generate the NFA in `generate_nfa`:

* introduced new super structures `RE` and `nfa` where `nfa` has properties `start`
and `end` which are both `nfa_node*`
* base cases are the empty string and a 1 character regex
* both of them will return an `nfa*` with start and end connected either by
an "always" or a conditioned connection
* after that the algorithm splits the regex in groups (grouped by `()`) and
connects the `nfa*` returned for recursive calls on each group according to either
the iteration rule for `*` or the concatenation rule

### Performance

Because the use of a **non-deterministic** finite automaton requires keeping track
of all possible states it should be running quite slowly.

I observed this when comparing the performance against grep:

* `126ms` for `cgrep '*.;$' cgrep.c` and
* `3ms` for `grep '*.;$' cgrep.c`
