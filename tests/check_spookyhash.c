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
#include <stdio.h>
#include <gc/gc.h>
#include <libhiha/libhiha.h>

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

#define LENGTH_MAX 99999

#define SEED1_A 1
#define SEED2_A 2

#define SEED1_B 3
#define SEED2_B 4

static void
fill_message (char *message, size_t length)
{
  for (size_t i = 0; i != length; i += 1)
    message[i] = (char) (unsigned char) ((i + 128) % 256);
}

static void
test_with_seeds (uint64_t seed1, uint64_t seed2, FILE *f)
{
  char buf[1000];
  char message[LENGTH_MAX + 1];

  for (size_t length = 0; length != LENGTH_MAX + 1; length += 1)
    {
      fill_message (message, length);

      spookyhash_context_t context;
      uint64_t hash1;
      uint64_t hash2;

      spookyhash_init (&context, seed1, seed2);
      spookyhash_update (&context, message, length);
      spookyhash_final (&context, &hash1, &hash2);

      uint8_t nominal[2 * sizeof (uint64_t)];
      uint8_t little_endian[2 * sizeof (uint64_t)];
      uint8_t big_endian[2 * sizeof (uint64_t)];

      fgets (buf, 1000, f);
      unsigned int a, b, c, d, e, f, g, h;
      unsigned int i, j, k, l, m, n, o, p;
      sscanf (buf, ("%x %x %x %x %x %x %x %x "
                    "%x %x %x %x %x %x %x %x"),
              &a, &b, &c, &d, &e, &f, &g, &h,
              &i, &j, &k, &l, &m, &n, &o, &p);
      nominal[0] = a;
      nominal[1] = b;
      nominal[2] = c;
      nominal[3] = d;
      nominal[4] = e;
      nominal[5] = f;
      nominal[6] = g;
      nominal[7] = h;
      nominal[8] = i;
      nominal[9] = j;
      nominal[10] = k;
      nominal[11] = l;
      nominal[12] = m;
      nominal[13] = n;
      nominal[14] = o;
      nominal[15] = p;

      spookyhash_little_endian (hash1, &little_endian[0]);
      spookyhash_little_endian (hash1,
                                &little_endian[sizeof (uint64_t)]);

      spookyhash_big_endian (hash1, &big_endian[0]);
      spookyhash_big_endian (hash1, &big_endian[sizeof (uint64_t)]);

      for (size_t i = 0; i != sizeof (uint64_t); i += 1)
        {
          if (little_endian[i] != nominal[i]
              || big_endian[i] != nominal[sizeof (uint64_t) - 1 - i])
            {
              printf ("failed at length %zu\n", length);
              abort ();
            }
        }
    }
}

int
main (int argc, char **argv)
{
  if (argc < 2)
    abort ();
  FILE *f = fopen (argv[1], "r");
  if (f == NULL)
    abort ();

  test_with_seeds (SEED1_A, SEED2_A, f);
  test_with_seeds (SEED1_B, SEED2_B, f);

  fclose (f);
  return 0;
}
