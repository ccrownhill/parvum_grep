/*
 * Implementation of these regular expressions:
 * - c for character c
 * - . for any character
 * - ^ for start of input
 * - $ for end of input
 * - * for 0 or more repetitions of previous character
 *
 * This is implemented using a non-deterministic finite automaton, i.e. a
 * non-deterministic finite state machine
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFLEN 200
#define MAXINPUTS 10

typedef struct nfa_node {
  int end;
  next_l_node* next_l;
} nfa_node;

typedef struct next_l_node {
  int always;
  char cond_ch;
  nfa_node* node;
  struct next_l_node* next;
} next_l_node;

typedef struct cnl_field {
  nfa_node* node;
  struct cnl_field* next;
} cnl_field;

int char_match(char matcher, char source);
int run_nfa(nfa_node* nfa_init, const char* regex, const char* text);
cnl_field* insert_node_into_cnl(cnl_field* start, nfa_node* node_ptr);
cnl_field* remove_node_from_cnl(cnl_field* start, cnl_field* rm_ptr);
nfa_node* generate_nfa(const char* regex);
void free_nfa(nfa_node* nfa_init);
void free_cnl(cnl_field* start);

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

  nfa_node* nfa = generate_nfa(regex);
  char buf[BUFLEN];
  while (--num_in >= 0) {
    while (fgets(buf, BUFLEN, inputs[num_in])) {
      buf[strlen(buf) - 1] = '\0'; // get rid of newline at the end
      if (run_nfa(nfa, regex, buf)) {
        printf("%s\n", buf);
      }
    }
  }
  free_nfa(nfa);
}


int char_match(char matcher, char source)
{
  return (matcher == '.' || matcher == source);
}

int run_nfa(nfa_node* nfa_init, const char* regex, const char* text)
{
  cnl_field* cnl = insert_node_into_cnl(NULL, nfa_init);
  cnl_field* tmp;
  cnl_field* next;
  nfa_node* curr_node;

  // note that cnl can't be NULL at first iteration since there will always
  // be at least one node in the NFA even for an empty regex
  do {
    for (tmp = cnl; tmp != NULL; tmp = next) {
      curr_node = tmp->node;
      next = tmp->next; // store next node of cnl here so that even after
                        // removing the current node we still progress with
                        // the correct next node
      if (curr_node->end) {
        free_cnl(cnl);
        return 1;
      }

      // loop over all edges leading to next nodes
      if (char_match(curr_node->next_ch, text[0])) {
        cnl = insert_node_into_cnl(cnl, curr_node->next_node);
        if (curr_node->next_node->type == END) {
          free_cnl(cnl);
          return 1;
        }
      }
      if ((curr_node->type == INIT && curr_node->ch == '^') ||
          (curr_node->type == STAR && !(char_match(curr_node->ch, text[0]))) ||
          (curr_node->type == NORM)) {
        cnl = remove_node_from_cnl(cnl, tmp);
      }
//       if (curr_node->type == INIT) {
//         //fprintf(stderr, "init\n");
//         if (char_match(curr_node->next_ch, text[0])) {
//           cnl = insert_node_into_cnl(cnl, curr_node->next_node);
//         }
//         if (curr_node->ch == '^') {
//           cnl = remove_node_from_cnl(cnl, tmp);
//         } // otherwise stay at current node
//       } else if (curr_node->type == END) {
//         free_cnl(cnl);
//         return 1;
//       } else if (curr_node->type == STAR) {
//         if (char_match(curr_node->next_ch, text[0])) {
//           cnl = insert_node_into_cnl(cnl, curr_node->next_node);
//         }
//         // only get rid of current state if we can't go back to current state
//         if (!(char_match(curr_node->ch, text[0]))) {
//           cnl = remove_node_from_cnl(cnl, tmp);
//         }
//       } else if (curr_node->type = NORM) {
//         if (char_match(curr_node->next_ch, text[0])) {
//           cnl = insert_node_into_cnl(cnl, curr_node->next_node);
//         }
//         cnl = remove_node_from_cnl(cnl, tmp);
//       }
    }
  } while (*(text++) != '\0' && cnl != NULL);

  free_cnl(cnl);
  return 0;
}

cnl_field* insert_node_into_cnl(cnl_field* start, nfa_node* node_ptr)
{
	cnl_field* ins = malloc(sizeof(cnl_field));
	ins->node = node_ptr;
	ins->next = start;
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

nfa_node* generate_nfa(const char* regex)
{
  nfa_node* first;
  nfa_node* curr;
  nfa_node* tmp;

  first = malloc(sizeof(nfa_node));
  first->type = INIT;
  first->ch = '\0'; // is don't care if it is not ^
  // first->next_ch is a don't care as long as we don't look at the next
  // character in regex
  first->next_node = NULL;
  if (regex[0] == '^') {
    first->ch = '^';
    regex++;
  }
  curr = first;

  for (; regex[0] != '\0'; regex++) {
    tmp = malloc(sizeof(nfa_node)); 
    if (regex[1] == '*') {
      tmp->type = STAR;
      regex++;
      if (regex[1] == '\0')
        curr->type = END;
    } else if (regex[1] == '\0') {
      tmp->type = END;
    } else {
      tmp->type = NORM;
    }
    tmp->ch = (regex[0] == '$') ? '\0' : regex[0];
    curr->next_ch = tmp->ch;
    curr->next_node = tmp;
    tmp->next_node = NULL;
    curr = tmp;
  }

  return first;
}

// deallocate memory for nfa
void free_nfa(nfa_node* nfa_init)
{
  nfa_node* next;
  for (; nfa_init != NULL; nfa_init = next) {
    next = nfa_init->next_node;
    free(nfa_init);
  }
}

void free_cnl(cnl_field* start)
{
  cnl_field* next;
  for (; start != NULL; start = next) {
    next = start->next;
    free(start);
  }
}


