#ifndef PTY_WRAPPER_H
#define PTY_WRAPPER_H

struct PTYData_t {
  int failed; // non-zero in case of error
  int pid;
  int master_fd;
};
typedef struct PTYData_t PTYData;

PTYData fork_with_pty();

#endif