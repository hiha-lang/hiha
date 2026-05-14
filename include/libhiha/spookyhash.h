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

#ifndef __LIBHAHA__SPOOKYHASH_H__INCLUDED__
#define __LIBHAHA__SPOOKYHASH_H__INCLUDED__

#include <stdint.h>
#include <stddef.h>

typedef struct
{
  /* Unhashed data, for partial messages. */
  uint64_t data[24];
  /* Internal state of the hash. */
  uint64_t state[12];
  /* Total length of the input so far. */
  size_t length;
  /* Length of unhashed data stashed in data. */
  uint8_t remainder;
} spookyhash_context_t;

/*
  spookyhash_init:

  Initialize the context of a SpookyHash.

  Any 64-bit value, including zero, will work as a seed. Different
  seeds produce hashes independent of each other.
*/
void spookyhash_init (spookyhash_context_t * context,
                      uint64_t seed1, uint64_t seed2);

/*
  spookyhash_update:

  Add a message fragment to a SpookyHash context.
*/
void spookyhash_update (spookyhash_context_t * context,
                        const void *message, size_t length);

/*
  spookyhash_final:

  Compute the hash for the current context.

  *hash1 = the first 64 bits, in native byte order
  *hash2 = the second 64 bits, in native byte order

  Either hash1 or hash2 may be set to NULL.

  You can continue updating the context, even after
  you have used spookyhash_final.
*/
void spookyhash_final (spookyhash_context_t * context,
                       uint64_t * hash1, uint64_t * hash2);

#endif /* __LIBHAHA__SPOOKYHASH_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
