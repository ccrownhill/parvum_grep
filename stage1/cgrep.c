#include <stdio.h>
#include <string.h>

#define BUFLEN 200
#define MAXINPUTS 10

int match(const char* regex, const char* text);
int matchhere(const char* regex, const char* text);
int matchstar(char c, const char* regex, const char* text);

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
  char buf[BUFLEN];
  while (--num_in >= 0) {
    while (fgets(buf, BUFLEN, inputs[num_in])) {
      buf[strlen(buf) - 1] = '\0'; // get rid of newline at the end
      if (match(regex, buf)) {
        printf("%s\n", buf);
      }
    }
  }
}

// this will match the following regular expressions
// ^ for beginning of input
// $ for end of input
// c for character c
// . for any character
// * for 0 or more repetitions of previous character
int match(const char* regex, const char* text)
{
  if (regex[0] == '^') {
    return matchhere(regex + 1, text);
  } else {
    // check from every possible starting position
    do {
      if (matchhere(regex, text))
        return 1;
    } while (*text++ != '\0'); // note that we still check if the text is just
                               // a '\0' character because this could be
                               // matched by '$'
    return 0;
  }
}

int matchhere(const char* regex, const char* text)
{
  if (regex[0] == '\0') {
    return 1; 
  }
  if (regex[0] == '$' && regex[1] == '\0') {
    return (text[0] == '\0');
  }
  if (strlen(regex) > 1 && regex[1] == '*') {
    return matchstar(regex[0], regex+2, text);
  }
  if (text[0] != '\0' && (regex[0] == '.' || regex[0] == text[0])) {
    return matchhere(regex + 1, text + 1);
  }
  return 0;
}

int matchstar(char c, const char* regex, const char* text) {
  while (!(matchhere(regex, text))) {
    if ((c != '.' && text[0] != c) || text[0] == '\0')
      return 0;
    text++; 
  }
  return matchhere(regex, text);
}
