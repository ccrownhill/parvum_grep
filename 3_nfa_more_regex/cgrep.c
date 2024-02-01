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


typedef struct next_l_edge {
  char cond_ch;
  int always;
  struct nfa_node* node;
  struct next_l_edge* next;
} next_l_edge;

typedef struct nfa_node {
  int isend;
  struct next_l_edge* next_l;
} nfa_node;

typedef struct nfa {
  int num_nodes;
  struct nfa_node* start;
  struct nfa_node* end;
} nfa;

typedef struct cnl_field {
  const char* text;
  struct nfa_node* node;
  struct cnl_field* next;
} cnl_field;

typedef struct RE {
  int match_start;
  int match_end;
  struct nfa* re_nfa;
} RE;

int char_match(char matcher, char source);
int RE_run(RE* re, const char* regex, const char* text);
RE* RE_gen(char* regex);
void RE_destroy(RE* re);
cnl_field* insert_node_into_cnl(cnl_field* start, nfa_node* node_ptr, const char* text);
cnl_field* remove_node_from_cnl(cnl_field* start, cnl_field* rm_ptr);
next_l_edge* insert_next_edge(next_l_edge* start, int always, char cond_ch, nfa_node* node);
nfa* generate_nfa(const char* regex);
void free_nfa(nfa* nfa_init);
void free_cnl(cnl_field* start);
int is_node_in_list(nfa_node** list, nfa_node* node, int list_size);

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

int RE_run(RE* re, const char* regex, const char* text)
{
  cnl_field* cnl = insert_node_into_cnl(NULL, re->re_nfa->start, text);
  cnl_field* tmp;
  cnl_field* next;
  nfa_node* curr_node;

  // note that cnl can't be NULL at first iteration since there will always
  // be at least one node in the NFA even for an empty regex
  int end = 0;
  do {
    for (tmp = cnl; tmp != NULL; tmp = next) {
      curr_node = tmp->node;
      next = tmp->next; // store next node of cnl here so that even after
                        // removing the current node we still progress with
                        // the correct next node
      

      // only finish when we reach an end node AND
      // if we need to match the end (because of $) the text is over
      if (curr_node->isend && (!re->match_end || tmp->text[0] == '\0')) {
        end = 1;
        goto over;
      }


      // loop over all edges leading to next nodes
      next_l_edge* iter;
      for (iter = curr_node->next_l; iter != NULL; iter = iter->next) {
        if (iter->always || (char_match(iter->cond_ch, tmp->text[0]))) {
          if (iter->always)
            cnl = insert_node_into_cnl(cnl, iter->node, tmp->text);
          else if (tmp->text[0] != '\0')
            cnl = insert_node_into_cnl(cnl, iter->node, tmp->text + 1);
        }
      }
      
      // as long as there is text left and we don't need to match the start
      // (because of ^) we can keep this node and make it look at the next
      // character
      if (curr_node == re->re_nfa->start &&
          !re->match_start &&
          tmp->text[0] != '\0') {
        tmp->text++;
      } else {
        cnl = remove_node_from_cnl(cnl, tmp);
      }
    }
  } while (!end && cnl != NULL);

over:
  free_cnl(cnl);
  return end;
}

cnl_field* insert_node_into_cnl(
    cnl_field* start, nfa_node* node_ptr, const char* text)
{
	cnl_field* ins = malloc(sizeof(cnl_field));
	ins->node = node_ptr;
	ins->next = start;
  ins->text = text;
  return ins;
}

cnl_field* remove_node_from_cnl(cnl_field* start, cnl_field* rm_ptr)
{
  if (start == NULL) {
    return NULL;
  }
  cnl_field* iter;
  cnl_field* next;
  cnl_field* prev;
  if (start == rm_ptr) {
    next = start->next;
    free(start);
    return next;
  }

  prev = start;
  for (iter = start->next; iter != NULL; iter = next) {
    next = iter->next;
    if (iter == rm_ptr) {
      free(iter);
      prev->next = next;
    } else {
      prev = iter;
    }
  }
  return start;
}

next_l_edge* insert_next_edge(next_l_edge* start, int always, char cond_ch, nfa_node* node)
{
  next_l_edge* out = malloc(sizeof(next_l_edge));
  out->cond_ch = cond_ch;
  out->node = node;
  out->next = start;
  out->always = always;
  return out;
}

nfa* generate_nfa(const char* regex) {
  nfa* out_nfa = malloc(sizeof(nfa));
  nfa_node* start;
  nfa_node* end;
  if (regex[0] == '\0') {
    start = malloc(sizeof(nfa_node));
    end = malloc(sizeof(nfa_node));
    start->isend = 0;
    start->next_l = insert_next_edge(NULL, 1, '\0', end);
    end->isend = 1;
    end->next_l = NULL;
    out_nfa->num_nodes = 2;
  } else if (regex[1] == '\0') {
    start = malloc(sizeof(nfa_node));
    end = malloc(sizeof(nfa_node));
    start->isend = 0;
    start->next_l = insert_next_edge(NULL, 0, regex[0], end);
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
      start->next_l = insert_next_edge(NULL, 1, '\0', first->start);
      first->start->next_l = insert_next_edge(first->start->next_l, 1, '\0', second->start);
      first->end->next_l =
        insert_next_edge(first->end->next_l, 1, '\0', first->start);
      first->end->isend = 0;
      end = second->end;
      out_nfa->num_nodes = 1;
    } else { // concatenation
      second = generate_nfa(regex_end + 1);
      first->end->isend = 0;
      first->end->next_l =
        insert_next_edge(first->end->next_l, 1, '\0', second->start);
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

  out->re_nfa = generate_nfa(regex);
  return out;
}

void RE_destroy(RE* re)
{
  free_nfa(re->re_nfa);
  free(re);
}

int is_node_in_list(nfa_node** list, nfa_node* node, int list_size)
{
  for (int i = 0; i < list_size; i++) {
    if (list[i] == node)
      return 1;
  }
  return 0;
}

// deallocate memory for nfa
void free_nfa(nfa* nfa)
{
  nfa_node** tofree = malloc(sizeof(nfa_node) * nfa->num_nodes);
  tofree[0] = nfa->start;
  int node_count = 1; // count how many nodes have been added to the array
  int i;
  next_l_edge* tmp_edge;
  next_l_edge* next_edge;
  for (i = 0; i < nfa->num_nodes; i++) {
    for (tmp_edge = tofree[i]->next_l; tmp_edge != NULL; tmp_edge = next_edge) {
      next_edge = tmp_edge->next;
      if (!is_node_in_list(tofree, tmp_edge->node, node_count))
        tofree[node_count++] = tmp_edge->node;
      free(tmp_edge);
    }
  }

  for (i = 0; i < nfa->num_nodes; i++) {
    free(tofree[i]);
  }

  free(nfa);
  free(tofree);
}

void free_cnl(cnl_field* start)
{
  cnl_field* next;
  for (; start != NULL; start = next) {
    next = start->next;
    free(start);
  }
}
