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
#include <assert.h>
#include <locale.h>
#include <gc/gc.h>
#include <libhiha/libhiha.h>

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

int
main (void)
{
  GC_INIT ();
  setlocale (LC_ALL, "C.utf8");

  const char *error_message;
  token_t tok;
  string_t str;

  /* ‘eĥoŝanĝo ĉiuĵaŭde’ with decomposed combining accents. */
  dequote_string_literal (make_string_t
                          ("\"eh\314\202os\314\202ang\314\202o "
                           "c\314\202iuj\314\202au\314\206de\""),
                          &tok, &str, &error_message);
  assert (error_message == NULL);
  assert (string_t_cmp (tok->token_kind, make_string_t ("STR")) == 0);
  /* The token_value will have composed accents. */
  assert (string_t_cmp
          (tok->token_value,
           make_string_t ("\"eĥoŝanĝo ĉiuĵaŭde\"")) == 0);
  assert (string_t_cmp (str, make_string_t ("eĥoŝanĝo ĉiuĵaŭde"))
          == 0);

  /* ‘eĥoŝanĝo ĉiuĵaŭde’ with composed accents. */
  dequote_string_literal (make_string_t ("\"eĥoŝanĝo ĉiuĵaŭde\""),
                          &tok, &str, &error_message);
  assert (error_message == NULL);
  assert (string_t_cmp (tok->token_kind, make_string_t ("STR")) == 0);
  assert (string_t_cmp
          (tok->token_value,
           make_string_t ("\"eĥoŝanĝo ĉiuĵaŭde\"")) == 0);
  assert (string_t_cmp (str, make_string_t ("eĥoŝanĝo ĉiuĵaŭde"))
          == 0);

  /* A string of sparkles. */
  dequote_string_literal (make_string_t
                          ("\"\\x2728;\\x2728;\\x2728;\""),
                          &tok, &str, &error_message);
  assert (error_message == NULL);
  assert (string_t_cmp (tok->token_kind, make_string_t ("STR")) == 0);
  assert (string_t_cmp
          (tok->token_value,
           make_string_t ("\"\\x2728;\\x2728;\\x2728;\"")) == 0);
  assert (string_t_cmp (str, make_string_t ("✨✨✨")) == 0);

  /* Simple escapes. */
  dequote_string_literal (make_string_t
                          ("\"\\\"\\\\\\|\\a\\b\\t\\n\\r\\v\\f\""),
                          &tok, &str, &error_message);
  assert (error_message == NULL);
  assert (string_t_cmp (tok->token_kind, make_string_t ("STR")) == 0);
  assert (string_t_cmp
          (tok->token_value,
           make_string_t ("\"\\\"\\\\\\|\\a\\b\\t\\n\\r\\v\\f\""))
          == 0);
  assert (string_t_cmp (str, make_string_t ("\"\\|\a\b\t\n\r\v\f"))
          == 0);

  /* Backslash-newline */
  dequote_string_literal (make_string_t
                          ("\"interr\\\n  \t \302\240 uption\""),
                          &tok, &str, &error_message);
  assert (error_message == NULL);
  assert (string_t_cmp (tok->token_kind, make_string_t ("STR")) == 0);
  assert (string_t_cmp
          (tok->token_value,
           make_string_t ("\"interr\\\n  \t \302\240 uption\"")) == 0);
  assert (string_t_cmp (str, make_string_t ("interruption")) == 0);

  /* Backslash-spaces-and-tabs-then-newline */
  dequote_string_literal
    (make_string_t ("\"interr\\ \302\240\t\n  \t \302\240 uption\""),
     &tok, &str, &error_message);
  assert (error_message == NULL);
  assert (string_t_cmp (tok->token_kind, make_string_t ("STR")) == 0);
  assert (string_t_cmp
          (tok->token_value,
           make_string_t
           ("\"interr\\ \302\240\t\n  \t \302\240 uption\"")) == 0);
  assert (string_t_cmp (str, make_string_t ("interruption")) == 0);

  /* Some badly quoted strings. */
  dequote_string_literal (empty_string_t (), &tok, &str,
                          &error_message);
  assert (error_message != NULL);
  dequote_string_literal (make_string_t ("abc"), &tok, &str,
                          &error_message);
  assert (error_message != NULL);
  dequote_string_literal (make_string_t ("\"abc"), &tok, &str,
                          &error_message);
  assert (error_message != NULL);
  dequote_string_literal (make_string_t ("abc\""), &tok, &str,
                          &error_message);
  assert (error_message != NULL);
  dequote_string_literal (make_string_t ("\"ab\"c\""), &tok, &str,
                          &error_message);
  assert (error_message != NULL);
  dequote_string_literal (make_string_t ("\"ab\"c"), &tok, &str,
                          &error_message);
  assert (error_message != NULL);

  return EXIT_SUCCESS;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
