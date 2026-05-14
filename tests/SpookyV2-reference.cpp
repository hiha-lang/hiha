/*

  Copyright © 2021, 2026 Barry Schwartz

  This program is free software: you can redistribute it and/or
  modify it under the terms of the GNU General Public License, as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received copies of the GNU General Public License
  along with this program. If not, see
  <https://www.gnu.org/licenses/>.

*/

#include <config.h>
#include <cstdio>
#include <cstdint>
#include <cinttypes>
#include <cstdlib>
#include "SpookyV2.h"

const char version_etc_copyright[] = "Copyright %s %d Barry Schwartz";

#ifndef BE_NOISY
#define BE_NOISY 0
#endif

#if BE_NOISY
#define IF_NOISY(code)                          \
  do                                            \
    {                                           \
      code;                                     \
    }                                           \
  while (0)
#else
#define IF_NOISY(code)                          \
  do                                            \
    {                                           \
      /* nothing */;                            \
    }                                           \
  while (0)
#endif

static void
fill_message_0 (char *message,
                size_t length)
{
  for (size_t i = 0; i != length; i += 1)
    message[i] = (char) (unsigned char) ((i + 128) % 256);
}

static void
fill_message (char *message,
              size_t length,
              int pattern)
{
  switch (pattern)
    {
    case 0:
      fill_message_0 (message, length);
      break;
    default:
      printf ("pattern other than 0 is not supported.\n");
      exit (1);
    }
}

#if 0
int
main (int argc, char *argv[])
{
  const uint64_t seed1 =
    (2 <= argc) ? strtoull (argv[1], NULL, 16) : 0;
  const uint64_t seed2 =
    (3 <= argc) ? strtoull (argv[2], NULL, 16) : 0;
  const size_t length = (4 <= argc) ? atoll (argv[3]) : 0;
  const int pattern = (5 <= argc) ? atoll (argv[4]) : 0;
  IF_NOISY (fprintf (stderr, "seed1 = 0x%016" PRIX64 "\n", seed1));
  IF_NOISY (fprintf (stderr, "seed2 = 0x%016" PRIX64 "\n", seed2));
  IF_NOISY (fprintf (stderr, "length = %zu\n", length));
  IF_NOISY (fprintf (stderr, "pattern = %d\n", pattern));

  char *message = new char[length];
  fill_message (message, length, pattern);

  uint64_t hash1 = seed1;
  uint64_t hash2 = seed2;
  SpookyHash::Hash128(message, length, &hash1, &hash2);

  printf ("%016" PRIX64 " %016" PRIX64 " %zu %d "
          "%016" PRIX64 " %016" PRIX64 "\n",
          seed1, seed2, length, pattern, hash1, hash2);

  return 0;
}
#endif

int
main (int argc, char *argv[])
{
  const uint64_t seed1 =
    (2 <= argc) ? strtoull (argv[1], NULL, 16) : 0;
  const uint64_t seed2 =
    (3 <= argc) ? strtoull (argv[2], NULL, 16) : 0;
  const size_t length_max = (4 <= argc) ? atoll (argv[3]) : 0;

  char *message = new char[length_max + 1];

  for (size_t length = 0; length != length_max + 1; length += 1)
    {
      fill_message (message, length, 0);

      uint64_t hash1 = seed1;
      uint64_t hash2 = seed2;

      SpookyHash::Hash128(message, length, &hash1, &hash2);

      const char *separator = "";

      for (size_t i = 0; i != 8; i += 1)
        {
          printf ("%s%02x", separator, hash1 % 256);
          separator = " ";
          hash1 /= 256;
        }

      for (size_t i = 0; i != 8; i += 1)
        {
          printf ("%s%02x", separator, hash2 % 256);
          hash2 /= 256;
        }

      printf("\n");
    }

  return 0;
}
