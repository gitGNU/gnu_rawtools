/*** numrange.h --- ?  -*- C -*- */

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
#ifndef NUMRANGE_H
#define NUMRANGE_H

#include <stddef.h>             /* for size_t */
#include <stdint.h>

void nrange_extend_int8_tt    (const int8_t   *vec, size_t size,
                               int8_t *min,   int8_t *max);
void nrange_extend_int16_tt   (const int16_t  *vec, size_t size,
                               int16_t *min,  int16_t *max);
void nrange_extend_int32_tt   (const int32_t  *vec, size_t size,
                               int32_t *min,  int32_t *max);
void nrange_extend_int64_tt   (const int64_t  *vec, size_t size,
                               int64_t *min,  int64_t *max);

void nrange_extend_uint8_tt   (const uint8_t  *vec, size_t size,
                               uint8_t *min,  uint8_t *max);
void nrange_extend_uint16_tt  (const uint16_t *vec, size_t size,
                               uint16_t *min, uint16_t *max);
void nrange_extend_uint32_tt  (const uint32_t *vec, size_t size,
                               uint32_t *min, uint32_t *max);
void nrange_extend_uint64_tt  (const uint64_t *vec, size_t size,
                               uint64_t *min, uint64_t *max);

void nrange_extend_float      (const float    *vec, size_t size,
                               float *min,    float *max);
void nrange_extend_double     (const double   *vec, size_t size,
                               double *min,   double *max);

#endif
/*** Emacs stuff */
/** Local variables: */
/** fill-column: 72 */
/** indent-tabs-mode: nil */
/** ispell-local-dictionary: "british" */
/** mode: outline-minor */
/** outline-regexp: "/[*][*][*]" */
/** End: */
/** LocalWords:   */
/*** numrange.h ends here */
