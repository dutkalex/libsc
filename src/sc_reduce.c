/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2010 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sc_reduce.h>
#include <sc_search.h>

#ifdef SC_MPI

static void
sc_reduce_recursive (MPI_Comm mpicomm,
                     char *data, int count, MPI_Datatype datatype,
                     int groupsize, int target,
                     int maxlevel, int level, int branch,
                     sc_reduce_t reduce_fn)
{
  int                 mpiret;
  int                 orig_target;
  int                 myrank, peer, higher;
  int                 doall;
  char               *peerdata;
  size_t              datasize;
  MPI_Status          rstatus;

  orig_target = target;
  doall = 0;
  if (target == -1) {
    doall = 1;
    target = 0;
  }

  SC_ASSERT (0 <= target && target < groupsize);

  myrank = sc_search_bias (maxlevel, level, branch, target);

  SC_ASSERT (0 <= myrank && myrank < groupsize);
  SC_ASSERT (reduce_fn != NULL);

  if (level == 0) {
    /* result is in data */
  }
  else {
    /* *INDENT-OFF* HORRIBLE indent bug */
    datasize = (size_t) count * sc_mpi_sizeof (datatype);
    /* *INDENT-ON* */
    peer = sc_search_bias (maxlevel, level, branch ^ 0x01, target);
    SC_ASSERT (peer != myrank);

    higher = sc_search_bias (maxlevel, level - 1, branch / 2, target);
    if (myrank == higher) {
      if (peer < groupsize) {
        /* temporary data to compare against peer */
        peerdata = SC_ALLOC (char, datasize);

        mpiret = MPI_Recv (peerdata, datasize, MPI_BYTE,
                           peer, SC_TAG_REDUCE, mpicomm, &rstatus);
        SC_CHECK_MPI (mpiret);

        /* execute reduction operation here */
        reduce_fn (peerdata, data, count, datatype);
        SC_FREE (peerdata);
      }

      /* execute next higher level of recursion */
      sc_reduce_recursive (mpicomm, data, count, datatype,
                           groupsize, orig_target,
                           maxlevel, level - 1, branch / 2, reduce_fn);

      if (doall && peer < groupsize) {
        /* if allreduce send back result of reduction */
        mpiret = MPI_Send (data, datasize, MPI_BYTE,
                           peer, SC_TAG_REDUCE, mpicomm);
        SC_CHECK_MPI (mpiret);
      }
    }
    else {
      if (peer < groupsize) {
        mpiret = MPI_Send (data, datasize, MPI_BYTE,
                           peer, SC_TAG_REDUCE, mpicomm);
        SC_CHECK_MPI (mpiret);
        if (doall) {
          /* if allreduce receive back result of reduction */
          mpiret = MPI_Recv (data, datasize, MPI_BYTE,
                             peer, SC_TAG_REDUCE, mpicomm, &rstatus);
          SC_CHECK_MPI (mpiret);
        }
      }
    }
  }
}

#endif /* SC_MPI */

static void
sc_reduce_max (void *sendbuf, void *recvbuf,
               int sendcount, MPI_Datatype sendtype)
{
  int                 i;

  switch (sendtype) {
  case MPI_CHAR:
  case MPI_BYTE:
    {
      const char         *s = (char *) sendbuf;
      char               *r = (char *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_SHORT:
    {
      const short        *s = (short *) sendbuf;
      short              *r = (short *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_UNSIGNED_SHORT:
    {
      const unsigned short *s = (unsigned short *) sendbuf;
      unsigned short     *r = (unsigned short *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_INT:
    {
      const int          *s = (int *) sendbuf;
      int                *r = (int *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_UNSIGNED:
    {
      const unsigned     *s = (unsigned *) sendbuf;
      unsigned           *r = (unsigned *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_LONG:
    {
      const long         *s = (long *) sendbuf;
      long               *r = (long *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_UNSIGNED_LONG:
    {
      const unsigned long *s = (unsigned long *) sendbuf;
      unsigned long      *r = (unsigned long *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_LONG_LONG_INT:
    {
      const long long    *s = (long long *) sendbuf;
      long long          *r = (long long *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_FLOAT:
    {
      const float        *s = (float *) sendbuf;
      float              *r = (float *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_DOUBLE:
    {
      const double       *s = (double *) sendbuf;
      double             *r = (double *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  case MPI_LONG_DOUBLE:
    {
      const long double  *s = (long double *) sendbuf;
      long double        *r = (long double *) recvbuf;
      for (i = 0; i < sendcount; ++i)
        if (s[i] > r[i])
          r[i] = s[i];
      break;
    }
  default:
    SC_ABORT ("Unsupported MPI datatype in sc_reduce_max");
  }
}

static int
sc_reduce_custom_dispatch (void *sendbuf, void *recvbuf, int sendcount,
                           MPI_Datatype sendtype, sc_reduce_t reduce_fn,
                           int target, MPI_Comm mpicomm)
{
#ifdef SC_MPI
  int                 mpiret;
  int                 mpisize;
  int                 mpirank;
  int                 maxlevel;
#endif
  size_t              datasize;

  SC_ASSERT (sendcount >= 0);

  /* *INDENT-OFF* HORRIBLE indent bug */
  datasize = (size_t) sendcount * sc_mpi_sizeof (sendtype);
  /* *INDENT-ON* */
  memcpy (recvbuf, sendbuf, datasize);

#ifdef SC_MPI
  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  SC_ASSERT (-1 <= target && target < mpisize);

  maxlevel = SC_LOG2_32 (mpisize - 1) + 1;
  sc_reduce_recursive (mpicomm, recvbuf, sendcount, sendtype, mpisize,
                       target, maxlevel, maxlevel, mpirank, reduce_fn);
#endif

  return MPI_SUCCESS;
}

int
sc_allreduce_custom (void *sendbuf, void *recvbuf, int sendcount,
                     MPI_Datatype sendtype, sc_reduce_t reduce_fn,
                     MPI_Comm mpicomm)
{
  return sc_reduce_custom_dispatch (sendbuf, recvbuf, sendcount,
                                    sendtype, reduce_fn, -1, mpicomm);
}

int
sc_reduce_custom (void *sendbuf, void *recvbuf, int sendcount,
                  MPI_Datatype sendtype, sc_reduce_t reduce_fn,
                  int target, MPI_Comm mpicomm)
{
  SC_CHECK_ABORT (target >= 0,
                  "sc_reduce_custom requires non-negative target");

  return sc_reduce_custom_dispatch (sendbuf, recvbuf, sendcount,
                                    sendtype, reduce_fn, target, mpicomm);
}

static int
sc_reduce_dispatch (void *sendbuf, void *recvbuf, int sendcount,
                    MPI_Datatype sendtype, MPI_Op operation,
                    int target, MPI_Comm mpicomm)
{
  sc_reduce_t         reduce_fn;

  switch (operation) {
  case MPI_MAX:
    reduce_fn = sc_reduce_max;
    break;
  default:
    reduce_fn = NULL;
    SC_ABORT ("Unsupported operation in sc_allreduce or sc_reduce");
  }

  return sc_reduce_custom_dispatch (sendbuf, recvbuf, sendcount,
                                    sendtype, reduce_fn, target, mpicomm);
}

int
sc_allreduce (void *sendbuf, void *recvbuf, int sendcount,
              MPI_Datatype sendtype, MPI_Op operation, MPI_Comm mpicomm)
{
  return sc_reduce_dispatch (sendbuf, recvbuf, sendcount,
                             sendtype, operation, -1, mpicomm);
}

int
sc_reduce (void *sendbuf, void *recvbuf, int sendcount,
           MPI_Datatype sendtype, MPI_Op operation,
           int target, MPI_Comm mpicomm)
{
  SC_CHECK_ABORT (target >= 0, "sc_reduce requires non-negative target");

  return sc_reduce_dispatch (sendbuf, recvbuf, sendcount,
                             sendtype, operation, target, mpicomm);
}