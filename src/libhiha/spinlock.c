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
#include <stdlib.h>
#include <libhiha/spinlock.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

#if HAVE___BUILTIN_IA32_PAUSE
#define CPU_PAUSE __builtin_ia32_pause ()
#elif #if HAVE__MM_PAUSE
#include <immintrin.h>
#define CPU_PAUSE _mm_pause ()
#else /* FIXME: Support pauses on more platforms. */
#define CPU_PAUSE do {} while (0)
#endif

static inline void
spinlock_pause (void)
{
  CPU_PAUSE;
}

VISIBLE void
acquire_spinlock (spinlock_t *lock)
{
  size_t my_ticket = atomic_fetch_add (&lock->available, 1);
  size_t active_ticket = atomic_load (&lock->active);
  while (my_ticket != active_ticket)
    {
      spinlock_pause ();
      active_ticket = atomic_load (&lock->active);
    }
  atomic_thread_fence (memory_order_seq_cst);
}

VISIBLE void
release_spinlock (spinlock_t *lock)
{
  atomic_thread_fence (memory_order_seq_cst);
  atomic_fetch_add (&lock->active, 1);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
