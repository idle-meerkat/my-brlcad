/*                         O T R A N S L A T E . C
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
/** @file otranslate.c
 *
 * The otranslate command.
 *
 */

#include "common.h"
#include "bio.h"

#include "./ged_private.h"


int
ged_otranslate(struct ged *gedp, int argc, const char *argv[])
{
    register struct directory *dp;
    struct ged_trace_data gtd;
    struct rt_db_internal intern;
    vect_t delta;
    mat_t dmat;
    mat_t emat;
    mat_t tmpMat;
    mat_t invXform;
    point_t rpp_min;
    point_t rpp_max;
    static const char *usage = "obj dx dy dz";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 5) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (ged_get_obj_bounds2(gedp, 1, argv+1, &gtd, rpp_min, rpp_max) == BRLCAD_ERROR)
	return BRLCAD_ERROR;

    dp = gtd.gtd_obj[gtd.gtd_objpos-1];
    if (!(dp->d_flags & DIR_SOLID)) {
	if (ged_get_obj_bounds(gedp, 1, argv+1, 1, rpp_min, rpp_max) == BRLCAD_ERROR)
	    return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &delta[X]) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "%s: bad x value - %s", argv[0], argv[2]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[3], "%lf", &delta[Y]) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "%s: bad y value - %s", argv[0], argv[3]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[4], "%lf", &delta[Z]) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "%s: bad z value - %s", argv[0], argv[4]);
	return BRLCAD_ERROR;
    }

    MAT_IDN(dmat);
    VSCALE(delta, delta, gedp->ged_wdbp->dbip->dbi_local2base);
    MAT_DELTAS_VEC(dmat, delta);

    bn_mat_inv(invXform, gtd.gtd_xform);
    bn_mat_mul(tmpMat, invXform, dmat);
    bn_mat_mul(emat, tmpMat, gtd.gtd_xform);

    GED_DB_GET_INTERNAL(gedp, &intern, dp, emat, &rt_uniresource, BRLCAD_ERROR);
    RT_CK_DB_INTERNAL(&intern);
    GED_DB_PUT_INTERNAL(gedp, dp, &intern, &rt_uniresource, BRLCAD_ERROR);

#if 0
    /* notify observers */
    bu_observer_notify(interp, &gedp->wdb_observers, bu_vls_addr(&gedp->wdb_name));
#endif

    return BRLCAD_OK;
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