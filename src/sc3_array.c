/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sc3_array.h>
#include <sc3_refcount.h>

struct sc3_array
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *aator;
  int                 setup;

  /* parameters fixed after setup call */
  int                 initzero, resizable, tighten;
  int                 ecount, ealloc;
  size_t              esize;

  /* member variables initialized in setup call */
  char               *mem;
};

int
sc3_array_is_valid (const sc3_array_t * a, char *reason)
{
  SC3E_TEST (a != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &a->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, a->aator, reason);
  SC3E_TEST (a->ecount >= 0 && a->ealloc >= 0, reason);

  /* check internal allocation logic depending on setup status */
  if (!a->setup) {
    SC3E_TEST (a->mem == NULL, reason);
  }
  else {
    SC3E_TEST (a->mem != NULL || a->ecount * a->esize == 0, reason);
    SC3E_TEST (a->ealloc == 0 || SC3_ISPOWOF2 (a->ealloc), reason);
    SC3E_TEST (a->ecount <= a->ealloc, reason);
  }
  SC3E_YES (reason);
}

int
sc3_array_is_new (const sc3_array_t * a, char *reason)
{
  SC3E_IS (sc3_array_is_valid, a, reason);
  SC3E_TEST (!a->setup, reason);
  SC3E_YES (reason);
}

int
sc3_array_is_setup (const sc3_array_t * a, char *reason)
{
  SC3E_IS (sc3_array_is_valid, a, reason);
  SC3E_TEST (a->setup, reason);
  SC3E_YES (reason);
}

int
sc3_array_is_resizable (const sc3_array_t * a, char *reason)
{
  SC3E_IS (sc3_array_is_setup, a, reason);
  SC3E_TEST (a->resizable, reason);
  SC3E_YES (reason);
}

int
sc3_array_is_unresizable (const sc3_array_t * a, char *reason)
{
  SC3E_IS (sc3_array_is_setup, a, reason);
  SC3E_TEST (!a->resizable, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_array_new (sc3_allocator_t * aator, sc3_array_t ** ap)
{
  sc3_array_t        *a;

  SC3E_RETVAL (ap, NULL);
  SC3A_IS (sc3_allocator_is_setup, aator);

  SC3E (sc3_allocator_ref (aator));
  SC3E (sc3_allocator_calloc_one (aator, sizeof (sc3_array_t), &a));
  SC3E (sc3_refcount_init (&a->rc));
  a->esize = 1;
  a->ealloc = 8;
  a->aator = aator;
  SC3A_IS (sc3_array_is_new, a);

  *ap = a;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_size (sc3_array_t * a, size_t esize)
{
  SC3A_IS (sc3_array_is_new, a);
  a->esize = esize;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_count (sc3_array_t * a, int ecount)
{
  SC3A_IS (sc3_array_is_new, a);
  SC3A_CHECK (0 <= ecount && ecount <= SC3_INT_HPOW);
  a->ecount = ecount;
  return NULL;
}

sc3_error_t        *
sc3_array_set_elem_alloc (sc3_array_t * a, int ealloc)
{
  SC3A_IS (sc3_array_is_new, a);
  SC3A_CHECK (0 <= ealloc && ealloc <= SC3_INT_HPOW);
  a->ealloc = ealloc;
  return NULL;
}

sc3_error_t        *
sc3_array_set_initzero (sc3_array_t * a, int initzero)
{
  SC3A_IS (sc3_array_is_new, a);
  a->initzero = initzero;
  return NULL;
}

sc3_error_t        *
sc3_array_set_resizable (sc3_array_t * a, int resizable)
{
  SC3A_IS (sc3_array_is_new, a);
  a->resizable = resizable;
  return NULL;
}

sc3_error_t        *
sc3_array_set_tighten (sc3_array_t * a, int tighten)
{
  SC3A_IS (sc3_array_is_new, a);
  a->tighten = tighten;
  return NULL;
}

sc3_error_t        *
sc3_array_setup (sc3_array_t * a)
{
  const int           ib = SC3_INT_BITS;
  int                 lg;
  size_t              abytes;

  SC3A_IS (sc3_array_is_new, a);

  /* set a->ealloc to a fitting power of 2 */
  lg = sc3_log2_ceil (SC3_MAX (a->ealloc, a->ecount), ib - 1);
  SC3A_CHECK (0 <= lg && lg < ib - 1);
  SC3A_CHECK (a->ecount <= (1 << lg));
  SC3A_CHECK (a->ealloc <= (1 << lg));
  abytes = (a->ealloc = 1 << lg) * a->esize;

  /* allocate array storage */
  if (!a->initzero) {
    SC3E (sc3_allocator_malloc (a->aator, abytes, &a->mem));
  }
  else {
    SC3E (sc3_allocator_calloc_one (a->aator, abytes, &a->mem));
  }

  /* set array to setup state */
  a->setup = 1;
  SC3A_IS (sc3_array_is_setup, a);
  return NULL;
}

sc3_error_t        *
sc3_array_ref (sc3_array_t * a)
{
  SC3A_IS (sc3_array_is_unresizable, a);
  SC3E (sc3_refcount_ref (&a->rc));
  return NULL;
}

sc3_error_t        *
sc3_array_unref (sc3_array_t ** ap)
{
  int                 waslast;
  sc3_allocator_t    *aator;
  sc3_array_t        *a;
  sc3_error_t        *leak = NULL;

  SC3E_INOUTP (ap, a);
  SC3A_IS (sc3_array_is_valid, a);
  SC3E (sc3_refcount_unref (&a->rc, &waslast));
  if (waslast) {
    *ap = NULL;

    aator = a->aator;
    if (a->setup) {
      /* deallocate element storage */
      SC3E (sc3_allocator_free (aator, a->mem));
    }
    SC3E (sc3_allocator_free (aator, a));
    SC3L (&leak, sc3_allocator_unref (&aator));
  }
  return leak;
}

sc3_error_t        *
sc3_array_destroy (sc3_array_t ** ap)
{
  sc3_error_t        *leak = NULL;
  sc3_array_t        *a;

  SC3E_INULLP (ap, a);
  SC3L_DEMAND (&leak, sc3_refcount_is_last (&a->rc, NULL));
  SC3L (&leak, sc3_array_unref (&a));

  SC3A_CHECK (a == NULL || leak != NULL);
  return leak;
}

sc3_error_t        *
sc3_array_resize (sc3_array_t * a, int new_ecount)
{
  SC3A_IS (sc3_array_is_resizable, a);
  SC3A_CHECK (0 <= new_ecount && new_ecount <= SC3_INT_HPOW);

  /* query whether the allocation is sufficient */
  if (new_ecount > a->ealloc) {

    /* we need to enlarge allocation */
    if (a->ealloc == 0) {
      a->ealloc = 1;
    }
    while (new_ecount > a->ealloc) {
      a->ealloc *= 2;
    }
    SC3A_CHECK (new_ecount <= a->ealloc);
    SC3E (sc3_allocator_realloc (a->aator, a->ealloc * a->esize, &a->mem));
  }
  else if (a->tighten && new_ecount < a->ealloc) {
    int                 newalloc;

    /* we shall try to reduce memory usage */
    if (new_ecount == 0) {
      newalloc = 0;
    }
    else {
      newalloc = a->ealloc;
      while (newalloc / 2 >= new_ecount) {
        newalloc /= 2;
      }
      SC3A_CHECK (newalloc > 0);
    }
    if (newalloc < a->ealloc) {
      a->ealloc = newalloc;
      SC3A_CHECK (new_ecount <= a->ealloc);
      SC3E (sc3_allocator_realloc (a->aator, a->ealloc * a->esize, &a->mem));
    }
  }

  /* record new element count */
  a->ecount = new_ecount;
  return NULL;
}

int
sc3_array_is_sorted (sc3_array_t * a,
                     sc3_error_t * (*compar) (const void *, const void *,
                                              int *),
                     char *reason)
{
  int                 count;
  int                 zz, j;
  void               *vold, *vnew;

  count = sc3_array_elem_count_noerr (a);

  if (count <= 1) {
    SC3E_YES (reason);
  }

  vold = sc3_array_index_noerr (a, 0);
  for (zz = 1; zz < count; ++zz) {
    vnew = sc3_array_index_noerr (a, zz);
    compar (vold, vnew, &j);
    SC3E_TEST (j <= 0, reason);
    vold = vnew;
  }

  SC3E_YES (reason);
}

sc3_error_t        *
sc3_array_split (sc3_array_t * a, sc3_array_t * offsets,
                 int num_types, sc3_array_type_t type_fn,
                 void *data)
{
  int              count;
  int              zi, *zp;
  int              guess, low, high, type, step;
  size_t           elem_size;

  SC3E (sc3_array_get_elem_size (offsets, &elem_size));
  SC3A_CHECK (elem_size == sizeof (int));
  SC3E (sc3_array_resize (offsets, num_types + 1));

  SC3E (sc3_array_get_elem_count (a, &count));

  /** The point of this algorithm is to put offsets[i] into its final position
   * for i = 0,...,num_types, where the final position of offsets[i] is the
   * unique index k such that type_fn (array, j, data) < i for all j < k
   * and type_fn (array, j, data) >= i for all j >= k.
   *
   * The invariants of the loop are:
   *  1) if i < step, then offsets[i] <= low, and offsets[i] is final.
   *  2) if i >= step, then low is less than or equal to the final value of
   *     offsets[i].
   *  3) for 0 <= i <= num_types, offsets[i] is greater than or equal to its
   *     final value.
   *  4) for every index k in the array with k < low,
   *     type_fn (array, k, data) < step,
   *  5) for 0 <= i < num_types,
   *     for every index k in the array with k >= offsets[i],
   *     type_fn (array, k, data) >= i.
   *  6) if i < j, offsets[i] <= offsets[j].
   *
   * Initializing offsets[0] = 0, offsets[i] = count for i > 0,
   * low = 0, and step = 1, the invariants are trivially satisfied.
   */
  SC3E (sc3_array_index (offsets, 0, &zp));
  *zp = 0;
  for (zi = 1; zi <= num_types; zi++) {
    SC3E (sc3_array_index (offsets, zi, &zp));
    *zp = count;
  }

  if (count == 0 || num_types <= 1) {
    return NULL;
  }

  /** Because count > 0 we can add another invariant:
   *   7) if step < num_types, low < high = offsets[step].
   */

  low = 0;
  high = count;                 /* high = offsets[step] */
  step = 1;
  for (;;) {
    guess = low + (high - low) / 2;     /* By (7) low <= guess < high. */
    SC3E (type_fn (a, guess, data, &type));
    SC3A_CHECK (type < num_types);
    /** If type < step, then we can set low = guess + 1 and still satisfy
     * invariant (4).  Also, because guess < high, we are assured low <= high.
     */
    if (type < step) {
      low = guess + 1;
    }
    /** If type >= step, then setting offsets[i] = guess for i = step,..., type
     * still satisfies invariant (5).  Because guess >= low, we are assured
     * low <= high, and we maintain invariant (6).
     */
    else {
      for (zi = step; zi <= type; zi++) {
        SC3E (sc3_array_index (offsets, zi, &zp));
        *zp = guess;
      }
      high = guess;             /* high = offsets[step] */
    }
    /** If low = (high = offsets[step]), then by invariants (2) and (3)
     * offsets[step] is in its final position, so we can increment step and
     * still satisfy invariant (1).
     */
    while (low == high) {
      /* By invariant (6), high cannot decrease here */
      ++step;                   /* sc_array_index might be a macro */
      SC3E (sc3_array_index (offsets, step, &zp));
      high = *zp;
      /** If step = num_types, then by invariant (1) we have found the final
       * positions for offsets[i] for i < num_types, and offsets[num_types] =
       * count in all situations, so we are done.
       */
      if (step == num_types) {
        return NULL;
      }
    }
    /** To reach this point it must be true that low < high, so we preserve
     * invariant (7).
     */
  }
  return NULL;
}

sc3_error_t        *
sc3_array_push_count (sc3_array_t * a, int n, void *ptr)
{
  SC3A_IS (sc3_array_is_resizable, a);
  SC3A_CHECK (0 <= n && a->ecount + n <= SC3_INT_HPOW);

  /* preinitialize output variable */
  if (ptr != NULL) {
    *(void **) ptr = NULL;
  }

  /* reallocate to fit the new members */
  if (n > 0) {
    int                 old_ecount = a->ecount;
    SC3E (sc3_array_resize (a, old_ecount + n));
    if (ptr != NULL) {
      SC3E (sc3_array_index (a, old_ecount, ptr));
    }
  }
  return NULL;
}

sc3_error_t        *
sc3_array_push (sc3_array_t * a, void *ptr)
{
  SC3E (sc3_array_push_count (a, 1, ptr));
  return NULL;
}

sc3_error_t        *
sc3_array_pop (sc3_array_t * a)
{
  SC3A_IS (sc3_array_is_resizable, a);
  SC3A_CHECK (a->ecount > 0);

  /* shrink array by one */
  SC3E (sc3_array_resize (a, a->ecount - 1));
  return NULL;
}

sc3_error_t        *
sc3_array_freeze (sc3_array_t * a)
{
  SC3A_IS (sc3_array_is_setup, a);
  if (a->resizable) {
    if (a->tighten && a->ecount < a->ealloc) {
      a->ealloc = a->ecount;
      SC3E (sc3_allocator_realloc (a->aator, a->ealloc * a->esize, &a->mem));
    }
    a->resizable = 0;
  }
  return NULL;
}

sc3_error_t        *
sc3_array_index (sc3_array_t * a, int i, void *ptr)
{
  SC3A_IS (sc3_array_is_setup, a);
  SC3A_CHECK (0 <= i && i < a->ecount);
  SC3A_CHECK (ptr != NULL);

  *(void **) ptr = a->mem + i * a->esize;
  return NULL;
}

void               *
sc3_array_index_noerr (const sc3_array_t * a, int i)
{
#ifdef SC_ENABLE_DEBUG
  if (!sc3_array_is_setup (a, NULL) || i < 0 || i >= a->ecount) {
    return NULL;
  }
#endif
  return a->mem + i * a->esize;
}

sc3_error_t        *
sc3_array_get_elem_size (sc3_array_t * a, size_t *esize)
{
  SC3E_RETVAL (esize, 0);
  SC3A_IS (sc3_array_is_setup, a);

  *esize = a->esize;
  return NULL;
}

sc3_error_t        *
sc3_array_get_elem_count (sc3_array_t * a, int *ecount)
{
  SC3E_RETVAL (ecount, 0);
  SC3A_IS (sc3_array_is_setup, a);

  *ecount = a->ecount;
  return NULL;
}

int
sc3_array_elem_count_noerr (const sc3_array_t * a)
{
#ifdef SC_ENABLE_DEBUG
  return sc3_array_is_setup (a, NULL) ? a->ecount : 0;
#else
  return a->ecount;
#endif
}
