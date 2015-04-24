#include <stdio.h>
#include <string.h>

/** Go from "[PING]\r\n" to "PING". */
static char* process_line(char* line) {
  if (line[0] != '[') return NULL;
  char* end = strchr(line, 0);
  end--;
  if (*end == '\n') end--;
  if (*end == '\r') end--;
  if (*end != ']') return NULL;
  *end = 0;
  return line+1;
}

static void handle_command(int argc, char** argv) {
  if (argc == 0) return;

  if (strcmp(argv[0], "PING") == 0) {
    fputs("[PONG]\n", stdout);
    fflush(stdout);
  }
}

#define ARGV_MAX 8
static void handle_line(char* line) {
  int argc = 0;
  char* argv[ARGV_MAX];
  char* cur = line;
  while (*cur == ' ') {
    cur++;
  }

  for (int arg = 0; arg < ARGV_MAX; arg++) {
    argc++;
    argv[arg] = cur;
    while (*cur != 0 && *cur != ' ') {
      cur++;
    }
    if (*cur == ' ') {
      *cur = 0;
      cur++;
      while (*cur == ' ') {
        *cur = 0;
        cur++;
      }
    }

    if (*cur == 0) {
      break;
    }
  }

  handle_command(argc, argv);
}

#define LINE_MAX 8192

int main(int argc, const char** argv) {
  char line[LINE_MAX];

  while (fgets(line, LINE_MAX, stdin) != NULL) {
    line[LINE_MAX-1] = 0;

    // processed_line is a string inside line[LINE_MAX].
    char* processed_line = process_line(line);

    if (processed_line != NULL) {
      handle_line(processed_line);
    }
  }
}