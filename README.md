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

### More regular expressions

Now I also support `()` brackets to group regular expressions which allows for
expressions like

```
str(str)*
```

### Implementation

I use a more general recursive algorithm to generate the NFA in `generate_nfa`:

* introduced new super structures `RE` and `nfa` where `nfa` has properties `start`
and `end` which are both `nfa_node*`
* base cases are the empty string and a 1 character regex
* both of them will return an `nfa*` with start and end connected either by
an "always" or a conditioned connection
* after that the algorithm splits the regex in groups (grouped by `()`) and
connects the `nfa*` returned for recursive calls on each group according to either
the iteration rule for `*` or the concatenation rule

Note also that the implementation is now very easy to change to allow for more
regular expression features.

For example:

* `|` (alternation) could easily be implemented by adding a third case to `generate_nfa`
which simply connects nodes from two state machines in the correct way
* `[a-z]` like character classes could be implemented by making a new `struct`
as the most basic unit instead of a single character. Then I would only need
to change the `char_match` function to instead match an entity of this struct
with a given input character

However, I did not implement regular expressions exhaustively since this is just
for learning purposes and it would not add that much more new learning experience
vs. time spent.


### Performance

Because the use of a **non-deterministic** finite automaton requires keeping track
of all possible states it should be running quite slowly.

I observed this when comparing the performance against grep:

* `126ms` for `cgrep '*.;$' cgrep.c` and
* `3ms` for `grep '*.;$' cgrep.c`

(Using the `time` command)

## Stage 4: Using a Deterministic Finite Automaton

For this stage I converted the NFA from the previous stage to a DFA.
A DFA does only allow one edge away from a node per input character and can't
have always / $\epsilon$ connections.

This means that there is always only one possible next node we can reach which
means we don't have to keep track of all states our state machine could be in
which should give a big performance improvement for a little overhead when generating
the DFA.

### Implementation

This was done in the function for NFA to DFA conversion `nfa_to_dfa`.

`dfa` is a graph like `nfa` made up of `dfa_nodes` where each of them has a list
with edges and their condition characters

1. Generate an $\epsilon$ closure around the first NFA node, i.e. all nodes that
can be reached over an `always` connection
  * use the `always_group` to get a list of NFA nodes
  * store this list as well with information whether it contains an end node
  in a `dfagen_node` which contains the list of NFA nodes as well as a newly
  created `dfa_node`
1. Add this `dfagen_node` to a linked list `worklist`
1. While the `worklist` is not empty
  1. Pick first item `first`
  1. check all edges going from any of the NFA nodes in the `nfal` (NFA list)
  of `first`
  1. generate a map `conns` using a 2D list of nodes where nodes which can be reached over the same character from `first` are in the same sublist
  1. Remove `first` from worklist
  1. Insert it into `worked_list` (to keep track of older nodes)
  1. Loop over the map `conns`. For any given nfa list `list` associated with a condition
  character:
    1. if: `list` matches with any of the `dfagen_nodes` in `worked_list`:
    set `new_dfa_node` to this node from `worked_list`
    1. else: insert new node into `worklist` which has `list` as its set of NFA nodes
    and set `new_dfa_node` to this new node
    1. insert `new_dfa_node` into `dfa` graph by linking it to the node from `first`
    over an edge with the condition character equal to the map key from `conns`

### Performance

Since running should now be a lot quicker than with an NFA because we don't need
to keep track of multiple states anymore, we should see an overall performance
increase despite a slightly longer setup time in `RE_gen`.

Note that I chose the test regular expression `.*;$` because it means that the
NFA implementation will have to loop a lot while also having many always
connections which means frequent adjustments to the state list will be necessary.

Indeed we can see a huge performance difference when running this expression
on the `cgrep.c` file from stage 3:

* `126ms` for `3_nfa_more_regex/cgrep '*.;$' 3_nfa_more_regex/cgrep.c` and
* `5ms` for `4_dfa_from_nfa/cgrep '*.;$' 3_nfa_more_regex/cgrep.c` and
* `3ms` for `grep '*.;$' cgrep.c`

(Using the `time` command)

## A note on automated testing

The most sophisticated test script can be found in `4_dfa_from_nfa` which will
not only check the output for several regular expressions against the `grep` output
but also check for memory leaks using `valgrind`.
