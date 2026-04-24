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
#include <stddef.h>
#include <dlfcn.h>
#include <assert.h>
#include <xalloc.h>
#include <error.h>
#include <exitfail.h>
#include <lib/gl_avltree_list.h>
#include <lib/gl_xlist.h>
#include <libhiha/initialize_once.h>
#include <libhiha/pratt.h>
#include <libhiha/load_plugin.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

typedef void plugin_init_func_t (void);
typedef plugin_init_func_t *plugin_init_t;

VISIBLE void
load_plugin (const char *filename, const char **error_message)
{
  *error_message = NULL;
  void *handle = dlopen (filename, RTLD_LAZY | RTLD_LOCAL);
  if (handle == NULL)
    *error_message = xstrdup (dlerror ());
  else
    {
      // Clear any errors.
      dlerror ();

      plugin_init_t plugin_init =
        (plugin_init_t) dlsym (handle, "plugin_init");

      // If there is no plugin_init() function, just ignore this
      // library.
      if (plugin_init != NULL)
        plugin_init ();
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
