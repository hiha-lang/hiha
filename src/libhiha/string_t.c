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
#include <string.h>
#include <errno.h>
#include <error.h>
#include <xalloc.h>
#include <exitfail.h>
#include <libhiha/string_t.h>

// Change this if using gettext.
#define _(msgid) msgid

//const uint32_t *
//string_t_check (const string_t *str)
//{
//  return u32_check (str->s, str->n);
//}

int
string_t_casecoll (const string_t *str1, const string_t *str2,
		   const char *iso639_language, uninorm_t nf,
		   int *resultp)
{
  return u32_casecoll (str1->s, str1->n, str2->s, str2->n,
		       iso639_language, nf, resultp);
}

//uint32_t *
//string_t_normalize (uninorm_t nf, const string_t *src, string_t *result)
//{
//  return u32_normalize (nf, src->s, src->n, result->s, &result->n);
//}
//
//uint32_t *
//string_t_conv_from_encoding (const char *fromcode,
//                           enum iconv_ilseq_handler handler,
//                           const char *src, size_t srclen,
//                           size_t *offsets, string_t *result)
//{
//  return u32_conv_from_encoding (fromcode, handler, src, srclen,
//                               offsets, result->s, &result->n);
//}
//
//char *
//string_t_conv_to_encoding (const char *tocode,
//                         enum iconv_ilseq_handler handler,
//                         const string_t *src,
//                         size_t *offsets, char *resultbuf,
//                         size_t *lengthp)
//{
//  return u32_conv_to_encoding (tocode, handler, src->s, src->n, offsets,
//                             resultbuf, lengthp);
//}

void
string_t_free (string_t *str)
{
  free (str->s);
  free (str);
}

string_t *
string_t_canonical_from_str_len (const char *src, size_t srclen,
				 error_location_reporter_t errloc)
{
  string_t *str1 = string_t_from_str_len (src, srclen, errloc);
  string_t *str = string_t_canonicalize (str1, errloc);
  string_t_free (str1);
  return str;
}

string_t *
string_t_from_str_len (const char *src, size_t srclen,
		       error_location_reporter_t errloc)
{
  size_t n;
  uint32_t *u32 = u32_conv_from_encoding (locale_charset (),
					  iconveh_replacement_character,
					  src, srclen, NULL,
					  NULL, &n);
  int err_number = errno;
  if (u32 == NULL)
    {
      if (errloc != NULL)
	errloc->func (errloc);
      switch (err_number)
	{
	case ENOMEM:
	  error (exit_failure, 0, "%s", _("memory exhausted"));
	case EILSEQ:
	  error (exit_failure, 0, "%s", _("unconvertible character"));
	default:
	  error (exit_failure, err_number, "%s %d", _("error number"),
		 err_number);
	}
    }
  string_t *result = XMALLOC (string_t);
  result->s = u32;
  result->n = n;
  return result;
}

string_t *
string_t_canonicalize (const string_t *src,
		       error_location_reporter_t errloc)
{
  size_t n;
  uint32_t *u32 = u32_normalize (UNINORM_NFC, src->s, src->n,
				 NULL, &n);
  int err_number = errno;
  if (u32 == NULL)
    {
      if (errloc != NULL)
	errloc->func (errloc);
      error (exit_failure, err_number, "%s %d", _("error number"),
	     err_number);
    }
  string_t *result = XMALLOC (string_t);
  result->s = u32;
  result->n = n;
  return result;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
