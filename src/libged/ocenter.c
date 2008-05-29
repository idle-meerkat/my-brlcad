/*                         O C E N T E R . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
/** @file ocenter.c
 *
 * The ocenter command.
 *
 */

#include "ged.h"


int
ged_ocenter(struct rt_wdb *wdbp, int argc, const char *argv[])
{
    register struct directory *dp;
    struct wdb_trace_data wtd;
    struct rt_db_internal intern;
    mat_t dmat;
    mat_t emat;
    mat_t tmpMat;
    mat_t invXform;
    point_t rpp_min;
    point_t rpp_max;
    point_t oldCenter;
    point_t center;
    point_t delta;
    static const char *usage = "object [x y z]";

    GED_CHECK_DATABASE_OPEN(wdbp, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(&wdbp->wdb_result_str, 0);
    wdbp->wdb_result = GED_RESULT_NULL;
    wdbp->wdb_result_flags = 0;

    /* must be wanting help */
    if (argc == 1) {
	wdbp->wdb_result_flags |= GED_RESULT_FLAGS_HELP_BIT;
	bu_vls_printf(&wdbp->wdb_result_str, "Usage: %s %s", argv[0], usage);
	return GED_OK;
    }

    if (argc != 2 && argc != 5) {
	bu_vls_printf(&wdbp->wdb_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    /*
     * One of the get bounds routines needs to be fixed to
     * work with all cases. In the meantime...
     */
    if (ged_get_obj_bounds2(wdbp, 1, argv+1, &wtd, rpp_min, rpp_max) == GED_ERROR)
	return GED_ERROR;

    dp = wtd.wtd_obj[wtd.wtd_objpos-1];
    if (!(dp->d_flags & DIR_SOLID)) {
	if (ged_get_obj_bounds(wdbp, 1, argv+1, 1, rpp_min, rpp_max) == GED_ERROR)
	    return GED_ERROR;
    }

    VADD2(oldCenter, rpp_min, rpp_max);
    VSCALE(oldCenter, oldCenter, 0.5);

    if (argc == 2) {
	VSCALE(center, oldCenter, wdbp->dbip->dbi_base2local);
	bn_encode_vect(&wdbp->wdb_result_str, center);

	return GED_OK;
    }

    GED_CHECK_READ_ONLY(wdbp, GED_ERROR);

    /* Read in the new center */
    if (sscanf(argv[2], "%lf", &center[X]) != 1) {
	bu_vls_printf(&wdbp->wdb_result_str, "%s: bad x value - %s", argv[0], argv[2]);
	return GED_ERROR;
    }

    if (sscanf(argv[3], "%lf", &center[Y]) != 1) {
	bu_vls_printf(&wdbp->wdb_result_str, "%s: bad y value - %s", argv[0], argv[3]);
	return GED_ERROR;
    }

    if (sscanf(argv[4], "%lf", &center[Z]) != 1) {
	bu_vls_printf(&wdbp->wdb_result_str, "%s: bad z value - %s", argv[0], argv[4]);
	return GED_ERROR;
    }

    VSCALE(center, center, wdbp->dbip->dbi_local2base);
    VSUB2(delta, center, oldCenter);
    MAT_IDN(dmat);
    MAT_DELTAS_VEC(dmat, delta);

    bn_mat_inv(invXform, wtd.wtd_xform);
    bn_mat_mul(tmpMat, invXform, dmat);
    bn_mat_mul(emat, tmpMat, wtd.wtd_xform);

    GED_DB_GET_INTERNAL(wdbp, &intern, dp, emat, &rt_uniresource, GED_ERROR);
    RT_CK_DB_INTERNAL(&intern);
    GED_DB_PUT_INTERNAL(wdbp, dp, &intern, &rt_uniresource, GED_ERROR);

#if 0
    /* notify observers */
    bu_observer_notify(interp, &wdbp->wdb_observers, bu_vls_addr(&wdbp->wdb_name));
#endif

    return GED_OK;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
