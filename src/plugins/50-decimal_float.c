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

/*

  ----------------------
  DECIMAL FLOATING POINT
  ----------------------

*/

#include <config.h>
#include <string.h>
#include <xalloc.h>
#include <error.h>
#include <exitfail.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static bool
token_is_decimal_point (token_t tok)
{
  return (string_t_cmp (tok->token_value, make_string_t (".")) == 0);
}

static bool
token_is_i10 (token_t tok)
{
  return (string_t_cmp (tok->token_kind, make_string_t ("I10")) == 0);
}

static bool
token_is_exponent_start (token_t tok)
{
  return (string_t_cmp (tok->token_value, make_string_t ("^")) == 0
          || string_t_cmp (tok->token_value,
                           make_string_t ("↑")) == 0);
}

static bool
token_is_sign (token_t tok)
{
  return (string_t_cmp (tok->token_value, make_string_t ("+")) == 0
          || string_t_cmp (tok->token_value, make_string_t ("-")) == 0
          || string_t_cmp (tok->token_value,
                           make_string_t ("−")) == 0);
}

static void
scan_exponent (buffered_token_getter_t getter, bool *is_a_match,
               token_t *tok_expstart, token_t *tok_sign,
               token_t *tok_exponent, const char **error_message)
{
  /*

     Exponents start with ^+ ^- or ^− (MINUS_SIGN U+2212) or the same
     with the UPWARDS ARROW “↑” U+2191, and not with a lowercase e or
     any identifier. This is because the lowercase e is often used to
     represent a unit vector, and we want to use things like 123.45e10
     + 234.56f5 as vector expressions. This is how we plan to handle,
     for instance, complex number literals, using I as a unit bivector
     and complex numbers as multivectors (syntactically, even if not
     in any “rigorous” sense). Literals of quaternions, Grassmann
     multivectors, and ordinary Gibbs-Heaviside vectors (and of
     Hilbert state space points that are obfuscated representations of
     probability theory propositions, but which physicists mistake for
     logically impossible states of matter), could be handled
     similarly.

     Because hiha is an “orthogonal” language, more will be gained
     than just a notation for vector components. There will be a
     general syntax of JUXTAPOSITION. In the case above, juxtaposition
     will represent multiplication. For two strings, it will represent
     concatenation. Thus two string literals juxtaposed will represent
     one string, as they do in C, but this fact will arise from the
     “orthogonality” of the language—from the general syntax of
     juxtaposition. No special syntax is required.

     (Anyone who wants to argue with me about Hilbert state space must
     first explain how an entangled pair of photons can ever be
     reconciled with what surely must be the best established theory
     in all of physics, the electromagnetic field. The electromagnetic
     field is a FIELD. That is, it is mathematically described by the
     epsilon-delta method, which is a mathematical analog of ACTION BY
     IMMEDIATE CONTACT. So the electromagnetic field is explicitly in
     direct contradiction to “entangled photons.” It is impossible to
     reconcile the two hypotheses. As I say, the states of matter
     physicists believe points in Hilbert space represent are
     LOGICALLY impossible. The points are ACTUALLY obfuscated
     representations of probability theory propositions.

     (Hilbert space is a waste of time, presumably a shallow attempt
     to look like statistical mechanics, and can be discarded. I can
     derive the “quantum” correlation of the Aspect experiment using
     just plain probability theory and no Hilbert space. This
     correlation is merely the ordinary correlation function. John S.
     Bell and John Clauser calculated correlations incorrectly, and
     thus their “inequalities” are nonsense. Furthermore, Aspect’s
     supposed results defy even quantum mechanics, and are obtained by
     biased data analysis. Experimenters simply do not bother to check
     the results of this kind of experiment against nominal values.

     (The same correlation function I derive with probability theory
     is also easily derived from classical coherence theory! This has
     been known for decades. The correlation is a function of the
     relative intensities of the outputs of the polarizers. Are
     quantum physicists prepared to declare it total coincidence that
     classical coherence theory—which assumes action by direct contact
     only—gives the right answer?

     (More likely they do not understand that the epsilon-delta method
     is a mathematical representation of action by direct contact.
     Abraham Robinson may not have understood this when he introduced
     nonstandard analysis as a supposedly “more intuitive” alternative
     to epsilon-delta. The more conventional analysis has a direct
     mechanical meaning.

     (So, in summary, there is a reason I want hiha to give special
     attention to vectors and multivectors. They are widely
     misunderstood and programming languages should perhaps pay them
     more attention. But we gain even more. We gain the generalized
     syntax of juxtaposition.

     (This new programming language is here to do smart things simply,
     despite that the world is filled with stupid things done in
     complicated ways that are mistaken for intelligence.)

   */

  *is_a_match = false;
  *tok_expstart = NULL;
  *tok_sign = NULL;

  getter->look_at_token (getter, 0, tok_expstart, error_message);
  if (*error_message == NULL && token_is_exponent_start (*tok_expstart))
    {
      getter->look_at_token (getter, 1, tok_sign, error_message);
      if (*error_message == NULL && token_is_sign (*tok_sign))
        {
          getter->look_at_token (getter, 2, tok_exponent,
                                 error_message);
          if (*error_message == NULL && token_is_i10 (*tok_exponent))
            {
              (void) getter->get_token (getter, tok_exponent,
                                        error_message);
              if (*error_message == NULL)
                (void) getter->get_token (getter, tok_exponent,
                                          error_message);
              if (*error_message == NULL)
                (void) getter->get_token (getter, tok_exponent,
                                          error_message);
              *is_a_match = (*error_message == NULL);
            }
        }
    }
}

nud_handler_t next_i10_handler;
nud_handler_t next_i_i10_handler;

static void
i10_handler (void *state, buffered_token_getter_t getter,
             pratt_tables_t tables, token_t tok, void **lhs,
             const char **error_message)
{
  bool done = (*error_message != NULL);
  if (!done)
    {
      token_t t;
      getter->look_at_token (getter, 0, &t, error_message);
      done = (*error_message != NULL);
      if (!done && token_is_decimal_point (t))
        {
          token_t u;
          getter->look_at_token (getter, 1, &u, error_message);
          done = (*error_message != NULL);
          if (!done && token_is_i10 (u))
            {
              string_t str =
                concat_string_t (tok->token_value, t->token_value,
                                 u->token_value, NULL);
              *lhs =
                (void *) make_token_t (make_string_t ("I.I10"), str,
                                       tok->loc);
              (void) getter->get_token (getter, &u, error_message);
              if (*error_message == NULL)
                (void) getter->get_token (getter, &u, error_message);
              done = true;
            }
        }
    }
  if (!done)
    next_i10_handler (state, getter, tables, tok, lhs, error_message);
}

static void
i_i10_handler (void *state, buffered_token_getter_t getter,
               pratt_tables_t tables, token_t tok, void **lhs,
               const char **error_message)
{
  /* Convert notations such as 123_456.789E+30 to "F10" tokens. */

  bool done = (*error_message != NULL);
  if (!done)
    {
      bool is_a_match;
      token_t tok_expstart;
      token_t tok_sign;
      token_t tok_exponent;
      scan_exponent (getter, &is_a_match, &tok_expstart, &tok_sign,
                     &tok_exponent, error_message);
      done = (*error_message != NULL);
      if (!done && is_a_match)
        {
          string_t str = concat_string_t (tok->token_value,
                                          tok_expstart->token_value,
                                          tok_sign->token_value,
                                          tok_exponent->token_value,
                                          NULL);
          *lhs =
            (void *) make_token_t (make_string_t ("F10"), str,
                                   tok->loc);
          done = true;
        }
    }
  if (!done)
    next_i_i10_handler (state, getter, tables, tok, lhs, error_message);
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  next_i10_handler = pratt_nud_get (tables, make_string_t ("I10"));
  pratt_nud_put (tables, make_string_t ("I10"), &i10_handler);
  next_i_i10_handler = pratt_nud_get (tables, make_string_t ("I.I10"));
  pratt_nud_put (tables, make_string_t ("I.I10"), &i_i10_handler);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
