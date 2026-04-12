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

#ifndef __LIBHAHA__STRING_T_H__INCLUDED__
#define __LIBHAHA__STRING_T_H__INCLUDED__

#include <stdio.h>
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

struct text_location
{
  const char *filename;		/* Do not free this. */

  /* If either line_no or code_point_no is zero, it means that field
     should be ignored. */
  size_t line_no;		/* Starting at 1. */
  size_t code_point_no;		/* Starting at 1, after canonicalization. */
};
typedef struct text_location *text_location_t;

/* Some string constants. */
extern string_t string_t_EOF;	/* “EOF” */
extern string_t string_t_CODE_POINT;	/* “CODE_POINT” */

int string_t_cmp (const string_t str1, const string_t str2);

string_t make_string_t (const char *src);
string_t copy_string_t (const string_t str);
char *make_str_nul (const string_t str);

string_t string_t_from_str_len (const char *src, size_t srclen,
				text_location_t loc);
string_t string_t_canonicalize (const string_t src,
				text_location_t loc);
string_t string_t_canonical_from_str_len (const char *src,
					  size_t srclen,
					  text_location_t loc);

void str_len_from_string_t (const string_t src, char **s, size_t *n);

char *text_location_string (text_location_t);

void print_string_t (const string_t str, FILE *f);

#endif /* __LIBHAHA__STRING_T_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
