/*
  This file is part of p4est.
  p4est is a C library to manage a parallel collection of quadtrees and/or
  octrees.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef P4_TO_P8
#include <p4est_algorithms.h>
#include <p4est_bits.h>
#include <p4est_communication.h>
#include <p4est_vtk.h>
#else
#include <p8est_algorithms.h>
#include <p8est_bits.h>
#include <p8est_communication.h>
#include <p8est_vtk.h>
#endif

static void
p4est_coarsen_old (p4est_t * p4est, int coarsen_recursive,
                   p4est_coarsen_t coarsen_fn, p4est_init_t init_fn)
{
#ifdef P4EST_DEBUG
  size_t              data_pool_size;
#endif
  int                 i, maxlevel;
  int                 couldbegood;
  size_t              zz;
  size_t              incount, removed;
  size_t              cidz, first, last, rest, before;
  p4est_locidx_t      num_quadrants, prev_offset;
  p4est_topidx_t      jt;
  p4est_tree_t       *tree;
  p4est_quadrant_t   *c[P4EST_CHILDREN];
  p4est_quadrant_t   *cfirst, *clast;
  sc_array_t         *tquadrants;

  P4EST_GLOBAL_PRODUCTIONF ("Into " P4EST_STRING
                            "_coarsen_old with %lld total quadrants\n",
                            (long long) p4est->global_num_quadrants);
  P4EST_ASSERT (p4est_is_valid (p4est));

  /* loop over all local trees */
  prev_offset = 0;
  for (jt = p4est->first_local_tree; jt <= p4est->last_local_tree; ++jt) {
    tree = p4est_tree_array_index (p4est->trees, jt);
    tquadrants = &tree->quadrants;
#ifdef P4EST_DEBUG
    data_pool_size = 0;
    if (p4est->user_data_pool != NULL) {
      data_pool_size = p4est->user_data_pool->elem_count;
    }
#endif
    removed = 0;

    /* initial log message for this tree */
    P4EST_VERBOSEF ("Into coarsen tree %lld with %llu\n", (long long) jt,
                    (unsigned long long) tquadrants->elem_count);

    /* Initialize array indices.
       If children are coarsened, the array will have an empty window.
       first   index of the first child to be considered
       last    index of the last child before the hole in the array
       before  number of children before the hole in the array
       rest    index of the first child after the hole in the array
     */
    first = last = 0;
    before = rest = 1;

    /* run through the array and coarsen recursively */
    incount = tquadrants->elem_count;
    while (rest + P4EST_CHILDREN - 1 - before < incount) {
      couldbegood = 1;
      for (zz = 0; zz < P4EST_CHILDREN; ++zz) {
        if (zz < before) {
          c[zz] = p4est_quadrant_array_index (tquadrants, first + zz);
          if (zz != (size_t) p4est_quadrant_child_id (c[zz])) {
            couldbegood = 0;
            break;
          }
        }
        else {
          c[zz] = p4est_quadrant_array_index (tquadrants, rest + zz - before);
        }
      }
      if (couldbegood && p4est_quadrant_is_familypv (c) &&
          coarsen_fn (p4est, jt, c)) {
        /* coarsen now */
        for (zz = 0; zz < P4EST_CHILDREN; ++zz) {
          p4est_quadrant_free_data (p4est, c[zz]);
        }
        tree->quadrants_per_level[c[0]->level] -= P4EST_CHILDREN;
        cfirst = c[0];
        p4est_quadrant_parent (c[0], cfirst);
        p4est_quadrant_init_data (p4est, jt, cfirst, init_fn);
        tree->quadrants_per_level[cfirst->level] += 1;
        p4est->local_num_quadrants -= P4EST_CHILDREN - 1;
        removed += P4EST_CHILDREN - 1;

        rest += P4EST_CHILDREN - before;
        if (coarsen_recursive) {
          last = first;
          cidz = (size_t) p4est_quadrant_child_id (cfirst);
          if (cidz > first)
            first = 0;
          else
            first -= cidz;
        }
        else {
          /* don't coarsen again, move the counters and the hole */
          P4EST_ASSERT (first == last && before == 1);
          if (rest < incount) {
            ++first;
            cfirst = p4est_quadrant_array_index (tquadrants, first);
            clast = p4est_quadrant_array_index (tquadrants, rest);
            *cfirst = *clast;
            last = first;
            ++rest;
          }
        }
      }
      else {
        /* do nothing, just move the counters and the hole */
        ++first;
        if (first > last) {
          if (first != rest) {
            cfirst = p4est_quadrant_array_index (tquadrants, first);
            clast = p4est_quadrant_array_index (tquadrants, rest);
            *cfirst = *clast;
          }
          last = first;
          ++rest;
        }
      }
      before = last - first + 1;
    }

    /* adjust final array size */
    first = last;
    if (first + 1 < rest) {
      while (rest < incount) {
        ++first;
        cfirst = p4est_quadrant_array_index (tquadrants, first);
        clast = p4est_quadrant_array_index (tquadrants, rest);
        *cfirst = *clast;
        ++rest;
      }
      sc_array_resize (tquadrants, first + 1);
    }

    /* compute maximum level */
    maxlevel = 0;
    num_quadrants = 0;
    for (i = 0; i <= P4EST_QMAXLEVEL; ++i) {
      P4EST_ASSERT (tree->quadrants_per_level[i] >= 0);
      num_quadrants += tree->quadrants_per_level[i];    /* same type */
      if (tree->quadrants_per_level[i] > 0) {
        maxlevel = i;
      }
    }
    tree->maxlevel = (int8_t) maxlevel;
    tree->quadrants_offset = prev_offset;
    prev_offset += num_quadrants;

    /* do some sanity checks */
    P4EST_ASSERT (num_quadrants == (p4est_locidx_t) tquadrants->elem_count);
    P4EST_ASSERT (tquadrants->elem_count == incount - removed);
    if (p4est->user_data_pool != NULL) {
      P4EST_ASSERT (data_pool_size - removed ==
                    p4est->user_data_pool->elem_count);
    }
    P4EST_ASSERT (p4est_tree_is_sorted (tree));
    P4EST_ASSERT (p4est_tree_is_complete (tree));

    /* final log message for this tree */
    P4EST_VERBOSEF ("Done coarsen tree %lld now %llu\n", (long long) jt,
                    (unsigned long long) tquadrants->elem_count);
  }
  if (p4est->last_local_tree >= 0) {
    for (; jt < p4est->connectivity->num_trees; ++jt) {
      tree = p4est_tree_array_index (p4est->trees, jt);
      tree->quadrants_offset = p4est->local_num_quadrants;
    }
  }

  /* compute global number of quadrants */
  p4est_comm_count_quadrants (p4est);

  P4EST_ASSERT (p4est_is_valid (p4est));
  P4EST_GLOBAL_PRODUCTIONF ("Done " P4EST_STRING
                            "_coarsen_old with %lld total quadrants\n",
                            (long long) p4est->global_num_quadrants);
}

#ifndef P4_TO_P8
static const int    refine_level = 6;
#else
static const int    refine_level = 4;
#endif
static int          coarsen_all = 1;

static int
refine_fn (p4est_t * p4est, p4est_topidx_t which_tree,
           p4est_quadrant_t * quadrant)
{
  if ((int) quadrant->level >= (refine_level - (int) (which_tree % 3))) {
    return 0;
  }

  return 1;
}

static int
coarsen_fn (p4est_t * p4est, p4est_topidx_t which_tree,
            p4est_quadrant_t * q[])
{
  SC_CHECK_ABORT (p4est_quadrant_is_familypv (q), "Coarsen invocation");

  return coarsen_all || q[0]->y >= P4EST_ROOT_LEN / 2;
}

static void
p4est_coarsen_both (p4est_t * p4est, int coarsen_recursive,
                    p4est_coarsen_t coarsen_fn, p4est_init_t init_fn)
{
  int                 success;
  p4est_t            *copy;

  copy = p4est_copy (p4est, 1);
  p4est_coarsen_old (copy, coarsen_recursive, coarsen_fn, init_fn);
  p4est_coarsen (p4est, coarsen_recursive, coarsen_fn, init_fn);

  success = p4est_is_equal (p4est, copy, 1);
  SC_CHECK_ABORT (success, "Coarsen mismatch");

  p4est_destroy (copy);
}

int
main (int argc, char **argv)
{
  int                 mpirank, mpisize;
  int                 mpiret;
  MPI_Comm            mpicomm;
  p4est_t            *p4est;
  p4est_connectivity_t *connectivity;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = MPI_COMM_WORLD;
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);
  p4est_init (NULL, SC_LP_DEFAULT);

  /* create connectivity and forest structures */
#ifdef P4_TO_P8
  connectivity = p8est_connectivity_new_rotcubes ();
#else
  connectivity = p4est_connectivity_new_star ();
#endif
  p4est = p4est_new (mpicomm, connectivity, 15, 0, NULL, NULL);
  p4est_refine (p4est, 1, refine_fn, NULL);
  p4est_balance (p4est, P4EST_BALANCE_FULL, NULL);

  coarsen_all = 1;
  p4est_coarsen_both (p4est, 0, coarsen_fn, NULL);
  coarsen_all = 0;
  p4est_coarsen_both (p4est, 1, coarsen_fn, NULL);
  p4est_balance (p4est, P4EST_BALANCE_FULL, NULL);
  coarsen_all = 1;
  p4est_coarsen_both (p4est, 1, coarsen_fn, NULL);
  p4est_vtk_write_file (p4est, NULL, P4EST_STRING "_endcoarsen");

  if (mpisize == 1) {
    SC_CHECK_ABORT (p4est->global_num_quadrants ==
                    (p4est_gloidx_t) connectivity->num_trees, "Coarsen");
  }

  p4est_destroy (p4est);
  p4est_connectivity_destroy (connectivity);
  sc_finalize ();

  mpiret = MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}