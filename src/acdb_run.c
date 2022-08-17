#include "acdb.h"

#include <iowow/iwlog.h>
#include <iowow/iwp.h>
#include <iowow/iwpool.h>
#include <iwnet/iwn_proc.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct run {
  IWPOOL *pool;
};

//  arduino-cli compile --only-compilation-database --build-path ./b

iwrc run(void) {
  iwrc rc = 0;
  struct run run = { 0 };
  run.pool = iwpool_create_empty();
  if (!run.pool) {
    return iwrc_set_errno(IW_ERROR_ALLOC, errno);
  }
  char *cdir = iwp_allocate_tmpfile_path("acdb-");
  if (!cdir) {
    iwpool_destroy(run.pool);
    return iwrc_set_errno(IW_ERROR_ERRNO, errno);
  }



  
  iwpool_destroy(run.pool);
  free(cdir);
  return rc;
}
