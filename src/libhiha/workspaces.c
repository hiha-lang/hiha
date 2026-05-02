/*

  The hiha programming language

  Copyright © 2026 Barry Schwartz

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <config.h>
#include <ftw.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <exitfail.h>
#include <xalloc.h>
#include <libhiha/initialize_once.h>
#include <libhiha/workspaces.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static initialize_once_t _work_directory_init1t =
  INITIALIZE_ONCE_T_INIT;
static const char *_work_directory = NULL;

static int
_remove_work_directory_callback (const char *fpath,
                                 const struct stat *sb, int typeflag,
                                 struct FTW *ftwbuf)
{
  return remove (fpath);
}

static void
_remove_work_directory (void)
{
  nftw (_work_directory, _remove_work_directory_callback, 64,
        FTW_DEPTH | FTW_PHYS);
}

static void
_initialize_work_directory (void)
{
  //
  // FIXME: Allow an alternate TMPDIR.
  //
  char template[] = "/tmp/hiha.XXXXXX";
  char *dirname = mkdtemp (template);
  int err_number = errno;
  if (dirname == NULL)
    {
      error (exit_failure, err_number,
             "failed to make temporary directory");
      abort ();
    }
  _work_directory = xstrdup (dirname);
  atexit (_remove_work_directory);
}

HIHA_VISIBLE HIHA_PURE const char *
work_directory (void)
{
  INITIALIZE_ONCE (_work_directory_init1t, _initialize_work_directory);
  return _work_directory;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
