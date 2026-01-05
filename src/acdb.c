#include "acdb.h"
#include "config.h"

#include <iowow/iwxstr.h>
#include <iowow/iwlog.h>
#include <iowow/iwp.h>
#include <iowow/iwutils.h>
#include <iwnet/iwn_proc.h>

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

struct env g_env = { 0 };

static void _destroy(void) {
  iwpool_destroy(g_env.pool);
}

static void _usage(const char *err) {
  if (err) {
    fprintf(stderr, "\nError:\n\n");
    fprintf(stderr, "\t%s\n\n", err);
  }
  fprintf(stderr, "\n\tClang's C/C++ compile_commands.json generator for Arduino projects\n");
  fprintf(stderr, "\n%s [options] [sketch directory]\n", g_env.program);
  fprintf(stderr, "\t-a, --arduino-cli=<>\tPath to the arduino-cli binary.\n");
  fprintf(stderr, "\t-b, --fqbn=<>\t\tFully Qualified Board Name, e.g.: arduino:avr:uno\n");
  fprintf(stderr, "\t-V, --verbose\t\tPrint verbose output.\n");
  fprintf(stderr, "\t-v, --version\t\tShow program version.\n");
  fprintf(stderr, "\t-h, --help\t\tPrint usage help.\n");
  fprintf(stderr, "\n");
};

static void _on_signal(int signo) {
  if (g_env.verbose) {
    fprintf(stderr, "\nExiting on signal: %d\n", signo);
  }
  if (g_env.poller) {
    iwn_poller_shutdown_request(g_env.poller);
  } else {
    exit(0);
  }
}

static bool _do_checks(void) {
  if (g_env.verbose) {
    iwlog_verbose("Sketch dir: %s", g_env.sketch_dir);
  }
  struct stat st;
  int rv = stat(g_env.sketch_dir, &st);
  if (rv == -1) {
    iwlog_ecode_error(iwrc_set_errno(IW_ERROR_IO_ERRNO, errno), "%s", g_env.sketch_dir);
    return false;
  }
  if (!S_ISDIR(st.st_mode)) {
    fprintf(stderr, "%s is not a directory\n", g_env.sketch_dir);
    return false;
  }
  return true;
}

iwrc run(void);

static int _main(int argc, char *argv[]) {
  int rv = EXIT_FAILURE;
  iwrc rc = 0;
  umask(0077);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGALRM, SIG_IGN);
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  if (  signal(SIGTERM, _on_signal) == SIG_ERR
     || signal(SIGINT, _on_signal) == SIG_ERR) {
    return rv;
  }
  RCC(rc, finish, iw_init());
  RCB(finish, g_env.pool = iwpool_create_empty());

  {
    char *buf = getcwd(0, 0);
    if (!buf) {
      rc = iwrc_set_errno(IW_ERROR_ERRNO, errno);
      goto finish;
    }

    g_env.cwd = iwpool_strdup2(g_env.pool, buf), free(buf);
    RCB(finish, g_env.cwd);
  }

  {
    char path[PATH_MAX];
    RCC(rc, finish, iwp_exec_path(path, sizeof(path)));
    RCB(finish, g_env.program_file = iwpool_strdup2(g_env.pool, path));
    g_env.program = argc ? argv[0] : "";
  }

  g_env.argc = argc;
  RCB(finish, g_env.argv = iwpool_alloc(argc * sizeof(char*), g_env.pool));
  for (int i = 0; i < argc; ++i) {
    RCB(finish, g_env.argv[i] = iwpool_strdup2(g_env.pool, argv[i]));
  }

  static const struct option long_options[] = {
    { "help",        0, 0, 'h' },
    { "verbose",     0, 0, 'V' },
    { "version",     0, 0, 'v' },
    { "arduino-cli", 1, 0, 'a' },
    { "fqbn",        1, 0, 'b' },
  };

  int ch;
  while ((ch = getopt_long(argc, argv, "a:b:c:hvV", long_options, 0)) != -1) {
    switch (ch) {
      case 'h':
        _usage(0);
        rv = EXIT_SUCCESS;
        goto finish;
      case 'v':
        fprintf(stdout, "%s\n", VERSION_FULL);
        goto finish;
      case 'V':
        g_env.verbose = true;
        break;
      case 'a':
        RCB(finish, g_env.arduino_cli_path = iwpool_strdup2(g_env.pool, optarg));
        break;
      case 'b':
        RCB(finish, g_env.fqbn = iwpool_strdup2(g_env.pool, optarg));
        break;
      default:
        _usage(0);
        goto finish;
    }
  }

  if (argv[optind]) {
    RCB(finish, g_env.sketch_dir = iwpool_strdup2(g_env.pool, argv[optind]));
  } else {
    g_env.sketch_dir = g_env.cwd;
  }

  if (!g_env.arduino_cli_path) {
    g_env.arduino_cli_path = "arduino-cli";
  }

  if (!_do_checks()) {
    goto finish;
  }

  RCC(rc, finish, iwn_poller_create(2, 1, &g_env.poller));
  iwn_poller_flags_set(g_env.poller, IWN_POLLER_POLL_NO_FDS);
  RCC(rc, finish, run());
  iwn_poller_poll(g_env.poller);

  rv = g_env.exit_code;

finish:
  if (rc) {
    iwlog_ecode_error3(rc);
  }
  iwn_poller_destroy(&g_env.poller);
  iwn_proc_dispose();
  _destroy();
  return rv;
}

#ifdef IW_EXEC

int main(int argc, char *argv[]) {
  return _main(argc, argv);
}

#endif
