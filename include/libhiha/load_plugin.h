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

#ifndef __LIBHAHA__LOAD_PLUGIN_H__INCLUDED__
#define __LIBHAHA__LOAD_PLUGIN_H__INCLUDED__

#include <stddef.h>
#include <libhiha/pratt.h>

struct plugin_pratt_interface
{
  void **nud_handlers;
  void **led_handlers;
};
typedef struct plugin_pratt_interface *plugin_pratt_interface_t;

enum plugin_interface_tag
{
  tag_PLUGIN_PRATT_INTERFACE
};
typedef enum plugin_interface_tag plugin_interface_tag_t;

struct plugin_interface
{
  const char *filename;    /* The plugin’s filename, for reöpening. */
  size_t register_no;
  plugin_interface_tag_t tag;
  volatile void *interface;
};
typedef struct plugin_interface *plugin_interface_t;

void load_plugin (const char *filename,
                  volatile plugin_interface_t * interface,
                  const char **error_message);

void register_plugin (volatile plugin_interface_t interface);
volatile plugin_interface_t plugin (size_t register_no);

nud_handler_t plugin_nud_handler (size_t register_no,
                                  size_t handler_no);
led_handler_t plugin_led_handler (size_t register_no,
                                  size_t handler_no);

#endif /* __LIBHAHA__LOAD_PLUGIN_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
