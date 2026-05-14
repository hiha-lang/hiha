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

/*--------------------------------------------------------------------*/
/* SpookyHash version 2. first translated from Barry Schwartz’s       */
/* implementation for ATS/Postiats, but then much reworked for C.     */
/*                                                                    */
/* SpookyHash version 2 was invented by Bob Jenkins, with a public    */
/* domain reference implementation in C++.                            */
/*--------------------------------------------------------------------*/

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <libhiha/spookyhash.h>

/*--------------------------------------------------------------------*/

#ifndef SPOOKYHASH_NDEBUG
#define SPOOKYHASH_NDEBUG 1
#endif

#ifndef SPOOKYHASH_ASSERT
#if SPOOKYHASH_NDEBUG != 0
#define SPOOKYHASH_ASSERT(expr) do { } while (0)
#else
#define SPOOKYHASH_ASSERT(expr)                                         \
  do                                                                    \
    {                                                                   \
      if (!(expr))                                                      \
        {                                                               \
          fprintf (stderr,                                              \
                   "SPOOKYHASH_ASSERT failure at %s:%lu: “%s”\n",       \
                   __FILE__, __LINE__, #expr);                          \
          abort ();                                                     \
        }                                                               \
    }                                                                   \
  while (0)
#endif
#endif

#ifndef SPOOKYHASH_VISIBLE
#define SPOOKYHASH_VISIBLE [[gnu::visibility ("default")]]
#endif

#ifndef SPOOKYHASH_INLINE
#define SPOOKYHASH_INLINE [[gnu::always_inline]] static inline
#endif

/*--------------------------------------------------------------------*/

_Static_assert (sizeof (uint8_t) == 1, "uint8_t is not 1 byte");
_Static_assert (sizeof (uint16_t) == 2, "uint16_t is not 2 bytes");
_Static_assert (sizeof (uint32_t) == 4, "uint32_t is not 4 bytes");
_Static_assert (sizeof (uint64_t) == 8, "uint64_t is not 8 bytes");

/*--------------------------------------------------------------------*/

#define SPOOKYHASH_NUMVARS 12
#define SPOOKYHASH_TWICE_NUMVARS (2 * SPOOKYHASH_NUMVARS)
#define SPOOKYHASH_BLOCKSIZE    (sizeof (uint64_t) * SPOOKYHASH_NUMVARS)
#define SPOOKYHASH_BUFSIZE      (2 * SPOOKYHASH_BLOCKSIZE)

/*
 * sc_const: a constant which:
 *    is not zero
 *    is odd
 *    is a not-very-regular mix of 1's and 0's
 *    does not need any other special mathematical properties
 */
#ifndef SPOOKYHASH_CONST
#define SPOOKYHASH_CONST 0x5A758460830854B7ULL
#endif

#if BYTE_ORDER == LITTLE_ENDIAN

SPOOKYHASH_INLINE uint32_t
spookyhash_fix_byte_order_32 (uint32_t x)
{
  return x;
}

SPOOKYHASH_INLINE uint64_t
spookyhash_fix_byte_order_64 (uint64_t x)
{
  return x;
}

#elif BYTE_ORDER == BIG_ENDIAN

SPOOKYHASH_INLINE uint32_t
spookyhash_fix_byte_order_32 (uint32_t x)
{
  return bswap_32 (x);
}

SPOOKYHASH_INLINE uint64_t
spookyhash_fix_byte_order_64 (uint64_t x)
{
  return bswap_64 (x);
}

#else

#error "Only little and big endianness are supported."

#endif

SPOOKYHASH_INLINE uint64_t
spookyhash_aligned_64 (const void *p)
{
  return ((((uintptr_t) p) % sizeof (uint64_t)) == 0);
}

/* Bitwise left rotation by an amount in 0..63. */
SPOOKYHASH_INLINE uint64_t
lrotate_64 (uint64_t x, unsigned int i)
{
  return ((x << i) | (x >> (64 - i)));
}

SPOOKYHASH_INLINE void
spookyhash_store_hash_bits (uint64_t *dest, uint64_t src)
{
  if (dest != NULL)
    *dest = src;
}

/*--------------------------------------------------------------------*/

SPOOKYHASH_INLINE void
spookyhash_mix (uint64_t h[SPOOKYHASH_NUMVARS],
                const uint64_t data[SPOOKYHASH_NUMVARS])
{
  uint64_t h0 = h[0];
  uint64_t h1 = h[1];
  uint64_t h2 = h[2];
  uint64_t h3 = h[3];
  uint64_t h4 = h[4];
  uint64_t h5 = h[5];
  uint64_t h6 = h[6];
  uint64_t h7 = h[7];
  uint64_t h8 = h[8];
  uint64_t h9 = h[9];
  uint64_t h10 = h[10];
  uint64_t h11 = h[11];

  h0 += spookyhash_fix_byte_order_64 (data[0]);
  h2 ^= h10;
  h11 ^= h0;
  h0 = lrotate_64 (h0, 11);
  h11 += h1;

  h1 += spookyhash_fix_byte_order_64 (data[1]);
  h3 ^= h11;
  h0 ^= h1;
  h1 = lrotate_64 (h1, 32);
  h0 += h2;

  h2 += spookyhash_fix_byte_order_64 (data[2]);
  h4 ^= h0;
  h1 ^= h2;
  h2 = lrotate_64 (h2, 43);
  h1 += h3;

  h3 += spookyhash_fix_byte_order_64 (data[3]);
  h5 ^= h1;
  h2 ^= h3;
  h3 = lrotate_64 (h3, 31);
  h2 += h4;

  h4 += spookyhash_fix_byte_order_64 (data[4]);
  h6 ^= h2;
  h3 ^= h4;
  h4 = lrotate_64 (h4, 17);
  h3 += h5;

  h5 += spookyhash_fix_byte_order_64 (data[5]);
  h7 ^= h3;
  h4 ^= h5;
  h5 = lrotate_64 (h5, 28);
  h4 += h6;

  h6 += spookyhash_fix_byte_order_64 (data[6]);
  h8 ^= h4;
  h5 ^= h6;
  h6 = lrotate_64 (h6, 39);
  h5 += h7;

  h7 += spookyhash_fix_byte_order_64 (data[7]);
  h9 ^= h5;
  h6 ^= h7;
  h7 = lrotate_64 (h7, 57);
  h6 += h8;

  h8 += spookyhash_fix_byte_order_64 (data[8]);
  h10 ^= h6;
  h7 ^= h8;
  h8 = lrotate_64 (h8, 55);
  h7 += h9;

  h9 += spookyhash_fix_byte_order_64 (data[9]);
  h11 ^= h7;
  h8 ^= h9;
  h9 = lrotate_64 (h9, 54);
  h8 += h10;

  h10 += spookyhash_fix_byte_order_64 (data[10]);
  h0 ^= h8;
  h9 ^= h10;
  h10 = lrotate_64 (h10, 22);
  h9 += h11;

  h11 += spookyhash_fix_byte_order_64 (data[11]);
  h1 ^= h9;
  h10 ^= h11;
  h11 = lrotate_64 (h11, 46);
  h10 += h0;

  h[0] = h0;
  h[1] = h1;
  h[2] = h2;
  h[3] = h3;
  h[4] = h4;
  h[5] = h5;
  h[6] = h6;
  h[7] = h7;
  h[8] = h8;
  h[9] = h9;
  h[10] = h10;
  h[11] = h11;
}

SPOOKYHASH_INLINE void
spookyhash_mix_unaligned (uint64_t h[SPOOKYHASH_NUMVARS],
                          const void *possibly_unaligned_data)
{
  if (spookyhash_aligned_64 (possibly_unaligned_data))
    spookyhash_mix (h, possibly_unaligned_data);
  else
    {
      /* On x86 machines, copying the data to an aligned position is
         not necessary, but does no harm and might or might not be
         faster. For many other CPU designs, it is necessary. */
      uint64_t buf[SPOOKYHASH_NUMVARS];
      memcpy (buf, possibly_unaligned_data,
              SPOOKYHASH_NUMVARS * sizeof (uint64_t));
      spookyhash_mix (h, buf);
    }
}

SPOOKYHASH_INLINE void
spookyhash_end_partial (uint64_t h[SPOOKYHASH_NUMVARS])
{
  uint64_t h0 = h[0];
  uint64_t h1 = h[1];
  uint64_t h2 = h[2];
  uint64_t h3 = h[3];
  uint64_t h4 = h[4];
  uint64_t h5 = h[5];
  uint64_t h6 = h[6];
  uint64_t h7 = h[7];
  uint64_t h8 = h[8];
  uint64_t h9 = h[9];
  uint64_t h10 = h[10];
  uint64_t h11 = h[11];

  h11 += h1;
  h2 ^= h11;
  h1 = lrotate_64 (h1, 44);

  h0 += h2;
  h3 ^= h0;
  h2 = lrotate_64 (h2, 15);

  h1 += h3;
  h4 ^= h1;
  h3 = lrotate_64 (h3, 34);

  h2 += h4;
  h5 ^= h2;
  h4 = lrotate_64 (h4, 21);

  h3 += h5;
  h6 ^= h3;
  h5 = lrotate_64 (h5, 38);

  h4 += h6;
  h7 ^= h4;
  h6 = lrotate_64 (h6, 33);

  h5 += h7;
  h8 ^= h5;
  h7 = lrotate_64 (h7, 10);

  h6 += h8;
  h9 ^= h6;
  h8 = lrotate_64 (h8, 13);

  h7 += h9;
  h10 ^= h7;
  h9 = lrotate_64 (h9, 38);

  h8 += h10;
  h11 ^= h8;
  h10 = lrotate_64 (h10, 53);

  h9 += h11;
  h0 ^= h9;
  h11 = lrotate_64 (h11, 42);

  h10 += h0;
  h1 ^= h10;
  h0 = lrotate_64 (h0, 54);

  h[0] = h0;
  h[1] = h1;
  h[2] = h2;
  h[3] = h3;
  h[4] = h4;
  h[5] = h5;
  h[6] = h6;
  h[7] = h7;
  h[8] = h8;
  h[9] = h9;
  h[10] = h10;
  h[11] = h11;
}

SPOOKYHASH_INLINE void
spookyhash_final_end (uint64_t h[SPOOKYHASH_NUMVARS],
                      const uint64_t data[SPOOKYHASH_NUMVARS])
{
  h[0] += spookyhash_fix_byte_order_64 (data[0]);
  h[1] += spookyhash_fix_byte_order_64 (data[1]);
  h[2] += spookyhash_fix_byte_order_64 (data[2]);
  h[3] += spookyhash_fix_byte_order_64 (data[3]);
  h[4] += spookyhash_fix_byte_order_64 (data[4]);
  h[5] += spookyhash_fix_byte_order_64 (data[5]);
  h[6] += spookyhash_fix_byte_order_64 (data[6]);
  h[7] += spookyhash_fix_byte_order_64 (data[7]);
  h[8] += spookyhash_fix_byte_order_64 (data[8]);
  h[9] += spookyhash_fix_byte_order_64 (data[9]);
  h[10] += spookyhash_fix_byte_order_64 (data[10]);
  h[11] += spookyhash_fix_byte_order_64 (data[11]);

  spookyhash_end_partial (h);
  spookyhash_end_partial (h);
  spookyhash_end_partial (h);
}

/*--------------------------------------------------------------------*/

SPOOKYHASH_INLINE void
spookyhash_short_mix (uint64_t abcd[4])
{
  uint64_t a = abcd[0];
  uint64_t b = abcd[1];
  uint64_t c = abcd[2];
  uint64_t d = abcd[3];

  c = lrotate_64 (c, 50);
  c += d;
  a ^= c;

  d = lrotate_64 (d, 52);
  d += a;
  b ^= d;

  a = lrotate_64 (a, 30);
  a += b;
  c ^= a;

  b = lrotate_64 (b, 41);
  b += c;
  d ^= b;

  c = lrotate_64 (c, 54);
  c += d;
  a ^= c;

  d = lrotate_64 (d, 48);
  d += a;
  b ^= d;

  a = lrotate_64 (a, 38);
  a += b;
  c ^= a;

  b = lrotate_64 (b, 37);
  b += c;
  d ^= b;

  c = lrotate_64 (c, 62);
  c += d;
  a ^= c;

  d = lrotate_64 (d, 34);
  d += a;
  b ^= d;

  a = lrotate_64 (a, 5);
  a += b;
  c ^= a;

  b = lrotate_64 (b, 36);
  b += c;
  d ^= b;

  abcd[0] = a;
  abcd[1] = b;
  abcd[2] = c;
  abcd[3] = d;
}

SPOOKYHASH_INLINE void
spookyhash_short_end (uint64_t abcd[4])
{
  uint64_t a = abcd[0];
  uint64_t b = abcd[1];
  uint64_t c = abcd[2];
  uint64_t d = abcd[3];

  d ^= c;
  c = lrotate_64 (c, 15);
  d += c;

  a ^= d;
  d = lrotate_64 (d, 52);
  a += d;

  b ^= a;
  a = lrotate_64 (a, 26);
  b += a;

  c ^= b;
  b = lrotate_64 (b, 51);
  c += b;

  d ^= c;
  c = lrotate_64 (c, 28);
  d += c;

  a ^= d;
  d = lrotate_64 (d, 9);
  a += d;

  b ^= a;
  a = lrotate_64 (a, 47);
  b += a;

  c ^= b;
  b = lrotate_64 (b, 54);
  c += b;

  d ^= c;
  c = lrotate_64 (c, 32);
  d += c;

  a ^= d;
  d = lrotate_64 (d, 25);
  a += d;

  b ^= a;
  a = lrotate_64 (a, 63);
  b += a;

  abcd[0] = a;
  abcd[1] = b;
  abcd[2] = c;
  abcd[3] = d;
}

SPOOKYHASH_INLINE void
spookyhash_short_last_bytes (uint64_t abcd[4],
                             const void *data,
                             uint8_t num_bytes)
{
  uint64_t a = abcd[0];
  uint64_t b = abcd[1];
  uint64_t c = abcd[2];
  uint64_t d = abcd[3];

  const uint8_t *p = data;
  const uint32_t *q = data;
  const uint64_t *r = data;

  switch (num_bytes)
    {
    case 0:
      c += SPOOKYHASH_CONST;
      d += SPOOKYHASH_CONST;
      break;
    case 1:
      c += (uint64_t) p[0];
      break;
    case 2:
      c += ((uint64_t) p[1]) << 8;
      c += (uint64_t) p[0];
      break;
    case 3:
      c += ((uint64_t) p[2]) << 16;
      c += ((uint64_t) p[1]) << 8;
      c += (uint64_t) p[0];
      break;
    case 4:
      c += spookyhash_fix_byte_order_32 (q[0]);
      break;
    case 5:
      c += ((uint64_t) p[4]) << 32;
      c += spookyhash_fix_byte_order_32 (q[0]);
      break;
    case 6:
      c += ((uint64_t) p[5]) << 40;
      c += ((uint64_t) p[4]) << 32;
      c += spookyhash_fix_byte_order_32 (q[0]);
      break;
    case 7:
      c += ((uint64_t) p[6]) << 48;
      c += ((uint64_t) p[5]) << 40;
      c += ((uint64_t) p[4]) << 32;
      c += spookyhash_fix_byte_order_32 (q[0]);
      break;
    case 8:
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    case 9:
      d += (uint64_t) p[8];
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    case 10:
      d += ((uint64_t) p[9]) << 8;
      d += (uint64_t) p[8];
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    case 11:
      d += ((uint64_t) p[10]) << 16;
      d += ((uint64_t) p[9]) << 8;
      d += (uint64_t) p[8];
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    case 12:
      d += spookyhash_fix_byte_order_32 (q[2]);
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    case 13:
      d += ((uint64_t) p[12]) << 32;
      d += spookyhash_fix_byte_order_32 (q[2]);
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    case 14:
      d += ((uint64_t) p[13]) << 40;
      d += ((uint64_t) p[12]) << 32;
      d += spookyhash_fix_byte_order_32 (q[2]);
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    case 15:
      d += ((uint64_t) p[14]) << 48;
      d += ((uint64_t) p[13]) << 40;
      d += ((uint64_t) p[12]) << 32;
      d += spookyhash_fix_byte_order_32 (q[2]);
      c += spookyhash_fix_byte_order_64 (r[0]);
      break;
    default:
      SPOOKYHASH_ASSERT (0);
      abort ();
    }

  abcd[0] = a;
  abcd[1] = b;
  abcd[2] = c;
  abcd[3] = d;
}

SPOOKYHASH_INLINE void
short_aligned (const uint64_t *message, size_t length,
               uint64_t seed1, uint64_t seed2,
               uint64_t *hash1, uint64_t *hash2)
{
  const size_t block_count = length / 32;
  size_t remainder = length % 32;

  uint64_t abcd[4] = {
    [0] = seed1,
    [1] = seed2,
    [2] = SPOOKYHASH_CONST,
    [3] = SPOOKYHASH_CONST
  };

  const uint64_t *data = message;

  if (15 < length)
    {
      /* Handle all complete sets of 32 bytes. */
      for (size_t i = 0; i != block_count; i += 1)
        {
          abcd[2] += spookyhash_fix_byte_order_64 (data[0]);
          abcd[3] += spookyhash_fix_byte_order_64 (data[1]);
          spookyhash_short_mix (abcd);
          abcd[0] += spookyhash_fix_byte_order_64 (data[2]);
          abcd[1] += spookyhash_fix_byte_order_64 (data[3]);
          data += 4;
        }

      if (16 <= remainder)
        {
          /* Handle the case of 16 or more remaining bytes. */
          abcd[2] += spookyhash_fix_byte_order_64 (data[0]);
          abcd[3] += spookyhash_fix_byte_order_64 (data[1]);
          spookyhash_short_mix (abcd);
          data += 2;
          remainder -= 16;
        }
    }

  abcd[3] += ((uint64_t) length) << 56;
  spookyhash_short_last_bytes (abcd, data, remainder);
  spookyhash_short_end (abcd);

  spookyhash_store_hash_bits (hash1, abcd[0]);
  spookyhash_store_hash_bits (hash2, abcd[1]);
}

SPOOKYHASH_INLINE void
spookyhash_short (const void *message, size_t length,
                  uint64_t seed1, uint64_t seed2,
                  uint64_t *hash1, uint64_t *hash2)
{
  if (spookyhash_aligned_64 (message))
    short_aligned (message, length, seed1, seed2, hash1, hash2);
  else
    {
      /* On x86 machines, copying the data to an aligned position is
         not necessary, but does no harm and might or might not be
         faster. For many other CPU designs, it is necessary. */
      uint64_t buf[SPOOKYHASH_TWICE_NUMVARS];
      memcpy (buf, message, length);
      short_aligned (buf, length, seed1, seed2, hash1, hash2);
    }
}

/*--------------------------------------------------------------------*/

SPOOKYHASH_VISIBLE void
spookyhash_init (spookyhash_context_t *context,
                 uint64_t seed1, uint64_t seed2)
{
  memset (context, 0, sizeof (spookyhash_context_t));
  context->state[0] = seed1;
  context->state[1] = seed2;
  context->length = 0;
  context->remainder = 0;
}

SPOOKYHASH_VISIBLE void
spookyhash_update (spookyhash_context_t *context,
                   const void *message, size_t length)
{
  const size_t new_length = length + context->remainder;

  if (new_length < SPOOKYHASH_BUFSIZE)
    {
      /* The message fragment is short. Store it for later use. */
      void *p_void = context->data;
      uint8_t *p_8 = p_void;
      memcpy (p_8 + context->remainder, message, length);
      context->length += length;
      context->remainder = new_length;
    }
  else
    {
      uint64_t h[SPOOKYHASH_NUMVARS];

      if (SPOOKYHASH_BUFSIZE <= context->length)
        memcpy (h, context->state,
                SPOOKYHASH_NUMVARS * sizeof (uint64_t));
      else
        {
          const uint64_t h0 = context->state[0];
          const uint64_t h1 = context->state[1];

          h[0] = h0;
          h[3] = h0;
          h[6] = h0;
          h[9] = h0;

          h[1] = h1;
          h[4] = h1;
          h[7] = h1;
          h[10] = h1;

          h[2] = SPOOKYHASH_CONST;
          h[5] = SPOOKYHASH_CONST;
          h[8] = SPOOKYHASH_CONST;
          h[11] = SPOOKYHASH_CONST;
        }

      context->length += length;

      const uint8_t *messg = message;
      size_t length1 = length;

      if (context->remainder != 0)
        {
          const uint8_t prefx = SPOOKYHASH_BUFSIZE - context->remainder;

          void *p_data = context->data;

          uint64_t *p1_64 = p_data;
          uint64_t *p2_64 = p1_64 + SPOOKYHASH_NUMVARS;

          uint8_t *p_8 = p_data;

          /* Copy prefx bytes from the message to the data buffer. */
          memcpy (p_8 + context->remainder, messg, prefx);

          /* Mix in both halves of the data buffer. */
          spookyhash_mix (h, p1_64);
          spookyhash_mix (h, p2_64);

          /* This leaves us with the rest of the message. */
          messg += prefx;
          length1 -= prefx;
        }

      SPOOKYHASH_ASSERT
        (messg + length1 == (const uint8_t *) message + length);

      /* Divide the message into blocks and a small remainder. */
      const size_t block_count = length1 / SPOOKYHASH_BLOCKSIZE;
      const uint8_t remaindr = length1 % SPOOKYHASH_BLOCKSIZE;

      /* Handle all the full-size blocks. */
      for (size_t i = 0; i != block_count; i += 1)
        {
          spookyhash_mix_unaligned (h, messg);
          messg += SPOOKYHASH_BLOCKSIZE;
        }

      /* Store the remainder. */
      context->remainder = remaindr;
      memcpy (context->data, messg, context->remainder);

      memcpy (context->state, h,
              SPOOKYHASH_NUMVARS * sizeof (uint64_t));
    }
}

SPOOKYHASH_VISIBLE void
spookyhash_final (spookyhash_context_t *context,
                  uint64_t *hash1, uint64_t *hash2)
{
  if (context->length < SPOOKYHASH_BUFSIZE)
    spookyhash_short (context->data, context->length,
                      context->state[0], context->state[1],
                      hash1, hash2);
  else
    {
      uint64_t h[SPOOKYHASH_NUMVARS];

      memcpy (h, context->state,
              SPOOKYHASH_NUMVARS * sizeof (uint64_t));

      size_t block_num = 0;
      uint8_t remainder = context->remainder;

      if (SPOOKYHASH_BLOCKSIZE <= remainder)
        {
          /* The data field of a context can contain two blocks.
             Handle any whole first block. */
          spookyhash_mix (h, context->data);
          block_num = 1;
          remainder -= SPOOKYHASH_BLOCKSIZE;
        }

      /* Mix in the last partial block, and the length mod
         SPOOKYHASH_BLOCKSIZE. */
      uint64_t data_block[SPOOKYHASH_NUMVARS];
      void *p_void = data_block;
      uint8_t *p_8 = p_void;
      memset (data_block, 0, SPOOKYHASH_NUMVARS * sizeof (uint64_t));
      memcpy (data_block,
              context->data + (block_num * SPOOKYHASH_NUMVARS),
              remainder);
      p_8[SPOOKYHASH_BLOCKSIZE - 1] = remainder;
      spookyhash_final_end (h, data_block);

      spookyhash_store_hash_bits (hash1, h[0]);
      spookyhash_store_hash_bits (hash2, h[1]);
    }
}

SPOOKYHASH_VISIBLE void
spookyhash_little_endian (uint64_t hash1, uint64_t hash2,
                          uint8_t hash_bytes[2 * sizeof (uint64_t)])
{
  const uint64_t hsh1 = spookyhash_fix_byte_order_64 (hash1);
  const uint64_t hsh2 = spookyhash_fix_byte_order_64 (hash2);
  memcpy (&hash_bytes[0], &hsh1, sizeof hsh1);
  memcpy (&hash_bytes[sizeof (uint64_t)], &hsh2, sizeof hsh2);
}

/*--------------------------------------------------------------------*/
/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
