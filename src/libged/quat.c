/*                         Q U A T . C
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
/** @file quat.c
 *
 * The quat command.
 *
 */

#include "common.h"
#include "bio.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ged_private.h"

int
ged_quat(struct ged *gedp, int argc, const char *argv[])
{
    quat_t quat;
    static const char *usage = "a b c d";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_VIEW(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* return Viewrot as a quaternion */
    if (argc == 1) {
	quat_mat2quat(quat, gedp->ged_gvp->gv_rotation);
	bu_vls_printf(&gedp->ged_result_str, "%.12g %.12g %.12g %.12g", V4ARGS(quat));
	return BRLCAD_OK;
    }

    if (argc != 6) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    /* Set the view orientation given a quaternion */
    if (sscanf(argv[2], "%lf", quat) != 1 ||
	sscanf(argv[3], "%lf", quat+1) != 1 ||
	sscanf(argv[4], "%lf", quat+2) != 1 ||
	sscanf(argv[5], "%lf", quat+3) != 1) {

	bu_vls_printf(&gedp->ged_result_str, "%s quat: bad value detected - %s %s %s %s",
		      argv[0], argv[2], argv[3], argv[4], argv[5]);
	return BRLCAD_ERROR;
    }

    quat_quat2mat(gedp->ged_gvp->gv_rotation, quat);
    ged_view_update(gedp->ged_gvp);

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
