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

static initialize_once_t _plugins_init1t = INITIALIZE_ONCE_T_INIT;
static gl_list_t _plugins = NULL;

static void
_initialize_plugins (void)
{
  _plugins =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
}

static gl_list_t
plugins (void)
{
  INITIALIZE_ONCE (_plugins_init1t, _initialize_plugins);
  return _plugins;
}

static size_t
next_register_no (void)
{
  return gl_list_size (plugins ());
}

static void
put_plugin_in_list (volatile plugin_interface_t interface)
{
  if (interface->register_no < next_register_no ())
    gl_list_set_at (plugins (), interface->register_no, interface);
  else
    gl_list_add_last (plugins (), interface);
}

static void
initialize_pratt_handlers (volatile plugin_interface_t interface)
{
  size_t i;
  void *plugin_handle = dlopen (interface->filename, RTLD_LAZY | RTLD_LOCAL);
  if (plugin_handle == NULL)
    {
      error (exit_failure, 0, "%s", xstrdup (dlerror ()));
      abort ();
    }
  dlerror ();                   /* Clear any errors. */
  plugin_pratt_interface_t pi = (plugin_pratt_interface_t) interface->interface;
  if (pi->nud_handlers != NULL)
    for (i = 0; pi->nud_handlers[i] != NULL; i += 1)
      pi->nud_handlers[i] = dlsym (plugin_handle, pi->nud_handlers[i]);
  if (pi->led_handlers != NULL)
    for (i = 0; pi->led_handlers[i] != NULL; i += 1)
      pi->led_handlers[i] = dlsym (plugin_handle, pi->led_handlers[i]);
  //dlclose (plugin_handle);
}

VISIBLE void
register_plugin (volatile plugin_interface_t interface)
{
  assert (interface->register_no <= next_register_no ());
  put_plugin_in_list (interface);
  switch (interface->tag)
    {
    case tag_PLUGIN_PRATT_INTERFACE:
      initialize_pratt_handlers (interface);
      break;
    }
}

VISIBLE volatile plugin_interface_t
plugin (size_t register_no)
{
  return (volatile plugin_interface_t)
    gl_list_get_at (plugins (), register_no);
}

VISIBLE nud_handler_t
plugin_nud_handler (size_t register_no, size_t handler_no)
{
  plugin_interface_t pi = plugin (register_no);
  assert (pi->tag == tag_PLUGIN_PRATT_INTERFACE);
  return ((plugin_pratt_interface_t) pi->
          interface)->nud_handlers[handler_no];
}

VISIBLE led_handler_t
plugin_led_handler (size_t register_no, size_t handler_no)
{
  plugin_interface_t pi = plugin (register_no);
  assert (pi->tag == tag_PLUGIN_PRATT_INTERFACE);
  return ((plugin_pratt_interface_t) pi->
          interface)->led_handlers[handler_no];
}

typedef plugin_interface_t plugin_init_func_t (const char *, size_t);
typedef plugin_init_func_t *plugin_init_t;

VISIBLE void
load_plugin (const char *filename,
             volatile plugin_interface_t *interface,
             const char **error_message)
{
  *interface = NULL;
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
        {
          const size_t register_no = next_register_no ();
          *interface = plugin_init (filename, register_no);
        }

      //dlclose (handle);
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
