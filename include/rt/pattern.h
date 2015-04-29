/*                      R T _ P A T T E R N . H
 * BRL-CAD
 *
 * Copyright (c) 1993-2015 United States Government as represented by
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
/** @addtogroup librt */
/** @{ */
/** @file rt/pattern.h
 *
 * Functionality for generating pattenrs of rays.
 *
 */

#ifndef RT_PATTERN_H
#define RT_PATTERN_H

#include "common.h"

#include "rt/defines.h"
#include "rt/xray.h"


__BEGIN_DECLS

/**
 * Initial set of 'xrays' pattern generators that can
 * used to feed a bundle set of rays to rt_shootrays()
 */

/**
 * Make a bundle of rays around a main ray using a generator
 *
 * If data is NULL, return -1
 *
 * If the data in the rt_pattern_data struct does not meet the requirements
 * of the specified pattern, return -2
 *
 * If data->rays is NULL, return the number of rays that would have
 * been generated.
 *
 * If data->rays is not NULL and ray_cnt does not match the number of rays
 * that will be generated, return -3.
 *
 * If data->rays is not NULL and ray_cnt matches the number of rays
 * that will be generated, assign rays to the rays output.
 *
 *
 * Pattern data for each pattern type:
 *
 *   * all lengths are in mm;
 *   * center_ray.r_dir must have unit length, if
 *   * parameter data arrays are necessary they are the responsibility of
 *     the calling function
 *
 *   RT_RECT_GRID:
 *      Make a bundle of orthogonal rays around a center ray as a uniform
 *      rectangular grid.  Grid extents are from -a_vec to a_vec and -b_vec
 *      to b_vec.
 *      Params:
 *         center_ray
 *         a_vec       Direction for up
 *         b_vec       Direction for right
 *         p1          offset between rays in the a direction
 *         p2          offset between rays in the b direction
 *   RT_FRUSTUM:
 *      Make a bundle of rays around a main ray in the shape of a frustum
 *      as a uniform rectangular grid.
 *      Params:
 *         center_ray
 *         a_vec       Direction for up
 *         b_vec       Direction for right
 *         p_n = 4     Number of parameters
 *         n_p[0]      angle of divergence in the direction of a_vec
 *         n_p[1]      angle of divergence in the direction of b_vec
 *         n_p[2]      a_num
 *         n_p[3]      b_num
 *
 *   RT_CIRCULAR_GRID:
 *      Make a bundle of rays around a main ray using a uniform rectangular
 *      grid pattern with a circular extent.
 *      Params:
 *         center_ray
 *         a_vec       Direction for up
 *         p1          grid size
 *         p2          Radius
 *
 *   RT_CONIC:
 *      Make a bundle of rays around a main ray in the shape of a cone,
 *      using a uniform rectangular grid.
 *      Params:
 *         center_ray
 *         a_vec       Direction for up
 *         p1          angle of divergence of the cone
 *         p2          number of rays that line on each radial ring of the cone
 *
 *   RT_ELLIPTICAL_GRID:
 *      Make a bundle of rays around a main ray using a uniform rectangular
 *      grid pattern with an elliptical extent.
 *      Params:
 *         center_ray
 *         a_vec       Direction for up
 *         b_vec       Direction for right
 *         p1          grid size
 *
 *
 * return negative on error (data will be unmodified in error condition)
 *        ray count on success (>=0)
 *
 *
 * If ray count greater than zero, data->rays array will hold rays generated by pattern
 *
 *
 * The following is an example:
@code
int ray_cnt = 0;
struct rt_pattern_data data = RT_PATTERN_DATA_INIT;
VSET(data.a_vec, 0, 0, 1);
data.p1 = 0.1
data.p2 = 10
ray_cnt = rt_pattern(&data, RT_CIRCULAR_GRID);
if (ray_cnt < 0) {
    bu_log("error");
} else {
    do_something_with_rays(data.rays);
    bu_free(data.rays);
}
@endcode
 */

 /* RT_PATTERN_<SHAPE>_<SAMPLE> */
typedef enum {
    RT_PATTERN_CIRC_LAYERS,
    RT_PATTERN_CIRC_ORTHOGRID,
    RT_PATTERN_CIRC_PERSPGRID,
    RT_PATTERN_CIRC_SPIRAL,
    RT_PATTERN_ELLIPSE_ORTHOGRID,
    RT_PATTERN_ELLIPSE_PERSPGRID,
    RT_PATTERN_RECT_ORTHOGRID,
    RT_PATTERN_RECT_PERSPGRID,
    RT_PATTERN_SPH_LAYERS,
    RT_PATTERN_SPH_QRAND
} rt_pattern_t;

struct rt_pattern_data {
    /* output - MUST DO this should really be an array of fastf_t numbers... */
    fastf_t *rays;
    /* inputs */
    size_t ray_cnt;
    point_t center_pt;
    vect_t center_dir;
    size_t vn;
    vect_t *n_vec;
    size_t pn;
    fastf_t *n_p;
};
#define RT_PATTERN_DATA_INIT {NULL, 0, {0, 0, 0}, {0, 0, 0}, 0, NULL, 0, NULL}

RT_EXPORT extern int rt_pattern(struct rt_pattern_data *data, rt_pattern_t type);

/**
 * Make a bundle of rays around a main ray using a uniform rectangular
 * grid pattern with an elliptical extent.
 *
 * avec and bvec a.  The gridsize is
 * given in mm.
 *
 * rp[0].r_dir must have unit length.
 */
RT_EXPORT extern int rt_gen_elliptical_grid(struct xrays *rays,
					    const struct xray *center_ray,
					    const fastf_t *avec,
					    const fastf_t *bvec,
					    fastf_t gridsize);

/**
 * Make a bundle of rays around a main ray using a uniform rectangular
 * grid pattern with a circular extent.  The radius, gridsize is given
 * in mm.
 *
 * rp[0].r_dir must have unit length.
 */
RT_EXPORT extern int rt_gen_circular_grid(struct xrays *ray_bundle,
					  const struct xray *center_ray,
					  fastf_t radius,
					  const fastf_t *up_vector,
					  fastf_t gridsize);

/**
 * Make a bundle of rays around a main ray in the shape of a cone,
 * using a uniform rectangular grid; theta is the angle of divergence
 * of the cone, and rays_per_radius is the number of rays that lie on
 * any given radius of the cone.
 *
 * center_ray.r_dir must have unit length.
 */
RT_EXPORT extern int rt_gen_conic(struct xrays *rays,
				  const struct xray *center_ray,
				  fastf_t theta,
				  vect_t up_vector,
				  int rays_per_radius);

/**
 * Make a bundle of rays around a main ray in the shape of a frustum
 * as a uniform rectangular grid.  a_vec and b_vec are the directions
 * for up and right, respectively; a_theta and b_theta are the angles
 * of divergence in the directions of a_vec and b_vec respectively.
 * This is useful for creating a grid of rays for perspective
 * rendering.
 */
RT_EXPORT extern int rt_gen_frustum(struct xrays *rays,
				    const struct xray *center_ray,
				    const vect_t a_vec,
				    const vect_t b_vec,
				    const fastf_t a_theta,
				    const fastf_t b_theta,
				    const fastf_t a_num,
				    const fastf_t b_num);

/**
 * Make a bundle of orthogonal rays around a center ray as a uniform
 * rectangular grid.  a_vec and b_vec are the directions for up and
 * right, respectively; their magnitudes determine the extent of the
 * grid (the grid extends from -a_vec to a_vec in the up-direction and
 * from -b_vec to b_vec in the right direction).  da and db are the
 * offset between rays in the a and b directions respectively.
 */
RT_EXPORT extern int rt_gen_rect(struct xrays *rays,
				 const struct xray *center_ray,
				 const vect_t a_vec,
				 const vect_t b_vec,
				 const fastf_t da,
				 const fastf_t db);

__END_DECLS

#endif /* RT_PATTERN_H */
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
