/*
 * Implementation of these regular expressions:
 * - c for character c
 * - . for any character
 * - ^ for start of input
 * - $ for end of input
 * - * for 0 or more repetitions of previous character
 * - () for grouping regular expressions
 *
 * This is implemented using a non-deterministic finite automaton, i.e. a
 * non-deterministic finite state machine
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFLEN 200
#define MAXINPUTS 10

#define IS_NODE_IN_ARRAY(OUT, ARR, NODE, SIZE) ({\
  for (int i = 0; i < SIZE; i++) {\
    if (ARR[i] == NODE)\
      OUT = 1;\
  }\
  OUT = 0;\
})

#define FREE_FA_GRAPH(FIRST_NODE, TOFREE, ITER_EDGE, NEXT_EDGE, NUM_NODES) ({\
  int node_count = 1;\
  TOFREE[0] = FIRST_NODE;\
  for (int i = 0; i < NUM_NODES; i++) {\
    for (ITER_EDGE = TOFREE[i]->next_l; ITER_EDGE != NULL; ITER_EDGE = NEXT_EDGE) {\
      int is_in_arr;\
      IS_NODE_IN_ARRAY(is_in_arr, TOFREE, ITER_EDGE->node, node_count);\
      if (!is_in_arr)\
        TOFREE[node_count++] = ITER_EDGE->node;\
      NEXT_EDGE = ITER_EDGE->next;\
      free(ITER_EDGE);\
    }\
  }\
  for (int i = 0; i < NUM_NODES; i++) {\
    free(TOFREE[i]);\
  }\
})

typedef struct nfa_edge {
  char cond_ch;
  int always;
  struct nfa_node* node;
  struct nfa_edge* next;
} nfa_edge;

typedef struct dfa_edge {
  char cond_ch;
  struct dfa_node* node;
  struct dfa_edge* next;
} dfa_edge;


typedef struct nfa_node {
  int isend;
  struct nfa_edge* next_l;
} nfa_node;

typedef struct nfa {
  int num_nodes;
  struct nfa_node* start;
  struct nfa_node* end;
} nfa;


typedef struct dfa_node {
  int isend;
  struct dfa_edge* next_l;
} dfa_node;

typedef struct dfa {
  int num_nodes;
  struct dfa_node* start;
} dfa;


typedef struct RE {
  int match_start;
  int match_end;
  struct dfa* dfa;
} RE;

typedef struct nfal {
  nfa_node* node;
  struct nfal* next;
} nfal;

typedef struct dfagen_node {
  nfal* nfas;
  dfa_node* node;
  struct dfagen_node* next;
} dfagen_node;

typedef struct always_g {
  int isend;
  nfal* nfas;
} always_g;

typedef struct nfa_2dlist {
  char cond_ch;
  int isend;
  nfal* nfas;
  struct nfa_2dlist* next; 
} nfa_2dlist;


int char_match(char matcher, char source);
int RE_run(RE* re, const char* regex, const char* text);
int RE_matchhere(RE* re, const char* regex, const char* text);
RE* RE_gen(char* regex);
void RE_destroy(RE* re);
nfa_edge* insert_nfa_edge(nfa_edge* start, int always, char cond_ch, nfa_node* node);
dfa_edge* insert_dfa_edge(dfa_edge* start, char cond_ch, dfa_node* node);
nfa* generate_nfa(const char* regex);
void free_nfa(nfa* nfa_init);
int is_node_in_list(nfa_node** list, nfa_node* node, int list_size);

dfa* nfa_to_dfa(nfa* nfa_in);
dfagen_node* insert_into_dfagen_list(dfagen_node* worklist, int isend, nfal* alw_g);
dfagen_node* find_dfagen_node(dfagen_node* list, nfal* nfas);
dfagen_node* insert_node_into_dfagen_list(dfagen_node* worked_list, dfagen_node* item);

always_g* always_group(nfa_node* node);
nfal* insert_into_nfa_list(nfal* list, nfa_node* node);
nfal* union_into_nfa_list(nfal* a, nfal* b);
nfal* copy_nfa_list(nfal* list);
int contains_nfa_list(nfal* list, nfa_node* node);


nfa_2dlist* insert_into_conns(nfa_2dlist* conns, char cond_ch, int isend, nfal* nfas);

void free_nfa_list(nfal* nfa_list);
void free_nfa_2dlist(nfa_2dlist* list);
void free_dfagen_list(dfagen_node* list);
void free_dfa(dfa* dfa);
void free_always_group(always_g* alw_g);



int main(int argc, const char* argv[])
{
  FILE* inputs[MAXINPUTS] = {stdin};
  FILE* input;
  int num_in = 1;
  char regex[BUFLEN];
  if (argc < 2) {
    fprintf(stderr, "Need at least a regular expression\n");
    return 1;
  }
  strncpy(regex, argv[1], BUFLEN);
  regex[BUFLEN-1] = '\0'; // in case the length was longer than BUFLEN we need
                          // to add a '\0' termination byte
  if (argc >= 3) {
    num_in = 0;
    for (int i = 2; i < argc; i++) {
      if (!(input = fopen(argv[i], "r"))) {
        fprintf(stderr, "Can't open file '%s'\n", argv[i]);
        return 1;
      }
      inputs[i - 2] = input;
      num_in++;
    }
  }

  RE* re = RE_gen(regex);
  char buf[BUFLEN];
  while (--num_in >= 0) {
    while (fgets(buf, BUFLEN, inputs[num_in])) {
      buf[strlen(buf) - 1] = '\0'; // get rid of newline at the end
      if (RE_run(re, regex, buf)) {
        printf("%s\n", buf);
      }
    }
    fclose(inputs[num_in]);
  }
  RE_destroy(re);
}


int char_match(char matcher, char source)
{
  return (matcher == '.' || matcher == source);
}

RE* RE_gen(char* regex)
{
  RE* out = malloc(sizeof(RE));
  out->match_start = 0;
  out->match_end = 0;
  if (regex[0] == '^') {
    out->match_start = 1;
    regex++;
  }

  if (regex[strlen(regex) - 1] == '$') {
    out->match_end = 1;
    regex[strlen(regex) - 1] = '\0';
  }

  nfa* re_nfa = generate_nfa(regex);
  out->dfa = nfa_to_dfa(re_nfa);
  free_nfa(re_nfa);
  return out;
}

void RE_destroy(RE* re)
{
  free_dfa(re->dfa);
  free(re);
}

int RE_run(RE* re, const char* regex, const char* text)
{
  if (regex[0] == '^') {
    return RE_matchhere(re, regex + 1, text);
  } else {
    do {
      if (RE_matchhere(re, regex, text))
        return 1;
    } while (*text++ != '\0');
  }
  return 0;
}

int RE_matchhere(RE* re, const char* regex, const char* text)
{
  dfa_node* curr_node = re->dfa->start;

  int end = 0;
  int match = 0;
  do {
    // only finish when we reach an end node AND
    // if we need to match the end (because of $) the text is over
    if (curr_node->isend && (!re->match_end || text[0] == '\0')) {
      end = 1;
      break;
    }

    match = 0;

    // loop over all edges leading to next nodes
    dfa_edge* iter;
    for (iter = curr_node->next_l; iter != NULL; iter = iter->next) {
      if (char_match(iter->cond_ch, text[0])) {
        curr_node = iter->node;
        match = 1;
        break; // only one edge can match the current character
      }
    }
    
  } while (match && *text++ != '\0');

  return end;
}


// Convert Non-deterministic Finite Automaton to Deterministic Finite Automaton
dfa* nfa_to_dfa(nfa* nfa_in)
{
  dfa* out = malloc(sizeof(dfa));
  always_g* alw_g = always_group(nfa_in->start);
  dfagen_node* worklist = insert_into_dfagen_list(NULL, alw_g->isend, alw_g->nfas);
  free_always_group(alw_g); // FREE
  
  dfagen_node* worked_list = NULL;

  out->start = worklist->node;
  out->num_nodes = 1;
  dfa_node* curr_node;
  dfagen_node* tmp;

  dfagen_node* next;
  nfal* iter_nfa;
  nfa_edge* iter_edge;
  char cond_ch;
  nfa_2dlist* conns;
  nfa_2dlist* iter_conns;

  while (worklist != NULL) {
    curr_node = worklist->node;
    // loop over all nodes belonging to current dfa_node
    conns = NULL;
    for (iter_nfa = worklist->nfas; iter_nfa != NULL; iter_nfa = iter_nfa->next) {
      // access nfa node with iter_nfa->node;
      // loop over edges of the node
      for (iter_edge = iter_nfa->node->next_l; iter_edge != NULL; iter_edge = iter_edge->next) {
        if (iter_edge->always) // ignore always connections since they have
                               // already been captured by always_group
          continue;
        alw_g = always_group(iter_edge->node); // get epsilon closure of this edge
        conns = insert_into_conns(conns, iter_edge->cond_ch, alw_g->isend, alw_g->nfas);
        free_always_group(alw_g); // FREE
      }
    }

    // remove first item from worklist queue
    tmp = worklist->next;
    worked_list = insert_node_into_dfagen_list(worked_list, worklist);
    // note that worklist->nfas does not have to be freed since it is used by
    // worked_list
    free_nfa_list(worklist->nfas);

    free(worklist);
    worklist = tmp;
    // loop over the sets of nodes for each outgoing character edge
    for (iter_conns = conns; iter_conns != NULL; iter_conns = iter_conns->next) {
      // add dfagen_node to worklist with this set of nfa_nodes
      if ((tmp = find_dfagen_node(worked_list, iter_conns->nfas)) == NULL) {
        worklist = insert_into_dfagen_list(worklist, iter_conns->isend, iter_conns->nfas);
        worklist->node->isend = iter_conns->isend;
        worklist->node->next_l = NULL;
        tmp = worklist;
        out->num_nodes++;
      }
      curr_node->next_l = insert_dfa_edge(curr_node->next_l, iter_conns->cond_ch, tmp->node);
    }
    free_nfa_2dlist(conns);
  }
  free_dfagen_list(worked_list);


  return out;
}


// Generate Non-deterministic Finite Automaton for given regular expression
nfa* generate_nfa(const char* regex) {
  nfa* out_nfa = malloc(sizeof(nfa));
  nfa_node* start;
  nfa_node* end;
  if (regex[0] == '\0') {
    start = malloc(sizeof(nfa_node));
    end = malloc(sizeof(nfa_node));
    start->isend = 0;
    start->next_l = insert_nfa_edge(NULL, 1, '\0', end);
    end->isend = 1;
    end->next_l = NULL;
    out_nfa->num_nodes = 2;
  } else if (regex[1] == '\0') {
    start = malloc(sizeof(nfa_node));
    end = malloc(sizeof(nfa_node));
    start->isend = 0;
    start->next_l = insert_nfa_edge(NULL, 0, regex[0], end);
    end->isend = 1;
    end->next_l = NULL;
    out_nfa->num_nodes = 2;
  } else {
    const char* regex_end;
    char* left_regex;
    if (regex[0] == '(') {
      regex_end = strstr(regex, ")");
      int size = regex_end - (regex + 1);
      left_regex = strndup(regex + 1, size);
      left_regex[size-1] = '\0'; // remove trailing ')'
    } else {
      regex_end = regex;
      left_regex = strndup(regex, 1); // this will just give the first
                                      // character of the regex with '\0'
                                      // appended
    }

    nfa* first = generate_nfa(left_regex);
    free(left_regex);
    nfa* second;
    if (regex_end[1] == '*') { // iteration
      second = generate_nfa(regex_end + 2);
      start = malloc(sizeof(nfa_node));
      start->isend = 0;
      start->next_l = insert_nfa_edge(NULL, 1, '\0', first->start);
      first->start->next_l = insert_nfa_edge(first->start->next_l, 1, '\0', second->start);
      first->end->next_l =
        insert_nfa_edge(first->end->next_l, 1, '\0', first->start);
      first->end->isend = 0;
      end = second->end;
      out_nfa->num_nodes = 1;
    } else { // concatenation
      second = generate_nfa(regex_end + 1);
      first->end->isend = 0;
      first->end->next_l =
        insert_nfa_edge(first->end->next_l, 1, '\0', second->start);
      start = first->start;
      end = second->end;
      out_nfa->num_nodes = 0;
    }
    out_nfa->num_nodes += first->num_nodes + second->num_nodes;
    free(first);
    free(second);
  }
  out_nfa->start = start;
  out_nfa->end = end;
  return out_nfa;
}


// HELPER FUNCTIONS

always_g* always_group(nfa_node* node)
{
  always_g* out = malloc(sizeof(always_g));
  out->isend = node->isend;
  out->nfas = NULL;
  out->nfas = insert_into_nfa_list(out->nfas, node);

  nfa_edge* iter_edge;
  always_g* tmp_alwg;
  for (iter_edge = node->next_l; iter_edge != NULL; iter_edge = iter_edge->next) {
    if (iter_edge->always) {
      tmp_alwg = always_group(iter_edge->node);
      out->isend = out->isend || tmp_alwg->isend;
      out->nfas = union_into_nfa_list(out->nfas, tmp_alwg->nfas);
      free_always_group(tmp_alwg);
    } else {
      if (iter_edge->node == node)
      out->nfas = insert_into_nfa_list(out->nfas, iter_edge->node);
    }
  }


  return out;
}

nfal* insert_into_nfa_list(nfal* list, nfa_node* node)
{
  nfal* out = malloc(sizeof(nfal));
  out->node = node;
  out->next = list;
  return out;
}

// constructs union of lists a and b
// no copies of nfal* nodes are made just next references are changed
nfal* union_into_nfa_list(nfal* a, nfal* b)
{
  nfal* out = a;
  nfal* next;
  nfal* copy = copy_nfa_list(b);
  for (nfal* iter = copy; iter != NULL; iter = next) {
    next = iter->next;
    if (!(contains_nfa_list(a, iter->node))) {
      iter->next = out; // insert at start of output list
      out = iter;
    } else {
      free(iter);
    }
  }
  return out;
}

int contains_nfa_list(nfal* list, nfa_node* node)
{
  for (nfal* iter = list; iter != NULL; iter = iter->next)
    if (iter->node == node)
      return 1;
  return 0;
}

int match_nfa_list(nfal* a, nfal* b)
{
  nfal* iter_a;
  nfal* iter_b;
  for (iter_a = a, iter_b = b;
      iter_a != NULL && iter_b != NULL;
      iter_a = iter_a->next, iter_b = iter_b->next) {
    if (iter_a->node != iter_b->node)
      return 0;
  }
  return (iter_a == NULL && iter_b == NULL);
}

dfagen_node* find_dfagen_node(dfagen_node* list, nfal* nfas)
{
  dfagen_node* iter;
  for (iter = list; iter != NULL; iter = iter->next)
    if (match_nfa_list(iter->nfas, nfas))
      return iter;
  return NULL;
}

// checks if cond_ch already in list
// in that case: merge 'nodes' with 'list'
// otherwise: add 'nodes' to new node with cond_ch = 'cond_ch'
// set isend = 1 if one of the 'nodes' has isend = 1
nfa_2dlist* insert_into_conns(nfa_2dlist* conns, char cond_ch, int isend, nfal* nfas)
{
  nfa_2dlist* out = conns;
  int found = 0;
  for (nfa_2dlist* iter = conns; iter != NULL; iter = iter->next) {
    if (iter->cond_ch == cond_ch) {
      found = 1;
      iter->nfas = union_into_nfa_list(iter->nfas, copy_nfa_list(nfas));
      iter->isend = iter->isend || isend;
    }
  }

  if (!found) {
    nfa_2dlist* newl = malloc(sizeof(nfa_2dlist));
    newl->isend = isend;
    newl->nfas = copy_nfa_list(nfas);
    newl->cond_ch = cond_ch;
    newl->next = out;
    out = newl;
  }

  return out;
}

nfal* copy_nfa_list(nfal* list)
{
  nfal* out = NULL;
  for (nfal* iter = list; iter != NULL; iter = iter->next) {
    out = insert_into_nfa_list(out, iter->node);
  }
  return out;
}

dfagen_node* insert_into_dfagen_list(dfagen_node* worklist, int isend, nfal* nfas)
{
  dfagen_node* out = malloc(sizeof(dfagen_node));
  out->nfas = copy_nfa_list(nfas);
  out->node = malloc(sizeof(dfa_node));
  out->node->isend = isend;
  out->node->next_l = NULL;
  out->next = worklist;
  return out;
}

dfagen_node* insert_node_into_dfagen_list(dfagen_node* worked_list, dfagen_node* item)
{
  dfagen_node* out = malloc(sizeof(dfagen_node));
  out->nfas = copy_nfa_list(item->nfas);
  out->node = item->node;
  out->next = worked_list;
  return out;
}

nfa_edge* insert_nfa_edge(nfa_edge* start, int always, char cond_ch, nfa_node* node)
{
  nfa_edge* out = malloc(sizeof(nfa_edge));
  out->cond_ch = cond_ch;
  out->node = node;
  out->next = start;
  out->always = always;
  return out;
}

dfa_edge* insert_dfa_edge(dfa_edge* start, char cond_ch, dfa_node* node)
{
  dfa_edge* out = malloc(sizeof(dfa_edge));
  out->cond_ch = cond_ch;
  out->node = node;
  out->next = start;
  return out;
}


// DEALLOCATION

void free_always_group(always_g* alw_g)
{
  free_nfa_list(alw_g->nfas);
  free(alw_g);
}

void free_dfa(dfa* dfa)
{
  dfa_node** tofree = malloc(sizeof(dfa_node) * dfa->num_nodes);
  dfa_edge* next_edge;
  dfa_edge* iter_edge;
  
  FREE_FA_GRAPH(dfa->start, tofree, iter_edge, next_edge, dfa->num_nodes);

  free(dfa);
  free(tofree);
}


void free_nfa(nfa* nfa)
{
  nfa_node** tofree = malloc(sizeof(nfa_node) * nfa->num_nodes);
  tofree[0] = nfa->start;
  nfa_edge* iter_edge;
  nfa_edge* next_edge;

  FREE_FA_GRAPH(nfa->start, tofree, iter_edge, next_edge, nfa->num_nodes);
  
  free(nfa);
  free(tofree);
}

void free_nfa_list(nfal* nfa_list)
{
  nfal* next;
  for (; nfa_list != NULL; nfa_list = next) {
    next = nfa_list->next;
    free(nfa_list);
  }
}

void free_nfa_2dlist(nfa_2dlist* list)
{
  nfa_2dlist* next;
  for (; list != NULL; list = next) {
    next = list->next;
    free_nfa_list(list->nfas);
    free(list);
  }
}

void free_dfagen_list(dfagen_node* list)
{
  dfagen_node* next;
  for (; list != NULL; list = next) {
    next = list->next;
    free_nfa_list(list->nfas);
    free(list);
  }
}
