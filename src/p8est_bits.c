/*
  This file is part of p4est.
  p4est is a C library to manage a parallel collection of quadtrees and/or
  octrees.

  Copyright (C) 2007-2009 Carsten Burstedde, Lucas Wilcox.

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

#include <p4est_to_p8est.h>
#include "p4est_bits.c"

int
p8est_quadrant_is_outside_edge (const p4est_quadrant_t * q)
{
  int                 outface[P4EST_DIM];

  outface[0] = (int) (q->x < 0 || q->x >= P4EST_ROOT_LEN);
  outface[1] = (int) (q->y < 0 || q->y >= P4EST_ROOT_LEN);
  outface[2] = (int) (q->z < 0 || q->z >= P4EST_ROOT_LEN);

  return outface[0] + outface[1] + outface[2] == 2;
}

int
p8est_quadrant_is_outside_edge_extra (const p4est_quadrant_t * q, int *edge)
{
  int                 quad_contact[P4EST_FACES];
  int                 face_axis[P4EST_DIM];

  P4EST_ASSERT (q->level <= P4EST_QMAXLEVEL);

  quad_contact[0] = (int) (q->x < 0);
  quad_contact[1] = (int) (q->x >= P4EST_ROOT_LEN);
  quad_contact[2] = (int) (q->y < 0);
  quad_contact[3] = (int) (q->y >= P4EST_ROOT_LEN);
  quad_contact[4] = (int) (q->z < 0);
  quad_contact[5] = (int) (q->z >= P4EST_ROOT_LEN);
  face_axis[0] = quad_contact[0] || quad_contact[1];
  face_axis[1] = quad_contact[2] || quad_contact[3];
  face_axis[2] = quad_contact[4] || quad_contact[5];

  if (face_axis[0] + face_axis[1] + face_axis[2] != 2) {
    return 0;
  }

  if (edge != NULL) {
    if (!face_axis[0]) {
      *edge = 0 + 2 * quad_contact[5] + quad_contact[3];
    }
    else if (!face_axis[1]) {
      *edge = 4 + 2 * quad_contact[5] + quad_contact[1];
    }
    else if (!face_axis[2]) {
      *edge = 8 + 2 * quad_contact[3] + quad_contact[1];
    }
    else {
      SC_ABORT_NOT_REACHED ();
    }
    P4EST_ASSERT (p8est_quadrant_touches_edge (q, *edge, 0));
  }

  return 1;
}

int
p4est_quadrant_is_family (const p4est_quadrant_t * q0,
                          const p4est_quadrant_t * q1,
                          const p4est_quadrant_t * q2,
                          const p4est_quadrant_t * q3,
                          const p4est_quadrant_t * q4,
                          const p4est_quadrant_t * q5,
                          const p4est_quadrant_t * q6,
                          const p4est_quadrant_t * q7)
{
  const int8_t        level = q0->level;
  p4est_qcoord_t      inc;

  P4EST_ASSERT (p4est_quadrant_is_extended (q0));
  P4EST_ASSERT (p4est_quadrant_is_extended (q1));
  P4EST_ASSERT (p4est_quadrant_is_extended (q2));
  P4EST_ASSERT (p4est_quadrant_is_extended (q3));
  P4EST_ASSERT (p4est_quadrant_is_extended (q4));
  P4EST_ASSERT (p4est_quadrant_is_extended (q5));
  P4EST_ASSERT (p4est_quadrant_is_extended (q6));
  P4EST_ASSERT (p4est_quadrant_is_extended (q7));

  if (level == 0 || level != q1->level ||
      level != q2->level || level != q3->level ||
      level != q4->level || level != q5->level ||
      level != q6->level || level != q7->level) {
    return 0;
  }

  inc = P4EST_QUADRANT_LEN (level);
  return ((q0->x + inc == q1->x && q0->y == q1->y && q0->z == q1->z) &&
          (q0->x == q2->x && q0->y + inc == q2->y && q0->z == q2->z) &&
          (q1->x == q3->x && q2->y == q3->y && q0->z == q3->z) &&
          (q0->x == q4->x && q0->y == q4->y && q0->z + inc == q4->z) &&
          (q1->x == q5->x && q1->y == q5->y && q4->z == q5->z) &&
          (q2->x == q6->x && q2->y == q6->y && q4->z == q6->z) &&
          (q3->x == q7->x && q3->y == q7->y && q4->z == q7->z));
}

int
p4est_quadrant_is_familyv (const p4est_quadrant_t q[])
{
  const int8_t        level = q[0].level;
  p4est_qcoord_t      inc;

  P4EST_ASSERT (p4est_quadrant_is_extended (&q[0]));
  P4EST_ASSERT (p4est_quadrant_is_extended (&q[1]));
  P4EST_ASSERT (p4est_quadrant_is_extended (&q[2]));
  P4EST_ASSERT (p4est_quadrant_is_extended (&q[3]));
  P4EST_ASSERT (p4est_quadrant_is_extended (&q[4]));
  P4EST_ASSERT (p4est_quadrant_is_extended (&q[5]));
  P4EST_ASSERT (p4est_quadrant_is_extended (&q[6]));
  P4EST_ASSERT (p4est_quadrant_is_extended (&q[7]));

  if (level == 0 || level != q[1].level ||
      level != q[2].level || level != q[3].level ||
      level != q[4].level || level != q[5].level ||
      level != q[6].level || level != q[7].level) {
    return 0;
  }

  inc = P4EST_QUADRANT_LEN (level);
  return ((q[0].x + inc == q[1].x && q[0].y == q[1].y && q[0].z == q[1].z) &&
          (q[0].x == q[2].x && q[0].y + inc == q[2].y && q[0].z == q[2].z) &&
          (q[1].x == q[3].x && q[2].y == q[3].y && q[0].z == q[3].z) &&
          (q[0].x == q[4].x && q[0].y == q[4].y && q[0].z + inc == q[4].z) &&
          (q[1].x == q[5].x && q[1].y == q[5].y && q[4].z == q[5].z) &&
          (q[2].x == q[6].x && q[2].y == q[6].y && q[4].z == q[6].z) &&
          (q[3].x == q[7].x && q[3].y == q[7].y && q[4].z == q[7].z));
}

int
p4est_quadrant_is_familypv (p4est_quadrant_t * q[])
{
  const int8_t        level = q[0]->level;
  p4est_qcoord_t      inc;

  P4EST_ASSERT (p4est_quadrant_is_extended (q[0]));
  P4EST_ASSERT (p4est_quadrant_is_extended (q[1]));
  P4EST_ASSERT (p4est_quadrant_is_extended (q[2]));
  P4EST_ASSERT (p4est_quadrant_is_extended (q[3]));
  P4EST_ASSERT (p4est_quadrant_is_extended (q[4]));
  P4EST_ASSERT (p4est_quadrant_is_extended (q[5]));
  P4EST_ASSERT (p4est_quadrant_is_extended (q[6]));
  P4EST_ASSERT (p4est_quadrant_is_extended (q[7]));

  if (level == 0 || level != q[1]->level ||
      level != q[2]->level || level != q[3]->level ||
      level != q[4]->level || level != q[5]->level ||
      level != q[6]->level || level != q[7]->level) {
    return 0;
  }

  inc = P4EST_QUADRANT_LEN (level);
  return ((q[0]->x + inc == q[1]->x && q[0]->y == q[1]->y
           && q[0]->z == q[1]->z) && (q[0]->x == q[2]->x
                                      && q[0]->y + inc == q[2]->y
                                      && q[0]->z == q[2]->z)
          && (q[1]->x == q[3]->x && q[2]->y == q[3]->y && q[0]->z == q[3]->z)
          && (q[0]->x == q[4]->x && q[0]->y == q[4]->y
              && q[0]->z + inc == q[4]->z) && (q[1]->x == q[5]->x
                                               && q[1]->y == q[5]->y
                                               && q[4]->z == q[5]->z)
          && (q[2]->x == q[6]->x && q[2]->y == q[6]->y && q[4]->z == q[6]->z)
          && (q[3]->x == q[7]->x && q[3]->y == q[7]->y
              && q[4]->z == q[7]->z));
}

void
p8est_quadrant_edge_neighbor (const p4est_quadrant_t * q,
                              int edge, p4est_quadrant_t * r)
{
  const p4est_qcoord_t qh = P4EST_QUADRANT_LEN (q->level);

  P4EST_ASSERT (0 <= edge && edge < 12);
  P4EST_ASSERT (p4est_quadrant_is_valid (q));

  switch (edge / 4) {
  case 0:
    r->x = q->x;
    r->y = q->y + (2 * (edge & 0x01) - 1) * qh;
    r->z = q->z + ((edge & 0x02) - 1) * qh;
    break;
  case 1:
    r->x = q->x + (2 * (edge & 0x01) - 1) * qh;
    r->y = q->y;
    r->z = q->z + ((edge & 0x02) - 1) * qh;
    break;
  case 2:
    r->x = q->x + (2 * (edge & 0x01) - 1) * qh;
    r->y = q->y + ((edge & 0x02) - 1) * qh;
    r->z = q->z;
    break;
  default:
    SC_ABORT_NOT_REACHED ();
    break;
  }
  r->level = q->level;
  P4EST_ASSERT (p4est_quadrant_is_extended (r));
}

void
p8est_quadrant_edge_neighbor_extra (const p4est_quadrant_t * q, p4est_topidx_t
                                    t, int edge, sc_array_t * quads,
                                    sc_array_t * treeids,
                                    p4est_connectivity_t * conn)
{
  p4est_quadrant_t    temp;
  p4est_quadrant_t   *qp;
  p4est_topidx_t     *tp;
  int                 face;
  p8est_edge_info_t   ei;
  p8est_edge_transform_t *et;
  sc_array_t         *eta;
  size_t              etree;

  eta = &ei.edge_transforms;

  P4EST_ASSERT (SC_ARRAY_IS_OWNER (quads));
  P4EST_ASSERT (quads->elem_count == 0);
  P4EST_ASSERT (quads->elem_size == sizeof (p4est_quadrant_t));
  P4EST_ASSERT (SC_ARRAY_IS_OWNER (treeids));
  P4EST_ASSERT (treeids->elem_count == 0);
  P4EST_ASSERT (treeids->elem_size == sizeof (p4est_topidx_t));

  p8est_quadrant_edge_neighbor (q, edge, &temp);
  if (p4est_quadrant_is_inside_root (&temp)) {
    qp = p4est_quadrant_array_push (quads);
    *qp = temp;
    tp = (p4est_topidx_t *) sc_array_push (treeids);
    *tp = t;
    return;
  }

  if (!p8est_quadrant_is_outside_edge (&temp)) {
    qp = p4est_quadrant_array_push (quads);
    tp = (p4est_topidx_t *) sc_array_push (treeids);

    face = p8est_edge_faces[edge][0];
    p4est_quadrant_face_neighbor (q, face, &temp);
    if (p4est_quadrant_is_inside_root (&temp)) {
      face = p8est_edge_faces[edge][1];
      *tp = p8est_quadrant_face_neighbor_extra (&temp, t, face, qp, conn);
      if (*tp == -1) {
        qp = (p4est_quadrant_t *) sc_array_pop (quads);
        tp = (p4est_topidx_t *) sc_array_pop (treeids);
      }
      return;
    }
    face = p8est_edge_faces[edge][1];
    p4est_quadrant_face_neighbor (q, face, &temp);
    P4EST_ASSERT (p4est_quadrant_is_inside_root (&temp));
    face = p8est_edge_faces[edge][0];
    *tp = p8est_quadrant_face_neighbor_extra (&temp, t, face, qp, conn);
    if (*tp == -1) {
      qp = (p4est_quadrant_t *) sc_array_pop (quads);
      tp = (p4est_topidx_t *) sc_array_pop (treeids);
    }
    return;
  }
  sc_array_init (eta, sizeof (p8est_edge_transform_t));
  p8est_find_edge_transform (conn, t, edge, &ei);
  for (etree = 0; etree < eta->elem_count; etree++) {
    qp = p4est_quadrant_array_push (quads);
    tp = (p4est_topidx_t *) sc_array_push (treeids);
    et = p8est_edge_array_index (eta, etree);
    p8est_quadrant_transform_edge (&temp, qp, &ei, et, 1);
    *tp = et->ntree;
  }
  sc_array_reset (eta);
}

int
p8est_quadrant_touches_edge (const p4est_quadrant_t * q, int edge, int inside)
{
  int                 quad_contact[P4EST_FACES];
  int                 axis, side, incount;
  p4est_qcoord_t      lower, upper;

  P4EST_ASSERT (0 <= edge && edge < 12);

  axis = edge / 4;
  if (q->level == P4EST_MAXLEVEL) {
    P4EST_ASSERT (p4est_quadrant_is_node (q, inside));
    lower = 0;
    upper = P4EST_ROOT_LEN - (int) inside;
  }
  else {
    if (!inside) {
      P4EST_ASSERT (p4est_quadrant_is_extended (q));
      lower = -P4EST_QUADRANT_LEN (q->level);
      upper = P4EST_ROOT_LEN;
    }
    else {
      P4EST_ASSERT (p4est_quadrant_is_valid (q));
      lower = 0;
      upper = P4EST_LAST_OFFSET (q->level);
    }
  }
  quad_contact[0] = (q->x == lower);
  quad_contact[1] = (q->x == upper);
  quad_contact[2] = (q->y == lower);
  quad_contact[3] = (q->y == upper);
  quad_contact[4] = (q->z == lower);
  quad_contact[5] = (q->z == upper);

  incount = 0;
  if (axis != 0) {
    side = edge % 2;
    incount += quad_contact[side];
  }
  if (axis != 1) {
    side = (axis == 0) ? (edge % 2) : ((edge / 2) % 2);
    incount += quad_contact[2 + side];
  }
  if (axis != 2) {
    side = (edge / 2) % 2;
    incount += quad_contact[4 + side];
  }
#ifdef P4EST_DEBUG
  upper = P4EST_ROOT_LEN + (p4est_qcoord_t) (q->level == P4EST_MAXLEVEL
                                             && !inside);
  P4EST_ASSERT (axis != 0 || (q->x >= 0 && q->x < upper));
  P4EST_ASSERT (axis != 1 || (q->y >= 0 && q->y < upper));
  P4EST_ASSERT (axis != 2 || (q->z >= 0 && q->z < upper));
#endif

  return incount == 2;
}

void
p8est_quadrant_transform_edge (const p4est_quadrant_t * q,
                               p4est_quadrant_t * r,
                               const p8est_edge_info_t * ei,
                               const p8est_edge_transform_t * et, int inside)
{
  int                 iaxis;
  p4est_qcoord_t      mh, Rmh;
  p4est_qcoord_t      lshift, rshift;
  p4est_qcoord_t      my_xyz, *target_xyz[3];

  iaxis = (int) ei->iedge / 4;
  P4EST_ASSERT (0 <= et->naxis[0] && et->naxis[0] < 3);
  P4EST_ASSERT (0 <= et->naxis[1] && et->naxis[1] < 3);
  P4EST_ASSERT (0 <= et->naxis[2] && et->naxis[2] < 3);
  P4EST_ASSERT (et->naxis[0] != et->naxis[1] &&
                et->naxis[0] != et->naxis[2] && et->naxis[1] != et->naxis[2]);
  P4EST_ASSERT (0 <= et->nflip && et->nflip < 2);
  P4EST_ASSERT (q != r);

  if (q->level == P4EST_MAXLEVEL) {
    P4EST_ASSERT (!inside);
    P4EST_ASSERT (p8est_quadrant_touches_edge (q, (int) ei->iedge, inside));
    lshift = mh = 0;
    rshift = Rmh = P4EST_ROOT_LEN;
  }
  else {
    P4EST_ASSERT (p8est_quadrant_touches_edge (q, (int) ei->iedge, !inside));
    mh = -P4EST_QUADRANT_LEN (q->level);
    Rmh = P4EST_ROOT_LEN + mh;
    lshift = (inside ? 0 : mh);
    rshift = (inside ? Rmh : P4EST_ROOT_LEN);
  }
  target_xyz[0] = &r->x;
  target_xyz[1] = &r->y;
  target_xyz[2] = &r->z;

  /* transform coordinate axis parallel to edge */
  switch (iaxis) {
  case 0:
    my_xyz = q->x;
    break;
  case 1:
    my_xyz = q->y;
    break;
  case 2:
    my_xyz = q->z;
    break;
  default:
    SC_ABORT_NOT_REACHED ();
  }
  if (!et->nflip) {
    *target_xyz[et->naxis[0]] = my_xyz;
  }
  else {
    *target_xyz[et->naxis[0]] = Rmh - my_xyz;
  }

  /* create the other two coordinates */
  switch (et->corners) {
  case 0:
    *target_xyz[et->naxis[1]] = lshift;
    *target_xyz[et->naxis[2]] = lshift;
    break;
  case 1:
    *target_xyz[et->naxis[1]] = rshift;
    *target_xyz[et->naxis[2]] = lshift;
    break;
  case 2:
    *target_xyz[et->naxis[1]] = lshift;
    *target_xyz[et->naxis[2]] = rshift;
    break;
  case 3:
    *target_xyz[et->naxis[1]] = rshift;
    *target_xyz[et->naxis[2]] = rshift;
    break;
  default:
    SC_ABORT_NOT_REACHED ();
  }

  r->level = q->level;
  P4EST_ASSERT (p8est_quadrant_touches_edge (r, (int) et->nedge, inside));
}

void
p8est_quadrant_shift_edge (const p4est_quadrant_t * q,
                           p4est_quadrant_t * r, int edge)
{
  int                 outface;
  int                 cid, sid, step[P4EST_DIM];
  p4est_qcoord_t      th;
  p4est_quadrant_t    quad;
  /* *INDENT-OFF* */
  const int           contact[12] = {
    0x14, 0x18, 0x24, 0x28,
    0x11, 0x12, 0x21, 0x22,
    0x05, 0x06, 0x09, 0x0a
  };
  /* *INDENT-ON* */

  P4EST_ASSERT (q != r);
  P4EST_ASSERT (p4est_quadrant_is_valid (q));
  P4EST_ASSERT (edge >= 0 && edge < 12);

  P4EST_QUADRANT_INIT (&quad);

  quad = *q;
  for (;;) {
    th = P4EST_LAST_OFFSET (quad.level);
    cid = p4est_quadrant_child_id (&quad);
    switch (edge / 4) {
    case 0:
      sid = 2 * edge + (cid & 0x01);
      step[0] = 0;
      step[1] = 2 * (edge & 0x01) - 1;
      step[2] = (edge & 0x02) - 1;
      break;
    case 1:
      sid = 2 * (edge & 0x02) + (edge & 0x01) + (cid & 0x02);
      step[0] = 2 * (edge & 0x01) - 1;
      step[1] = 0;
      step[2] = (edge & 0x02) - 1;
      break;
    case 2:
      sid = edge - 8 + (cid & 0x04);
      step[0] = 2 * (edge & 0x01) - 1;
      step[1] = (edge & 0x02) - 1;
      step[2] = 0;
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
    p4est_quadrant_sibling (&quad, r, sid);
    P4EST_ASSERT (-1 <= step[0] && step[0] <= 1);
    P4EST_ASSERT (-1 <= step[1] && step[1] <= 1);
    P4EST_ASSERT (-1 <= step[2] && step[2] <= 1);

    outface = 0;
    if (step[0] != 0) {
      outface |= ((r->x <= 0) ? 0x01 : 0);
      outface |= ((r->x >= th) ? 0x02 : 0);
    }
    if (step[1] != 0) {
      outface |= ((r->y <= 0) ? 0x04 : 0);
      outface |= ((r->y >= th) ? 0x08 : 0);
    }
    if (step[2] != 0) {
      outface |= ((r->z <= 0) ? 0x10 : 0);
      outface |= ((r->z >= th) ? 0x20 : 0);
    }
    if (outface == contact[edge]) {
      break;
    }
    p4est_quadrant_parent (&quad, &quad);
    quad.x += (p4est_qcoord_t) step[0] * P4EST_QUADRANT_LEN (quad.level);
    quad.y += (p4est_qcoord_t) step[1] * P4EST_QUADRANT_LEN (quad.level);
    quad.z += (p4est_qcoord_t) step[2] * P4EST_QUADRANT_LEN (quad.level);
    P4EST_ASSERT (p4est_quadrant_is_extended (&quad));
  }

  if (step[0] != 0) {
    if (r->x < 0)
      r->x = 0;
    if (r->x >= P4EST_ROOT_LEN)
      r->x = th;
  }
  if (step[1] != 0) {
    if (r->y < 0)
      r->y = 0;
    if (r->y >= P4EST_ROOT_LEN)
      r->y = th;
  }
  if (step[2] != 0) {
    if (r->z < 0)
      r->z = 0;
    if (r->z >= P4EST_ROOT_LEN)
      r->z = th;
  }
  P4EST_ASSERT (p8est_quadrant_touches_edge (r, edge, 1));
}