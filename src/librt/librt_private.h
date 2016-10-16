/*                 L I B R T _ P R I V A T E . H
 * BRL-CAD
 *
 * Copyright (c) 2011-2016 United States Government as represented by
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
/** @file librt_private.h
 *
 * These are declarations for functions that are for internal use by
 * LIBRT but are not public API.  NO CODE outside of LIBRT should use
 * these functions.
 *
 * Non-public functions should NOT have an rt_*() or db_*() prefix.
 * Consider using an _rt_, _db_, or other file-identifying prefix
 * accordingly (e.g., ell_*() for functions defined in primitives/ell)
 *
 */
#ifndef LIBRT_LIBRT_PRIVATE_H
#define LIBRT_LIBRT_PRIVATE_H

#include "common.h"

#include "rt/db4.h"
#include "raytrace.h"

/* approximation formula for the circumference of an ellipse */
#define ELL_CIRCUMFERENCE(a, b) M_PI * ((a) + (b)) * \
    (1.0 + (3.0 * ((((a) - b))/((a) + (b))) * ((((a) - b))/((a) + (b))))) \
    / (10.0 + sqrt(4.0 - 3.0 * ((((a) - b))/((a) + (b))) * ((((a) - b))/((a) + (b)))))

__BEGIN_DECLS

/* db_flip.c */

/**
 * function similar to ntohs() but always flips the bytes.
 * used for v4 compatibility.
 */
extern short flip_short(short s);

/**
 * function similar to ntohf() but always flips the types.
 * used for v4 compatibility.
 */
extern fastf_t flip_dbfloat(dbfloat_t d);

/**
 * function that flips a dbfloat_t[3] vector into fastf_t[3]
 */
extern void flip_fastf_float(fastf_t *ff, const dbfloat_t *fp, int n, int flip);

/**
 * function that flips a dbfloat_t[16] matrix into fastf_t[16]
 */
extern void flip_mat_dbmat(fastf_t *ff, const dbfloat_t *dbp, int flip);

/**
 * function that flips a fastf_t[16] matrix into dbfloat_t[16]
 */
extern void flip_dbmat_mat(dbfloat_t *dbp, const fastf_t *ff);


/**
 * return angle required for smallest side to fall within tolerances
 * for ellipse.  Smallest side is a side with an endpoint at (a, 0, 0)
 * where a is the semi-major axis.
 */
extern fastf_t ell_angle(fastf_t *p1, fastf_t a, fastf_t b, fastf_t dtol, fastf_t ntol);

/**
 * used by rt_shootray_bundle()
 * FIXME: non-public API shouldn't be using rt_ prefix
 */
extern const union cutter *rt_advance_to_next_cell(struct rt_shootray_status *ssp);

/**
 * used by rt_shootray_bundle()
 * FIXME: non-public API shouldn't be using rt_ prefix
 */
extern void rt_plot_cell(const union cutter *cutp, struct rt_shootray_status *ssp, struct bu_list *waiting_segs_hd, struct rt_i *rtip);


extern fastf_t primitive_get_absolute_tolerance(
	const struct rt_tess_tol *ttol,
	fastf_t rel_to_abs);

extern fastf_t primitive_diagonal_samples(
	struct rt_db_internal *ip,
	const struct rt_view_info *info);

extern int approximate_parabolic_curve(
	struct rt_pt_node *pts,
	fastf_t p,
	int num_new_points);

extern fastf_t primitive_curve_count(
	struct rt_db_internal *ip,
	const struct rt_view_info *info);

extern int approximate_hyperbolic_curve(
	struct rt_pt_node *pts,
	fastf_t a,
	fastf_t b,
	int num_new_points);

extern void
ellipse_point_at_radian(
	point_t result,
	const vect_t center,
	const vect_t axis_a,
	const vect_t axis_b,
	fastf_t radian);

extern void plot_ellipse(
	struct bu_list *vhead,
	const vect_t t,
	const vect_t a,
	const vect_t b,
	int num_points);



/* db_fullpath.c */

/**
 * Function to test whether a path has a cyclic entry in it.
 *
 * @param fp [i] Full path to test
 * @param name [i] String to use when checking path (optional).  If NULL, use the name of the current directory pointer in fp.
 * @return 1 if the path is cyclic, 0 if it is not.
 */
extern int cyclic_path(const struct db_full_path *fp, const char *name);


/* db_diff.c */

/**
 * Function to convert an ft_get list of parameters into an avs.
 * @return 0 if the conversion succeeds, -1 if it does not.
 */
extern int tcl_list_to_avs(const char *tcl_list, struct bu_attribute_value_set *avs, int offset);


/* primitive_util.c */

extern int _rt_tcl_list_to_int_array(const char *list, int **array, int *array_len);
extern int _rt_tcl_list_to_fastf_array(const char *list, fastf_t **array, int *array_len);

#ifdef USE_OPENCL
extern cl_device_id clt_get_cl_device(void);
extern cl_program clt_get_program(cl_context context, cl_device_id device, cl_uint count, const char *filename[], const char *options);


#define CLT_DECLARE_INTERFACE(name) \
    extern size_t clt_##name##_pack(struct bu_pool *pool, struct soltab *stp)

CLT_DECLARE_INTERFACE(tor);
CLT_DECLARE_INTERFACE(tgc);
CLT_DECLARE_INTERFACE(ell);
CLT_DECLARE_INTERFACE(arb);
CLT_DECLARE_INTERFACE(ars);
CLT_DECLARE_INTERFACE(rec);
CLT_DECLARE_INTERFACE(sph);
CLT_DECLARE_INTERFACE(part);
CLT_DECLARE_INTERFACE(epa);
CLT_DECLARE_INTERFACE(ehy);
CLT_DECLARE_INTERFACE(bot);
CLT_DECLARE_INTERFACE(eto);

extern size_t clt_bot_pack(struct bu_pool *pool, struct soltab *stp);
#endif

__END_DECLS

#endif /* LIBRT_LIBRT_PRIVATE_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
