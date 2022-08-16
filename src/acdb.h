#pragma once

#include <iowow/iwpool.h>

struct env {
  const char  *cwd;
  const char  *program;
  const char  *program_file;
  const char  *config_file;
  const char  *config_file_dir;
  const char **argv;
  IWPOOL      *pool;
  int  argc;
  bool verbose;
};

extern struct env g_env;
