#include "pty_wrapper.h"

// Needed for posix_openpt:
#define _XOPEN_SOURCE 600
// Needed for cfmakeraw
#define _BSD_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

PTYData fork_with_pty() {
  PTYData pty_data;
  pty_data.failed = 1;

  // Open PTY
  int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
  pty_data.master_fd = master_fd;
  if (master_fd < 0) {
    fprintf(stderr, "posix_openpt: error %d\n", errno);
    return pty_data;
  }

  if (grantpt(master_fd)) {
    fprintf(stderr, "grantpt: error %d\n", errno);
    return pty_data;
  }

  if (unlockpt(master_fd)) {
    fprintf(stderr, "unlockpt: error %d\n", errno);
    return pty_data;
  }

  // Get PTY name
  const char* pts_name = ptsname(master_fd);

  // Open the slave PTY
  int slave_fd = open(pts_name, O_RDWR);

  int rv = fork();
  pty_data.pid = rv;
  if (rv < 0) {
    fprintf(stderr, "fork: error %d\n", errno);
    return pty_data;
  }

  if (rv) {
    //     Parent process

    // Close slave side
    close(slave_fd);
  } else {
    //     Child process

    // Close the master side of the PTY
    close(master_fd);

    // Change termios settings to RAW mode
    struct termios term_settings;
    if (tcgetattr(slave_fd, &term_settings)) {
      fprintf(stderr, "tcgetattr: error %d\n", errno);
      // But try to continue without changing termios
    } else {
      cfmakeraw(&term_settings);
      if (tcsetattr(slave_fd, TCSANOW, &term_settings)) {
        fprintf(stderr, "tcsetattr: error %d\n", errno);
      }
    }

    // Close stdin/stdout/stderr (parent fds)
    close(0);
    close(1);
    close(2);

    // Slave FD becomes stdin/stdout/stderr
    dup(slave_fd);
    dup(slave_fd);
    dup(slave_fd);
  }

  pty_data.failed = 0;
  return pty_data;
}