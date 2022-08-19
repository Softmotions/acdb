#include "acdb.h"

#include <iowow/iwlog.h>
#include <iowow/iwp.h>
#include <iowow/iwpool.h>
#include <iowow/iwxstr.h>
#include <iwnet/iwn_proc.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct run {
  char   *cdir;
  IWXSTR *xstr;
  IWPOOL *pool;
};

static void _run_destroy(struct run *run) {
  if (run) {
    if (run->cdir) {
      iwp_removedir(run->cdir);
      free(run->cdir);
    }
    iwxstr_destroy(run->xstr);
    iwpool_destroy(run->pool);
  }
}

static void _cli_on_stdout(const struct iwn_proc_ctx *ctx, const char *buf, size_t len) {
  fprintf(stdout, "[arduino-cli] %.*s", (int) len, buf);
}

static void _cli_on_stderr(const struct iwn_proc_ctx *ctx, const char *buf, size_t len) {
  fprintf(stderr, "[arduino-cli-err] %.*s", (int) len, buf);
}

static void _cli_on_exit(const struct iwn_proc_ctx *ctx) {
  int code = WIFEXITED(ctx->wstatus) ? WEXITSTATUS(ctx->wstatus) : -1;
  struct run *run = ctx->user_data;

  if (code != 0) {
    fprintf(stderr, "arduino-cli failed, exit code: %d aborting..\n", code);
    goto finish;
  }

finish:
  _run_destroy(run);
  iwn_poller_shutdown_request(g_env.poller);
  return;
}

iwrc run(void) {
  iwrc rc = 0;
  pid_t pid;
  struct run *run = 0;
  IWPOOL *pool = 0;

  RCB(finish, pool = iwpool_create_empty());
  RCB(finish, run = iwpool_calloc(sizeof(*run), pool));
  run->pool = pool;
  RCB(finish, run->xstr = iwxstr_new());

  run->cdir = iwp_allocate_tmpfile_path("acdb-");
  if (!run->cdir) {
    rc = iwrc_set_errno(IW_ERROR_ERRNO, errno);
    goto finish;
  }

  //  arduino-cli compile --only-compilation-database --build-path ./b
  iwxstr_cat2(run->xstr,
              "compile"
              "\1--only-compilation-database"
              "\1--output-dir");
  iwxstr_printf(run->xstr, "\1%s", run->cdir);
  if (g_env.fqbn) {
    iwxstr_cat2(run->xstr, "\1--fqbn");
    iwxstr_printf(run->xstr, "\1%s", g_env.fqbn);
  }
  if (g_env.sketch_dir) {
    iwxstr_printf(run->xstr, "\1%s", g_env.sketch_dir);
  }

  struct iwn_proc_spec spec = {
    .poller                  = g_env.poller,
    .path                    = g_env.arduino_cli_path,
    .args                    = iwpool_split_string(run->pool,iwxstr_ptr(run->xstr),"\1", true),
    .on_stderr               = _cli_on_stderr,
    .on_stdout               = _cli_on_stdout,
    .on_exit                 = _cli_on_exit,
    .find_executable_in_path = true,
    .user_data               = run
  };


  if (g_env.verbose) {
    char *cmd = iwn_proc_command_get(&spec);
    iwlog_verbose("Spawn: %s", cmd);
    free(cmd);
  }

  RCC(rc, finish, iwn_proc_spawn(&spec, &pid));

finish:
  if (rc) {
    iwlog_ecode_error3(rc);
    if (!run) {
      iwpool_destroy(pool);
    } else {
      _run_destroy(run);
    }
  }
  return rc;
}
