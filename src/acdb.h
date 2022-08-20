#pragma once

#include <iowow/iwpool.h>
#include <iwnet/iwn_poller.h>

struct env {
  const char  *cwd;
  const char  *program;
  const char  *program_file;
  const char **argv;
  const char  *sketch_dir;
  const char  *arduino_cli_path;
  const char  *fqbn;

  struct iwn_poller *poller;

  IWPOOL *pool;
  int     argc;
  int     exit_code;
  bool    verbose;
};

extern struct env g_env;
