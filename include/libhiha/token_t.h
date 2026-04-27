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

token_t make_token_t (string_t token_kind, string_t token_value,
                      text_location_t loc);
void serialize_token_t (const token_t tok, FILE *f);

struct token_getter;
typedef struct token_getter *token_getter_t;
struct token_getter
{
  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);
};

token_getter_t make_token_getter_from_source_file_t
  (const char *filename, FILE *f);

token_getter_t make_token_getter_from_serialized_tokens_t
  (const char *filename, FILE *f);

struct buffered_token_getter;
typedef struct buffered_token_getter *buffered_token_getter_t;
struct buffered_token_getter
{
  void (*get_token) (buffered_token_getter_t this_struct,
                     token_t *tok, const char **error_message);
  void (*look_at_token) (buffered_token_getter_t this_struct, size_t,
                         token_t *tok, const char **error_message);
};

buffered_token_getter_t make_buffered_token_getter_t
  (token_getter_t unbuffered_getter);

/* Play the files in sequence, converting all the EOF but the last to
   formfeed. */
buffered_token_getter_t make_buffered_token_getter_from_source_files
  (size_t n, const char *filenames[n]);

buffered_token_getter_t
  make_buffered_token_getter_from_serialized_tokens
  (const char *filename, FILE *f);

/* The first argument to check_for_mismatch MUST be the value that was
   returned as *output_getter. The second argument should be either a
   token outputted by the Pratt parser, to include it in the check
   whether there are any mismatches between inputs and outputs, or
   NULL, to check whether a mismatch has been detected. Thus we can
   achieve fixed point detection in lexical analysis. */
void make_token_getter_with_mismatch_check
  (buffered_token_getter_t input_getter,
   buffered_token_getter_t *output_getter,
   const bool (**check_for_mismatch) (buffered_token_getter_t,
                                      token_t));

struct token_putter;
typedef struct token_putter *token_putter_t;
struct token_putter
{
  void (*put_token) (token_putter_t this_struct,
                     token_t tok, const char **error_message);
};

token_putter_t make_token_putter_to_stream_serialized_t
  (const char *filename, FILE *f);

token_putter_t make_token_putter_with_mismatch_check
  (token_putter_t input_putter, buffered_token_getter_t output_getter,
   bool (*check_for_mismatch) (buffered_token_getter_t, token_t));

#endif /* __LIBHAHA__TOKEN_T_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
