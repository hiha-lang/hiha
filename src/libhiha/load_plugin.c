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
#include <xalloc.h>
#include <libhiha/load_plugin.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

VISIBLE void
load_plugin (const char *filename, const char **error_message)
{
  *error_message = NULL;
  void *handle = dlopen (filename, RTLD_LAZY | RTLD_LOCAL);
  if (handle == NULL)
    {
      *error_message = xstrdup (dlerror ());
    }
  else
    {
      dlerror ();
      void (*plugin_init) (void) =
        (void (*)(void)) dlsym (handle, "plugin_init");
      plugin_init ();
      dlclose (handle);
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
