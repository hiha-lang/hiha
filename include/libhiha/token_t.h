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

#ifndef __LIBHAHA__TOKEN_T_H__INCLUDED__
#define __LIBHAHA__TOKEN_T_H__INCLUDED__

#include <stdio.h>
#include <libhiha/string_t.h>

struct token
{
  string_t token_kind;
  string_t token_value;
  text_location_t loc;
};
typedef struct token *token_t;

struct token_getter;
typedef struct token_getter *token_getter_t;
struct token_getter
{
  void (*get_token) (token_getter_t this_struct,
		     token_t *tok, const char **error_message);
};

struct token_getter_from_file;
typedef struct token_getter_from_file *token_getter_from_file_t;

token_getter_from_file_t
make_token_getter_from_file_t (const char *filename, FILE *f);

#endif /* __LIBHAHA__TOKEN_T_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
