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
#include <stdint.h>
#include <inttypes.h>
#include <xalloc.h>
#include <unistr.h>
#include <unictype.h>
#include <libhiha/token_t.h>
#include <libhiha/string_t.h>
#include <libhiha/string_literal.h>

#define _(msgid) HIHA_GETTEXT (msgid)

#define STORE_IF_NOT_NULL(dest, src)            \
  do                                            \
    {                                           \
      if ((dest) != NULL)                       \
        *(dest) = (src);                        \
    }                                           \
  while (0)

static void
look_at_token (buffered_token_getter_t getter, size_t i,
               token_t *tok, const char **error_message)
{
  getter->look_at_token (getter, i, tok, error_message);
  if (*error_message == NULL)
    if (string_t_cmp ((*tok)->token_kind, string_t_EOF ()) != 0)
      switch ((*tok)->token_value->n)
        {
        case 0:
          {
            // In place of an empty value, push back U+200B ZERO WIDTH
            // SPACE.
            token_t t;
            getter->get_token (getter, &t, error_message);
            if (*error_message == NULL)
              getter->push_back_string (getter, string_t_zerowidth (),
                                        t->loc);
          }
          break;
        case 1:
          // Do nothing.
          break;
        default:
          {
            token_t t;
            getter->get_token (getter, &t, error_message);
            if (*error_message == NULL)
              getter->push_back_string (getter, t->token_value, t->loc);
          }
          break;
        }
}

static const char *
eof_in_open_string (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: end of input in open string"),
            text_location_string (loc));
  return xstrdup (buf);
}

static const char *
unrecognized_escape (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: unrecognized string escape"),
            text_location_string (loc));
  return xstrdup (buf);
}

static const char *
bad_newline_escape (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: perhaps a bad newline escape"),
            text_location_string (loc));
  return xstrdup (buf);
}

static const char *
empty_hex_escape (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: empty hex escape in string"),
            text_location_string (loc));
  return xstrdup (buf);
}

static const char *
illegal_char_in_hex_escape (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: illegal character in hex escape"),
            text_location_string (loc));
  return xstrdup (buf);
}

static const char *
not_a_code_point (text_location_t loc, const char *hex)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: not a Unicode code point: \\x%s;"),
            text_location_string (loc), hex);
  return xstrdup (buf);
}

static bool
token_is_EOF (token_t tok)
{
  return (string_t_cmp (tok->token_kind, string_t_EOF ()) == 0);
}

static bool
token_is_char (token_t tok, int ch)
{
  return (tok->token_value->n == 1 && tok->token_value->s[0] == ch);
}

static bool
token_is_space_separator_or_tab (token_t tok)
{
  return (tok->token_value->n == 1
          && (uc_is_general_category (tok->token_value->s[0],
                                      UC_CATEGORY_Zs)
              || tok->token_value->s[0] == '\t'));

}

static bool
token_is_ascii_hex_digit (token_t tok)
{
  return (tok->token_value->n == 1
          && uc_is_property (tok->token_value->s[0],
                             UC_PROPERTY_ASCII_HEX_DIGIT));
}

static void
skip_blanks (buffered_token_getter_t getter, string_t *tokval,
             string_t *substring, const char **error_message)
{
  if (*error_message == NULL)
    {
      *substring = empty_string_t ();
      *tokval = empty_string_t ();
      token_t t;
      look_at_token (getter, 0, &t, error_message);
      while (*error_message == NULL
             && token_is_space_separator_or_tab (t))
        {
          getter->get_token (getter, &t, error_message);
          if (*error_message == NULL)
            {
              *tokval = concat_string_t (*tokval, t->token_value, NULL);
              look_at_token (getter, 0, &t, error_message);
            }
        }
      if (*error_message != NULL && token_is_EOF (t))
        *error_message = eof_in_open_string (t->loc);
    }
}

static void
skip_blanks_newline_blanks (buffered_token_getter_t getter,
                            string_t *tokval, string_t *substring,
                            const char **error_message)
{
  /* Skip blanks until a newline, and then skip more blanks. This is
     how backslash at the end of a line is handled is handled in
     R⁷RS Scheme, and is how we will handle it here. */

  skip_blanks (getter, tokval, substring, error_message);
  if (*error_message == NULL)
    {
      token_t t;
      look_at_token (getter, 0, &t, error_message);
      if (!token_is_char (t, '\n'))
        *error_message = bad_newline_escape (t->loc);
      else
        {
          getter->get_token (getter, &t, error_message);
          if (*error_message == NULL)
            {
              *tokval = concat_string_t (*tokval, t->token_value, NULL);
              string_t tokval2;
              string_t s;
              skip_blanks (getter, &tokval2, &s, error_message);
              *tokval = concat_string_t (*tokval, tokval2, NULL);
            }
        }
    }
}

static void
hex_ends_badly (token_t tok, const char **error_message)
{
  if (token_is_EOF (tok))
    *error_message = eof_in_open_string (tok->loc);
  else
    *error_message = illegal_char_in_hex_escape (tok->loc);
}

static void
scan_the_hex (token_t tok, string_t_vector_t v, uint32_t *code_point,
              const char **error_message)
{
  string_t str = empty_string_t ();
  for (size_t i = 0; i != string_t_vector_length (v); i += 1)
    str = concat_string_t (str, string_t_vector_ref (v, i), NULL);
  const char *s = make_str_nul (str);
  int num_scanned = sscanf (s, "%" SCNx32, code_point);
  if (num_scanned != 1)
    *error_message = not_a_code_point (tok->loc, s);
  else if (u32_check (code_point, 1) != NULL)
    *error_message = not_a_code_point (tok->loc, s);
}

static void
hex_ending_semicolon (buffered_token_getter_t getter,
                      string_t_vector_t v, string_t *tokval,
                      uint32_t *code_point, const char **error_message)
{
  token_t t;
  look_at_token (getter, 0, &t, error_message);
  if (*error_message == NULL)
    {
      if (token_is_char (t, ';'))
        {
          getter->get_token (getter, &t, error_message);
          if (*error_message == NULL)
            {
              *tokval = concat_string_t (*tokval, t->token_value, NULL);
              if (string_t_vector_length (v) == 0)
                *error_message = empty_hex_escape (t->loc);
              else
                scan_the_hex (t, v, code_point, error_message);
            }
        }
      else
        hex_ends_badly (t, error_message);
    }
}

static void
hex_until_semicolon (buffered_token_getter_t getter, string_t *tokval,
                     uint32_t *code_point, const char **error_message)
{
  if (*error_message == NULL)
    {
      *tokval = empty_string_t ();
      *code_point = 0xFFFD;     /* U+FFFD REPLACEMENT CHARACTER */
      string_t_vector_t v = NULL;
      token_t t;
      look_at_token (getter, 0, &t, error_message);
      while (*error_message == NULL && token_is_ascii_hex_digit (t))
        {
          getter->get_token (getter, &t, error_message);
          if (*error_message == NULL)
            {
              *tokval = concat_string_t (*tokval, t->token_value, NULL);
              v = string_t_vector_push (v, t->token_value);
              look_at_token (getter, 0, &t, error_message);
            }
        }
      if (*error_message == NULL)
        hex_ending_semicolon (getter, v, tokval, code_point,
                              error_message);
    }
}

static void
simple_escape (buffered_token_getter_t getter,
               string_t *tokval, string_t *substring,
               const char **error_message, int new_char)
{
  token_t t;
  getter->get_token (getter, &t, error_message);
  if (*error_message == NULL)
    {
      *tokval = t->token_value;
      char buf[2];
      buf[0] = (char) new_char;
      buf[1] = '\0';
      *substring = make_string_t (buf);
    }
}

static void
escape_newline (buffered_token_getter_t getter, string_t *tokval,
                string_t *substring, const char **error_message)
{
  token_t t;
  getter->get_token (getter, &t, error_message);
  if (*error_message == NULL)
    {
      *tokval = t->token_value;
      string_t tokval2;
      skip_blanks (getter, &tokval2, substring, error_message);
      *tokval = concat_string_t (*tokval, tokval2, NULL);
    }
}

static void
escape_blank (buffered_token_getter_t getter, string_t *tokval,
              string_t *substring, const char **error_message)
{
  token_t t;
  getter->get_token (getter, &t, error_message);
  if (*error_message == NULL)
    {
      *tokval = t->token_value;
      string_t tokval2;
      skip_blanks_newline_blanks (getter, &tokval2, substring,
                                  error_message);
      *tokval = concat_string_t (*tokval, tokval2, NULL);
    }
}

static void
escape_x (buffered_token_getter_t getter, string_t *tokval,
          string_t *substring, const char **error_message)
{
  token_t t;
  getter->get_token (getter, &t, error_message);
  if (*error_message == NULL)
    {
      *tokval = t->token_value;
      string_t tokval2;
      uint32_t code_point;
      hex_until_semicolon (getter, &tokval2, &code_point,
                           error_message);
      *tokval = concat_string_t (*tokval, tokval2, NULL);
      if (*error_message == NULL)
        {
          struct string *str = XMALLOC (struct string);
          str->n = 1;
          str->s = XNMALLOC (1, uint32_t);
          str->s[0] = code_point;
          *substring = str;
        }
    }
}

static void
scan_escape (buffered_token_getter_t getter, string_t *tokval,
             string_t *substring, const char **error_message)
{
  token_t t;
  string_t tokval2;
  getter->get_token (getter, &t, error_message);
  if (*error_message == NULL)
    {
      *tokval = t->token_value;
      look_at_token (getter, 0, &t, error_message);
      if (token_is_EOF (t))
        *error_message = eof_in_open_string (t->loc);
      else if (token_is_space_separator_or_tab (t))
        {
          escape_blank (getter, &tokval2, substring, error_message);
          *tokval = concat_string_t (*tokval, tokval2, NULL);
        }
      else
        {
          switch (t->token_value->s[0])
            {
            case 'x':
              escape_x (getter, &tokval2, substring, error_message);
              break;
            case '\n':
              escape_newline (getter, &tokval2, substring,
                              error_message);
              break;
            case '"':
              simple_escape (getter, &tokval2, substring, error_message,
                             '"');
              break;
            case '\\':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\\');
              break;
            case '|':
              simple_escape (getter, &tokval2, substring, error_message,
                             '|');
              break;
            case 'a':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\a');
              break;
            case 'b':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\b');
              break;
            case 't':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\t');
              break;
            case 'n':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\n');
              break;
            case 'r':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\r');
              break;
            case 'v':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\013');
              break;
            case 'f':
              simple_escape (getter, &tokval2, substring, error_message,
                             '\014');
              break;
            default:
              *error_message = unrecognized_escape (t->loc);
              break;
            }
          *tokval = concat_string_t (*tokval, tokval2, NULL);
        }
    }
}

static void
scan_string_portion (buffered_token_getter_t getter, bool *done,
                     string_t *tokval, string_t *substring,
                     const char **error_message)
{
  *done = false;
  if (*error_message == NULL)
    {
      token_t t;
      *tokval = empty_string_t ();
      look_at_token (getter, 0, &t, error_message);
      if (token_is_EOF (t))
        *error_message = eof_in_open_string (t->loc);
      else if (token_is_char (t, '\\'))
        scan_escape (getter, tokval, substring, error_message);
      else if (token_is_char (t, '"'))
        {
          getter->get_token (getter, &t, error_message);
          if (*error_message == NULL)
            {
              *tokval = t->token_value;
              *substring = empty_string_t ();
              *done = true;
            }
        }
      else
        {
          getter->get_token (getter, &t, error_message);
          if (*error_message == NULL)
            {
              *tokval = t->token_value;
              *substring = t->token_value;
            }
        }
    }
  *done = (*done || *error_message != NULL);
}

HIHA_VISIBLE void
scan_string_literal (buffered_token_getter_t getter, token_t *tok,
                     string_t *string, const char **error_message)
{
  string_t str = empty_string_t ();
  token_t token = NULL;
  token_t t;
  getter->get_token (getter, &t, error_message);
  if (*error_message == NULL)
    {
      assert (token_is_char (t, '"'));
      string_t tokval = t->token_value;
      bool done;
      string_t subtokval;
      string_t substring;
      scan_string_portion (getter, &done, &subtokval, &substring,
                           error_message);
      while (!done)
        {
          tokval = concat_string_t (tokval, subtokval, NULL);
          str = concat_string_t (str, substring, NULL);
          scan_string_portion (getter, &done, &subtokval, &substring,
                               error_message);
        }
      if (*error_message == NULL)
        {
          tokval = concat_string_t (tokval, subtokval, NULL);
          token = make_token_t (make_string_t ("STR"), tokval, t->loc);
          str = concat_string_t (str, substring, NULL);
        }
    }
  STORE_IF_NOT_NULL (tok, token);
  STORE_IF_NOT_NULL (string, str);
}

static const char *
string_literal_badly_quoted (string_t str)
{
  char buf[1000];
  snprintf (buf, 1000, _("string literal improperly quoted: %-100s"),
            make_str_nul (str));
  return xstrdup (buf);
}

static void
check_quoting (string_t literal, const char **error_message)
{
  if (*error_message == NULL)
    {
      if (literal->n < 2 || literal->s[0] != '"'
          || literal->s[literal->n - 1] != '"')
        *error_message = string_literal_badly_quoted (literal);
      else
        for (size_t i = 1; i != literal->n - 1; i += 1)
          if (literal->s[i] == '"' && literal->s[i - 1] != '\\')
            *error_message = string_literal_badly_quoted (literal);
    }
}

HIHA_VISIBLE void
dequote_string_literal (string_t literal, token_t *tok,
                        string_t *result, const char **error_message)
{
  *result = empty_string_t ();
  *error_message = NULL;
  check_quoting (literal, error_message);
  if (*error_message == NULL)
    {
      token_getter_t string_getter =
        make_token_getter_from_string (literal);
      buffered_token_getter_t getter =
        make_buffered_token_getter_t (string_getter);
      scan_string_literal (getter, tok, result, error_message);
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
