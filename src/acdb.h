#pragma once

#include <iowow/iwpool.h>
#include <iwnet/iwn_poller.h>

struct env {
  const char  *cwd;
  const char  *program;
  const char  *program_file;
  const char **argv;
  const char  *sketch_dir;

  struct iwn_poller *poller;

  IWPOOL *pool;
  int     argc;
  bool    verbose;
};

extern struct env g_env;
