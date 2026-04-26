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

#ifndef __LIBHAHA__LEXICAL_H__INCLUDED__
#define __LIBHAHA__LEXICAL_H__INCLUDED__

#include <libhiha/pratt.h>
#include <libhiha/token_t.h>

pratt_tables_t lexical_pratt_tables (void);

void scan_tokens (void *state, buffered_token_getter_t getter,
                  token_putter_t putter, const char **error_message);

#endif /* __LIBHAHA__LEXICAL_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
