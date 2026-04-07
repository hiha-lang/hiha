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

#include <stdlib.h>
#include <stdint.h>
#include <unistr.h>
#include <unicase.h>
#include <uniconv.h>
#include <uninorm.h>

struct string
{
  uint32_t *s;
  size_t n;
};
typedef struct string *string_t;

struct error_location_reporter
{
  void (*func) (struct error_location_reporter *);

  // Extend the structure with whatever data is needed, such as file
  // path, line number, and so on.
};
typedef struct error_location_reporter *error_location_reporter_t;

void string_t_free (string_t str);
int string_t_cmp (const string_t str1, const string_t str2);

string_t string_t_from_str_len (const char *src, size_t srclen,
				error_location_reporter_t errloc);
string_t string_t_canonicalize (const string_t src,
				error_location_reporter_t errloc);
string_t string_t_canonical_from_str_len (const char *src,
					  size_t srclen,
					  error_location_reporter_t
					  errloc);

void str_len_from_string_t (const string_t src, char **s, size_t *n);

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
