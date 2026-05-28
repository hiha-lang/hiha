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
look_at_token (buffered_token_getter_t getter, token_t *tok,
               const char **error_message)
{
  getter->look_at_token (getter, 0, tok, error_message);
  if (*error_message == NULL
      && string_t_cmp ((*tok)->token_kind, string_t_EOF ()) != 0)
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
                                      t->loc, error_message);
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
            getter->push_back_string (getter, t->token_value, t->loc,
                                      error_message);
        }
        break;
      }
}

static void
look_at_and_get_token (buffered_token_getter_t getter,
                       bool (*pred) (token_t), token_t *tok,
                       const char **error_message)
{
  look_at_token (getter, tok, error_message);
  if (*error_message == NULL && pred (*tok))
    {
      token_t t;
      getter->get_token (getter, &t, error_message);
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
illegal_u_escape (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: malformed \\u escape"),
            text_location_string (loc));
  return xstrdup (buf);
}

static const char *
expected_a_low_surrogate (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: expected a low surrogate"),
            text_location_string (loc));
  return xstrdup (buf);
}

static const char *
an_unexpected_low_surrogate (text_location_t loc)
{
  char buf[1000];
  snprintf (buf, 1000, _("%s: an unexpected low surrogate"),
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
      look_at_and_get_token (getter, &token_is_space_separator_or_tab,
                             &t, error_message);
      while (*error_message == NULL
             && token_is_space_separator_or_tab (t))
        {
          *tokval = concat_string_t (*tokval, t->token_value, NULL);
          look_at_and_get_token (getter,
                                 &token_is_space_separator_or_tab, &t,
                                 error_message);
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
      look_at_token (getter, &t, error_message);
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
  look_at_token (getter, &t, error_message);
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
      look_at_and_get_token (getter, &token_is_ascii_hex_digit, &t,
                             error_message);
      while (*error_message == NULL && token_is_ascii_hex_digit (t))
        {
          *tokval = concat_string_t (*tokval, t->token_value, NULL);
          v = string_t_vector_push (v, t->token_value);
          look_at_and_get_token (getter, &token_is_ascii_hex_digit, &t,
                                 error_message);
        }
      if (*error_message == NULL)
        hex_ending_semicolon (getter, v, tokval, code_point,
                              error_message);
    }
}

static void
check_json_utf16_hex (token_t tok, const char **error_message)
{
  if (*error_message == NULL)
    if (token_is_EOF (tok) || !token_is_ascii_hex_digit (tok))
      *error_message = illegal_u_escape (tok->loc);
}

static void
check_surrogates (bool low_surrogate_situation, uint32_t code_unit,
                  text_location_t loc, const char **error_message)
{
  if (*error_message == NULL)
    {
      if (low_surrogate_situation)
        {
          if (code_unit < 0xDC00 || 0xDFFF < code_unit)
            *error_message = expected_a_low_surrogate (loc);
        }
      else
        {
          if (0xDC00 <= code_unit && code_unit <= 0xDFFF)
            *error_message = an_unexpected_low_surrogate (loc);
        }
    }
}

static void
four_digit_hex (buffered_token_getter_t getter,
                bool low_surrogate_situation, string_t *tokval,
                uint32_t *code_unit, const char **error_message)
{
  char buf[5];
  token_t t;
  text_location_t loc[4];

  *tokval = empty_string_t ();
  *code_unit = 0xFFFD;          /* U+FFFD REPLACEMENT CHARACTER */

  size_t i = 0;
  while (*error_message == NULL && i != 4)
    {
      look_at_token (getter, &t, error_message);
      if (*error_message == NULL)
        {
          loc[i] = t->loc;
          check_json_utf16_hex (t, error_message);
          if (*error_message == NULL)
            {
              getter->get_token (getter, &t, error_message);
              if (*error_message == NULL)
                {
                  *tokval =
                    concat_string_t (*tokval, t->token_value, NULL);
                  buf[i] = t->token_value->s[0];
                  look_at_token (getter, &t, error_message);
                }
            }
        }
      i += 1;
    }
  buf[4] = '\0';

  if (*error_message == NULL)
    sscanf (buf, "%" SCNx32, code_unit);

  check_surrogates (low_surrogate_situation, *code_unit, loc[0],
                    error_message);
}

static void
the_u_of_backslash_u (buffered_token_getter_t getter, string_t *tokval,
                      const char **error_message)
{
  if (*error_message == NULL)
    {
      token_t t;
      look_at_token (getter, &t, error_message);
      if (*error_message == NULL)
        {
          if (token_is_EOF (t))
            *error_message = eof_in_open_string (t->loc);
          else if (!token_is_char (t, 'u'))
            *error_message = expected_a_low_surrogate (t->loc);
          else
            {
              getter->get_token (getter, &t, error_message);
              if (*error_message == NULL)
                *tokval = t->token_value;
            }
        }
    }
}

static void
backslash_u (buffered_token_getter_t getter, string_t *tokval,
             const char **error_message)
{
  if (*error_message == NULL)
    {
      token_t t;
      look_at_token (getter, &t, error_message);
      if (*error_message == NULL)
        {
          if (token_is_EOF (t))
            *error_message = eof_in_open_string (t->loc);
          else if (!token_is_char (t, '\\'))
            *error_message = expected_a_low_surrogate (t->loc);
          else
            {
              getter->get_token (getter, &t, error_message);
              if (*error_message == NULL)
                {
                  *tokval = t->token_value;
                  string_t tokval2;
                  the_u_of_backslash_u (getter, &tokval2,
                                        error_message);
                  if (*error_message == NULL)
                    *tokval = concat_string_t (*tokval, tokval2, NULL);
                }
            }
        }
    }
}

static uint32_t
surrogate_pair_to_code_point (uint32_t pair1, uint32_t pair2)
{
  return (((uint32_t) 0x10000) +
          (((uint32_t) 0x400) * (pair1 - ((uint32_t) 0xD800))) +
          (pair2 - ((uint32_t) 0xDC00)));
}

static void
json_utf16 (buffered_token_getter_t getter, string_t *tokval,
            uint32_t *code_point, const char **error_message)
{
  if (*error_message == NULL)
    {
      uint32_t pair1;
      uint32_t pair2;
      string_t tokval1;
      string_t tokval2a;
      string_t tokval2b;

      *tokval = empty_string_t ();
      *code_point = 0xFFFD;     /* U+FFFD REPLACEMENT CHARACTER */

      four_digit_hex (getter, false, &tokval1, &pair1, error_message);
      if (*error_message == NULL)
        {
          if (pair1 <= 0xD7FF || 0xE000 <= pair1)
            {
              /* There is no surrogate pair, and ‘pair1’ is the actual
                 code point. */
              *tokval = tokval1;
              *code_point = pair1;
            }
          else
            {
              /* Look for the second part of a surrogate pair. */
              backslash_u (getter, &tokval2a, error_message);
              four_digit_hex (getter, true, &tokval2b, &pair2,
                              error_message);
              if (*error_message == NULL)
                {
                  *tokval =
                    concat_string_t (tokval1, tokval2a, tokval2b, NULL);
                  *code_point =
                    surrogate_pair_to_code_point (pair1, pair2);
                }
            }
        }
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
escape_u (buffered_token_getter_t getter, string_t *tokval,
          string_t *substring, const char **error_message)
{
  /* JSON-style UTF-16. */

  token_t t;
  getter->get_token (getter, &t, error_message);
  if (*error_message == NULL)
    {
      assert (t->token_value->s[0] == 'u');
      *tokval = t->token_value;
      string_t tokval2;
      uint32_t code_point;
      json_utf16 (getter, &tokval2, &code_point, error_message);
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
      look_at_token (getter, &t, error_message);
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
            case 'u':
              escape_u (getter, &tokval2, substring, error_message);
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
      look_at_token (getter, &t, error_message);
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
        {
          size_t i = 1;
          while (*error_message == NULL && i != literal->n - 1)
            {
              if (literal->s[i] == '"' && literal->s[i - 1] != '\\')
                *error_message = string_literal_badly_quoted (literal);
              i += 1;
            }
        }
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
