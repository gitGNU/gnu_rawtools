/*** numrange.c --- C Numeric Types Extrema Finding  -*- C -*- */

/*** Copyright (C) 2007 Ivan Shmakov */

/** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful, but
 ** WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 ** 02110-1301 USA
 */

/*** Code: */
#include <stddef.h>             /* for size_t */
#include <stdint.h>

#define NUM_RANGE_EXTEND(fn, type) \
    void \
    fn (const type *vec, size_t size, type *min, type *max) { \
      size_t rest; \
      const type *p; \
      type min1, max1; \
      if (size < 1) { /* . */ return; } \
      min1 = (min != 0) ? *min : vec[0]; \
      max1 = (max != 0) ? *max : vec[0]; \
      for (rest = size, p = vec; \
           rest > 0; \
           rest--, p++) { \
        if (*p < min1) { min1 = *p; } \
        if (*p > max1) { max1 = *p; } \
      } \
      if (min != 0) { *min = min1; } \
      if (max != 0) { *max = max1; } \
    }

NUM_RANGE_EXTEND (nrange_extend_int8_tt,        int8_t)
NUM_RANGE_EXTEND (nrange_extend_int16_tt,       int16_t)
NUM_RANGE_EXTEND (nrange_extend_int32_tt,       int32_t)
NUM_RANGE_EXTEND (nrange_extend_int64_tt,       int64_t)

NUM_RANGE_EXTEND (nrange_extend_uint8_tt,       uint8_t)
NUM_RANGE_EXTEND (nrange_extend_uint16_tt,      uint16_t)
NUM_RANGE_EXTEND (nrange_extend_uint32_tt,      uint32_t)
NUM_RANGE_EXTEND (nrange_extend_uint64_tt,      uint64_t)

NUM_RANGE_EXTEND (nrange_extend_float,          float)
NUM_RANGE_EXTEND (nrange_extend_double,         double)

/*** Emacs stuff */
/** Local variables: */
/** fill-column: 72 */
/** indent-tabs-mode: nil */
/** ispell-local-dictionary: "british" */
/** mode: outline-minor */
/** outline-regexp: "/[*][*][*]" */
/** End: */
/** LocalWords:   */
/*** numrange.c ends here */
