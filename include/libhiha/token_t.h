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

#include <libhiha/string_t.h>

// FIXME: THIS IS FOR NOW. A FUTURE VERSION (probably written in the
// language itself) might have a scanner that lets you add things to
// it.
enum token_kind_t
{
  TOK_INTEGER,
  TOK_IDENTIFIER,
  TOK_PLUS,
  TOK_MINUS,
  TOK_TIMES,
  TOK_DIVIDE
};

typedef enum token_kind_t token_kind_t;

struct token_t
{
  token_kind_t token_kind;
  string_t token_value;
};

typedef struct token_t token_t;

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
