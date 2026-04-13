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

#ifndef __LIBHAHA__SPINLOCK_H__INCLUDED__
#define __LIBHAHA__SPINLOCK_H__INCLUDED__

#include <stdatomic.h>

/* A ticketing system is employed, to help ensure an attempt to
   acquire a lock does indeed get its turn. */

struct spinlock_t
{
  atomic_size_t active;
  atomic_size_t available;
};
typedef struct spinlock_t spinlock_t;

#define SPINLOCK_T_INIT { .active = 0, .available = 0 }

void acquire_spinlock (spinlock_t *lock);
void release_spinlock (spinlock_t *lock);

#endif /* __LIBHAHA__SPINLOCK_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
