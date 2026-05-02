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
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <base64.h>
#include <xalloc.h>
#include <libhiha/spinlock.h>
#include <libhiha/initialize_once.h>
#include <libhiha/gensym.h>

#define _(msgid) HIHA_GETTEXT (msgid)

const char alphanum[63] =
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static uintmax_t _gensym_counter = 0;
static spinlock_t _gensym_counter_lock = SPINLOCK_T_INIT;

static uintmax_t
gensym_counter (void)
{
  uintmax_t result;
  acquire_spinlock (&_gensym_counter_lock);
  result = _gensym_counter;
  _gensym_counter += 1;
  release_spinlock (&_gensym_counter_lock);
  return result;
}

static bool
getentropy_works (void)
{
  char buf[256];
  int retval = getentropy (buf, 256);
  return (retval == 0);
}

enum gensym_method
{
  gensym_method_FALLBACK,
  gensym_method_GETENTROPY
};
typedef enum gensym_method gensym_method_t;

static initialize_once_t _gensym_method_init1t = INITIALIZE_ONCE_T_INIT;
static gensym_method_t _gensym_method;

static void
_initialize_gensym_method (void)
{
  _gensym_method = gensym_method_FALLBACK;
  if (getentropy_works ())
    _gensym_method = gensym_method_GETENTROPY;
}

static gensym_method_t
gensym_method (void)
{
  INITIALIZE_ONCE (_gensym_method_init1t, _initialize_gensym_method);
  return _gensym_method;
}

static char *
gensym_given_unlikely_text (const char *prefix,
                            const char *unlikely_text)
{
  char scounter[100];
  snprintf (scounter, 100, "%zu", gensym_counter ());
  size_t ncounter = strlen (scounter);
  size_t nprefix = strlen (prefix);
  size_t nunlikely = strlen (unlikely_text);
  size_t n = ncounter + nprefix + nunlikely;
  char *s = XNMALLOC (n + 1, char);
  memcpy (s, prefix, nprefix * sizeof (char));
  memcpy (s + nprefix, unlikely_text, nunlikely * sizeof (char));
  memcpy (s + nprefix + nunlikely, scounter, ncounter * sizeof (char));
  s[n] = '\0';
  return s;
}

static char *
gensym_by_fallback_method (const char *prefix)
{
  //
  // Use a random string that was generated once upon a time.
  //
  return gensym_given_unlikely_text (prefix,
                                     "E7fDSzid40l4fZiTDEqR27AWwOLZhzgN0H7W7QcbfWka");
}

static char *
gensym_by_getentropy_method (const char *prefix)
{
  //
  // 32 bytes is 256 bits, which is twice as many bits as a UUID.
  // However, we do compromise a slightly on + and / characters in the
  // BASE64 encoding.
  //

  char inbuf[32];
  char outbuf[BASE64_LENGTH (32) + 1];
  char *s;

  int retval = getentropy (inbuf, 32);
  if (retval != 0)
    s = gensym_by_fallback_method (prefix);
  else
    {
      base64_encode (inbuf, sizeof inbuf / sizeof (char),
                     outbuf, sizeof outbuf / sizeof (char));
      for (size_t i = 0; outbuf[i] != '\0'; i += 1)
        if (outbuf[i] == '+' || outbuf[i] == '/' || outbuf[i] == '=')
          outbuf[i] = alphanum[random () % 62];
      s = gensym_given_unlikely_text (prefix, outbuf);
    }
  return s;
}

HIHA_VISIBLE const char *
gensym (const char *prefix)
{
  char *s;
  switch (gensym_method ())
    {
    case gensym_method_FALLBACK:
      s = gensym_by_fallback_method (prefix);
      break;
    case gensym_method_GETENTROPY:
      s = gensym_by_getentropy_method (prefix);
      break;
    default:
      assert (false);
      abort ();
    }
  return s;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
