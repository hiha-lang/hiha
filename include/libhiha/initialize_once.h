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

#ifndef __LIBHAHA__INITIALIZE_ONCE_H__INCLUDED__
#define __LIBHAHA__INITIALIZE_ONCE_H__INCLUDED__

#include <stdatomic.h>
#include <libhiha/spinlock.h>

struct initialize_once_t
{
  spinlock_t lock;
  atomic_bool initialized;
};
typedef struct initialize_once_t initialize_once_t;

#define INITIALIZE_ONCE_T_INIT                          \
  { .lock = SPINLOCK_T_INIT, .initialized = false }

/* Double-checked locking to initialize a value. To keep this
   implementation agnostic about the threads system, I have written it
   with my own spinlocks. */
#define INITIALIZE_ONCE(INIT1T, INITIALIZE_VALUE)               \
  do {                                                          \
    bool initialized =                                          \
      atomic_load_explicit (&(INIT1T).initialized,              \
                            memory_order_acquire);              \
    if (!initialized)                                           \
      {                                                         \
        acquire_spinlock (&(INIT1T).lock);                      \
        initialized =                                           \
          atomic_load_explicit (&(INIT1T).initialized,          \
                                memory_order_relaxed);          \
        if (!initialized)                                       \
          {                                                     \
            INITIALIZE_VALUE ();                                \
            atomic_store_explicit (&(INIT1T).initialized,       \
                                   true, memory_order_release); \
          }                                                     \
        release_spinlock (&(INIT1T).lock);                      \
      }                                                         \
  } while (0)

#endif /* __LIBHAHA__INITIALIZE_ONCE_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
