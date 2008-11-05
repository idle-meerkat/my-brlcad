/*                         M O V E . C
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
/** @file move.c
 *
 * The move command.
 *
 */

#include "common.h"

#include <string.h>

#include "bio.h"
#include "cmd.h"
#include "ged_private.h"

int
ged_move(struct ged *gedp, int argc, const char *argv[])
{
    register struct directory	*dp;
    struct rt_db_internal		intern;
    static const char *usage = "from to";

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

    if (argc != 3) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if ((dp = db_lookup(gedp->ged_wdbp->dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
	return BRLCAD_ERROR;

    if (db_lookup(gedp->ged_wdbp->dbip, argv[2], LOOKUP_QUIET) != DIR_NULL) {
	bu_vls_printf(&gedp->ged_result_str, "%s: already exists", argv[2]);
	return BRLCAD_ERROR;
    }

    if (rt_db_get_internal(&intern, dp, gedp->ged_wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
	bu_vls_printf(&gedp->ged_result_str, "Database read error, aborting");
	return BRLCAD_ERROR;
    }

    /*  Change object name in the in-memory directory. */
    if (db_rename(gedp->ged_wdbp->dbip, dp, argv[2]) < 0) {
	rt_db_free_internal(&intern, &rt_uniresource);
	bu_vls_printf(&gedp->ged_result_str, "error in db_rename to %s, aborting", argv[2]);
	return BRLCAD_ERROR;
    }

    /* Re-write to the database.  New name is applied on the way out. */
    if (rt_db_put_internal(dp, gedp->ged_wdbp->dbip, &intern, &rt_uniresource) < 0) {
	bu_vls_printf(&gedp->ged_result_str, "Database write error, aborting");
	return BRLCAD_ERROR;
    }

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