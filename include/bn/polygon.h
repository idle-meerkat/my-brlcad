/*                        P O L Y G O N . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/*----------------------------------------------------------------------*/
/* @file polygon.h */
/** @addtogroup poly */
/** @{ */

/**
 *  @brief Functions for working with polygons
 */

#ifndef BN_POLYGON_H
#define BN_POLYGON_H

#include "common.h"
#include "vmath.h"
#include "bn/defines.h"
#include "bn/tol.h"

__BEGIN_DECLS

/*********************************************************
  Operations on 2D point types
 *********************************************************/

/**
 * @brief
 * test whether a polygon is clockwise (CW) or counter clockwise (CCW)
 *
 * Determine if a set of points forming a polygon are in clockwise
 * or counter-clockwise order (see http://stackoverflow.com/a/1165943)
 *
 * @param[in] npts number of points pts contains
 * @param[in] pts array of points, building a convex polygon. duplicated points
 * aren't allowed. the points in the array will be sorted counter-clockwise.
 *
 * @return -1 if polygon is counter-clockwise
 * @return 1 if polygon is clockwise
 * @return 0 if the test failed
 */
BN_EXPORT extern int bn_polygon_clockwise(size_t npts, const point2d_t *pts);


/**
 * @brief
 * test whether a point is inside a 2d polygon
 *
 * franklin's test for point inclusion within a polygon - see
 * http://www.ecse.rpi.edu/homepages/wrf/research/short_notes/pnpoly.html
 * for more details and the implementation file polygon.c for license info.
 *
 * @param[in] npts number of points pts contains
 * @param[in] pts array of points, building a convex polygon. duplicated points
 * aren't allowed. the points in the array will be sorted counter-clockwise.
 * @param[in] test_pt point to test.
 *
 * @return 0 if point is outside polygon
 * @return 1 if point is inside polygon
 */
BN_EXPORT extern int bn_pt_in_polygon(size_t npts, const point2d_t *pts, const point2d_t *test_pt);

/**
 * @brief
 * Triangulate a 2D polygon.
 *
 * This routine generates a triangulation of the input polygon using the method
 * documented in David Eberly's Triangulation by Ear Clipping, section 2:
 * http://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
 *
 * The primary input polygon cannot have holes and must be provided as an array of
 * counter-clockwise 2D points.  If interior "hole" polygons are present, they must
 * be passed in via the holes_array and be ordered clockwise.
 *
 * If no holes are present, caller should pass NULL for holes_array and holes_npts,
 * and 0 for nholes.
 *
 * No points are added as part of this triangulation process - the result uses
 * only those points in the original polygon, and hence only the face
 * information is created as output.
 *
 * @param[out] faces Set of faces in the triangulation, stored as integer indices to the pts.  The first three indices are the vertices of the first face, the second three define the second face, and so forth.
 * @param[out] num_faces Number of faces created
 * @param[in] npts Number of points pts contains
 * @param[in] pts Array of points defining a polygon. Duplicated points
 * aren't allowed. The points in the array will be sorted counter-clockwise.
 *
 * @return 0 if triangulation is successful
 * @return 1 if triangulation is unsuccessful
 */
BN_EXPORT extern int bn_polygon_triangulate(int **faces, int *num_faces, const point2d_t *pts, size_t npts,
	const point2d_t **holes_array, const size_t *holes_npts, const size_t nholes);


/*********************************************************
  Operations on 3D point types - these are assumed to be
  polygons embedded in 3D planes in space
 *********************************************************/

/**
 * @brief
 * Calculate the interior area of a polygon in a 3D plane in space.
 *
 * If npts > 4, Greens Theorem is used. The polygon mustn't
 * be self-intersecting.
 *
 * @param[out] area The interior area of the polygon
 * @param[in] npts Number of point_ts, stored in pts
 * @param[in] pts All points of the polygon, sorted counter-clockwise.
 * The array mustn't contain duplicated or non-coplanar points.
 *
 * @return 0 if calculation was successful
 * @return 1 if calculation failed, e.g. because one parameter is a NULL-pointer
 */
BN_EXPORT extern int bn_3d_polygon_area(fastf_t *area, size_t npts, const point_t *pts);


/**
 * @brief
 * Calculate the centroid of a non self-intersecting polygon in a 3D plane in space.
 *
 * @param[out] cent The centroid of the polygon
 * @param[in] npts Number of point_ts, stored in pts
 * @param[in] pts all points of the polygon, sorted counter-clockwise.
 * The array mustn't contain duplicated points or non-coplanar points.
 *
 * @return 0 if calculation was successful
 * @return 1 if calculation failed, e.g. because one in-parameter is a NULL-pointer
 */
BN_EXPORT extern int bn_3d_polygon_centroid(point_t *cent, size_t npts, const point_t *pts);


/**
 * @brief
 * Sort an array of point_ts, building a convex polygon, counter-clockwise
 *
 *@param[in] npts Number of points, pts contains
 *@param pts Array of point_ts, building a convex polygon. Duplicated points
 *aren't allowed. The points in the array will be sorted counter-clockwise.
 *@param[in] cmp Plane equation of the polygon
 *
 *@return 0 if calculation was successful
 *@return 1 if calculation failed, e.g. because pts is a NULL-pointer
 */
BN_EXPORT extern int bn_3d_polygon_sort_ccw(size_t npts, point_t *pts, plane_t cmp);


/**
 * @brief
 * Calculate for an array of plane_eqs, which build a polyhedron, the
 * point_t's for each face.
 *
 * @param[out] npts Array, which stores for every face the number of
 * point_ts, added to pts. Needs to be allocated with npts[neqs] already.
 * @param[out] pts 2D-array which stores the point_ts for every
 * face. The array needs to be allocated with pts[neqs][neqs-1] already.
 * @param[in] neqs Number of plane_ts, stored in eqs
 * @param[in] eqs Array, that contains the plane equations, which
 * build the polyhedron
 *
 * @return 0 if calculation was successful
 * @return 1 if calculation failed, e.g. because one parameter is a NULL-Pointer
 */
BN_EXPORT extern int bn_3d_polygon_mk_pts_planes(size_t *npts, point_t **pts, size_t neqs, const plane_t *eqs);



__END_DECLS

#endif  /* BN_POLYGON_H */
/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
