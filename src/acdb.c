#include "acdb.h"
#include "config.h"

#include <iowow/iwxstr.h>
#include <iowow/iwlog.h>
#include <iowow/iwp.h>
#include <iowow/iwutils.h>
#include <iowow/iwini.h>

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  fprintf(stderr, "\n\tACDB\n");
  fprintf(stderr, "\nUsage %s [options]\n", g_env.program);
  fprintf(stderr, "\t-c, --conf=<>\t\t.ini configuration file\n");
  fprintf(stderr, "\t-V, --verbose\t\tPrint verbose output\n");
  fprintf(stderr, "\t-v, --version\t\tShow program version\n");
  fprintf(stderr, "\t-h, --help\t\tPrint usage help\n");
  fprintf(stderr, "\n");
};

static void _on_signal(int signo) {
  if (g_env.verbose) {
    fprintf(stderr, "\nExiting on signal: %d\n", signo);
  }
  // iwn_poller_shutdown_request(g_env.poller);
  exit(0);
}

static int _ini_handler(void *user_data, const char *section, const char *name, const char *value) {
  iwrc rc = 0;
  struct init *init = user_data;
  iwlog_info("%s:%s=%s", section, name, value);
  if (!section || !strcmp(section, "main")) {
    if (!strcmp(name, "verbose")) {
      IWINI_PARSE_BOOL(g_env.verbose);
    }
  }
  return rc == 0;
}

static const char* _replace_config_env(const char *key, void *op) {
  if (!strcmp(key, "/home/adam")) {
    return getenv("HOME");
  } else if (!strcmp(key, "{cwd}")) {
    return g_env.cwd;
  }
  return 0;
}

static iwrc _config_load(void) {
  iwrc rc = 0;
  IWXSTR *xstr = 0;
  char *data = 0;
  size_t len;

  RCB(finish, g_env.config_file_dir = iwpool_strdup2(g_env.pool, g_env.config_file));
  g_env.config_file_dir = iwp_dirname((char*) g_env.config_file_dir);

  data = iwu_file_read_as_buf_len(g_env.config_file, &len);
  if (!data) {
    rc = IW_ERROR_FAIL;
    iwlog_error("Error reading configuration file: %s", g_env.config_file);
    goto finish;
  }

  static const char *config_keys[] = { "/home/adam", "{cwd}" };
  RCC(rc, finish, iwu_replace(&xstr, data, len,
                              config_keys, sizeof(config_keys) / sizeof(config_keys[0]),
                              _replace_config_env, 0));

  rc = iwini_parse_string(iwxstr_ptr(xstr), _ini_handler, 0);

finish:
  return rc;
}

iwrc run(void);

static int _main(int argc, char *argv[]) {
  int rv = EXIT_SUCCESS;
  iwrc rc = 0;
  umask(0077);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGALRM, SIG_IGN);
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);

  if (  signal(SIGTERM, _on_signal) == SIG_ERR
     || signal(SIGINT, _on_signal) == SIG_ERR) {
    return EXIT_FAILURE;
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
    { "help",    0, 0, 'h' },
    { "verbose", 0, 0, 'V' },
    { "version", 0, 0, 'v' },
    { "conf",    1, 0, 'c' }
  };

  int ch;
  while ((ch = getopt_long(argc, argv, "c:hvV", long_options, 0)) != -1) {
    switch (ch) {
      case 'h':
        _usage(0);
        goto finish;
      case 'v':
        fprintf(stdout, "%s\n", VERSION_FULL);
        goto finish;
      case 'V':
        g_env.verbose = true;
        break;
      case 'c':
        g_env.config_file = iwpool_strdup2(g_env.pool, optarg);
        break;
      default:
        _usage(0);
        rv = EXIT_FAILURE;
        goto finish;
    }
  }

  if (g_env.config_file) {
    RCC(rc, finish, _config_load());
  }

  rc = run();

finish:
  _destroy();
  if (rc) {
    return EXIT_FAILURE;
  }
  return rv;
}

#ifdef IW_EXEC

int main(int argc, char *argv[]) {
  return _main(argc, argv);
}

#endif
