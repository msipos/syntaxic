#include <errno.h>
#include <stdio.h>
#include "pty_wrapper.h"
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

int child_running;

void signal_handler(int sig) {
  if (sig == SIGCHLD) {
    wait(NULL);
    child_running = 0;
  }
}

int main(int argc, char** argv) {
  if (argc <= 1) return 1;

  PTYData pty_data = fork_with_pty();

  if (pty_data.failed) {
    return 1;
  }

  if (pty_data.pid > 0) {

    // Parent process:

    child_running = 1;
    signal(SIGCHLD, signal_handler);

    #define BUFFER_LENGTH 1024
    char buffer[BUFFER_LENGTH];
    fd_set read_fds;

    for(;;) {
      FD_ZERO(&read_fds);
      FD_SET(STDIN_FILENO, &read_fds);
      FD_SET(pty_data.master_fd, &read_fds);

      int fd = select(pty_data.master_fd+1, &read_fds, NULL, NULL, NULL);
      if (fd == -1) {
        fprintf(stderr, "Error on select: %d\n", errno);
        return 1;
      }

      if (FD_ISSET(STDIN_FILENO, &read_fds)) {
        int num_read = read(STDIN_FILENO, buffer, BUFFER_LENGTH);
        if (num_read <= 0) {
          if (num_read == 0) return 0;
          fprintf(stderr, "Error on stdin read: %d\n", errno);
          return 1;
        }

        int num_written = write(pty_data.master_fd, buffer, num_read);
        if (num_written < num_read) {
          fprintf(stderr, "Error on pty write: %d\n", errno);
          return 1;
        }
      } else if (FD_ISSET(pty_data.master_fd, &read_fds)) {
        int num_read = read(pty_data.master_fd, buffer, BUFFER_LENGTH);
        if (num_read <= 0) {
          // Sometimes closing the slave side of the PTY makes an IO error here (errno 5).
          if (num_read == 0 || child_running == 0 || errno == EIO) return 0;
          fprintf(stderr, "Error on pty read: %d\n", errno);
          return 1;
        }

        int num_written = write(STDOUT_FILENO, buffer, num_read);
        if (num_written < num_read) {
          fprintf(stderr, "Error on stdout write: %d\n", errno);
          return 1;
        }
      }
    }
  } else {

    // Child process:

    execvp(argv[1], argv+1);
  }

  return 0;
}