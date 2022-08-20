#include "acdb.h"

#include <iowow/iwlog.h>
#include <iowow/iwp.h>
#include <iowow/iwpool.h>
#include <iowow/iwxstr.h>
#include <iowow/iwutils.h>
#include <iowow/iwjson.h>
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
  if (g_env.verbose) {
    fprintf(stdout, "[arduino-cli] %.*s", (int) len, buf);
  }
}

static void _cli_on_stderr(const struct iwn_proc_ctx *ctx, const char *buf, size_t len) {
  fprintf(stderr, "[arduino-cli-err] %.*s", (int) len, buf);
}

static void _compile_commands_gen(struct run *run) {
  iwrc rc = 0;

  IWPOOL *pool = 0;
  IWXSTR *xstr = 0;
  FILE *file = 0;
  JBL_NODE data, n;
  char *buf = 0;

  RCB(finish, pool = iwpool_create_empty());
  RCB(finish, xstr = iwxstr_new());
  RCC(rc, finish, iwxstr_printf(xstr, "%s/compile_commands.json", run->cdir));

  buf = iwu_file_read_as_buf(iwxstr_ptr(xstr));
  if (!buf) {
    rc = iwrc_set_errno(IW_ERROR_IO_ERRNO, errno);
    goto finish;
  }

  RCC(rc, finish, jbn_from_json(buf, &data, pool));
  free(buf), buf = 0;

  if (!data || data->type != JBV_ARRAY) {
    iwlog_error2("Failed to parse compile_commands.json");
    rc = IW_ERROR_FAIL;
    goto finish;
  }

  iwxstr_clear(xstr);
  RCC(rc, finish, iwxstr_printf(xstr, "%s/sketch/", run->cdir));

  const char *prefix = iwxstr_ptr(xstr);
  size_t prefix_len = iwxstr_size(xstr);

  for (JBL_NODE nn = data->child; nn; nn = nn->next) {
    jbn_at(nn, "/file", &n);
    if (n && n->type == JBV_STR && !strncmp(n->vptr, prefix, prefix_len)) {
      IWXSTR *npath = iwxstr_new();
      char *fn = (char*) n->vptr;
      char *c = strrchr(fn, '.');
      if (npath && c) {
        *c = 0;
        fn += prefix_len;
        iwxstr_printf(npath, "%s/%s", g_env.sketch_dir, fn);
        n->vptr = iwpool_strndup2(pool, iwxstr_ptr(npath), iwxstr_size(npath));
        n->vsize = iwxstr_size(npath);
      }
      iwxstr_destroy(npath);
    }
  }

  iwxstr_clear(xstr);
  RCC(rc, finish, iwxstr_printf(xstr, "%s/compile_commands.json", g_env.sketch_dir));
  file = fopen(iwxstr_ptr(xstr), "w+");
  if (!file) {
    iwlog_error("Write failed: %s", iwxstr_ptr(xstr));
    rc = iwrc_set_errno(IW_ERROR_IO_ERRNO, errno);
    goto finish;
  }

  iwxstr_clear(xstr);
  RCC(rc, finish, jbn_as_json(data, jbl_xstr_json_printer, xstr, JBL_PRINT_PRETTY));
  if (fwrite(iwxstr_ptr(xstr), iwxstr_size(xstr), 1, file) != 1) {
    iwlog_error("Write failed: %s", iwxstr_ptr(xstr));
    rc = iwrc_set_errno(IW_ERROR_IO_ERRNO, errno);
    goto finish;
  }


finish:
  if (rc) {
    iwlog_ecode_error3(rc);
    g_env.exit_code = EXIT_FAILURE;
  }
  iwxstr_destroy(xstr);
  iwpool_destroy(pool);
  free(buf);
  if (file) {
    fclose(file);
  }
}

static void _cli_on_exit(const struct iwn_proc_ctx *ctx) {
  int code = WIFEXITED(ctx->wstatus) ? WEXITSTATUS(ctx->wstatus) : -1;
  struct run *run = ctx->user_data;

  if (g_env.verbose) {
    iwlog_verbose("arduino-cli exited: %d", code);
  }

  if (code != 0) {
    fprintf(stderr, "arduino-cli failed, exit code: %d aborting..\n", code);
    g_env.exit_code = code;
    goto finish;
  }

  _compile_commands_gen(run);

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
              "\1--build-path");
  iwxstr_printf(run->xstr, "\1%s", run->cdir);
  if (g_env.fqbn) {
    iwxstr_cat2(run->xstr, "\1--fqbn");
    iwxstr_printf(run->xstr, "\1%s", g_env.fqbn);
  }
  iwxstr_printf(run->xstr, "\1%s", g_env.sketch_dir);

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
