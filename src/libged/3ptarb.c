/*                         3 P T A R B . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2009 United States Government as represented by
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
/** @file 3ptarb.c
 *
 * The 3ptarb command.
 *
 */

#include "common.h"
#include "bio.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "rtgeom.h"
#include "ged_private.h"

static char *p_arb3pt[] = {
    "Enter X, Y, Z for point 1: ",
    "Enter Y, Z: ",
    "Enter Z: ",
    "Enter X, Y, Z for point 2: ",
    "Enter Y, Z: ",
    "Enter Z: ",
    "Enter X, Y, Z for point 3: ",
    "Enter Y, Z: ",
    "Enter Z: "
};

int
ged_3ptarb(struct ged *gedp, int argc, const char *argv[])
{
    int			i, solve;
    vect_t			vec1;
    vect_t			vec2;
    fastf_t			pt4[2], length, thick;
    vect_t			norm;
    fastf_t			ndotv;
    char			**prompts;
    struct directory	*dp;
    struct rt_db_internal	internal;
    struct rt_arb_internal	*aip;
    static const char *usage = "name x1 y1 z1 x2 y2 z2 x3 y3 z3 coord c1 c2 th";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

#if 0
    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }
#endif

    if (argc > 15) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (argc < 2) {
	bu_vls_printf(&gedp->ged_result_str, "Enter name for this arb: ");
	return BRLCAD_MORE_ARGS;
    }

    if ( db_lookup( gedp->ged_wdbp->dbip, argv[1], LOOKUP_QUIET) != DIR_NULL ) {
	bu_vls_printf(&gedp->ged_result_str, "%s: %s already exists\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    /* read the three points */
    prompts = &p_arb3pt[0];
    if (argc < 11) {
	bu_vls_printf(&gedp->ged_result_str, prompts[argc-2]);
	return BRLCAD_MORE_ARGS;
    }

    /* preliminary calculations to check input so far */
    for (i=0; i<3; i++) {
	vec1[i] = atof(argv[i+2]) - atof(argv[i+5]);
	vec2[i] = atof(argv[i+2]) - atof(argv[i+8]);
    }
    VCROSS(norm, vec1, vec2);
    length = MAGNITUDE( norm );
    if (length == 0.0) {
	bu_vls_printf(&gedp->ged_result_str, "%s: points are colinear\n", argv[0]);
	return BRLCAD_ERROR;
    }
    VSCALE(norm, norm, 1.0/length);

    if (argc < 12) {
	bu_vls_printf(&gedp->ged_result_str, "Enter coordinate to solve for (x, y, or z): ");
	return BRLCAD_MORE_ARGS;
    }

    switch (argv[11][0]) {
	case 'x':
	    if (norm[0] == 0.0) {
		bu_vls_printf(&gedp->ged_result_str, "%s: X not unique in this face\n", argv[0]);
		return BRLCAD_ERROR;
	    }
	    solve = X;

	    if (argc < 13) {
		bu_vls_printf(&gedp->ged_result_str, "Enter the Y, Z coordinate values: ");
		return BRLCAD_MORE_ARGS;
	    }
	    if (argc < 14) {
		bu_vls_printf(&gedp->ged_result_str, "Enter the Z coordinate value: ");
		return BRLCAD_MORE_ARGS;
	    }

	    pt4[0] = atof( argv[12] ) * gedp->ged_wdbp->dbip->dbi_local2base;
	    pt4[1] = atof( argv[13] ) * gedp->ged_wdbp->dbip->dbi_local2base;
	    break;

	case 'y':
	    if (norm[1] == 0.0) {
		bu_vls_printf(&gedp->ged_result_str, "%s: Y not unique in this face\n", argv[0]);
		return BRLCAD_ERROR;
	    }
	    solve = Y;

	    if (argc < 13) {
		bu_vls_printf(&gedp->ged_result_str, "Enter the X, Z coordinate values: ");
		return BRLCAD_MORE_ARGS;
	    }
	    if (argc < 14) {
		bu_vls_printf(&gedp->ged_result_str, "Enter the Z coordinate value: ");
		return BRLCAD_MORE_ARGS;
	    }

	    pt4[0] = atof( argv[12] ) * gedp->ged_wdbp->dbip->dbi_local2base;
	    pt4[1] = atof( argv[13] ) * gedp->ged_wdbp->dbip->dbi_local2base;
	    break;

	case 'z':
	    if (norm[2] == 0.0) {
		bu_vls_printf(&gedp->ged_result_str, "%s: Z not unique in this face\n", argv[0]);
		return BRLCAD_ERROR;
	    }
	    solve = Z;

	    if (argc < 13) {
		bu_vls_printf(&gedp->ged_result_str, "Enter the X, Y coordinate values: ");
		return BRLCAD_MORE_ARGS;
	    }
	    if (argc < 14) {
		bu_vls_printf(&gedp->ged_result_str, "Enter the Y coordinate value: ");
		return BRLCAD_MORE_ARGS;
	    }

	    pt4[0] = atof( argv[12] ) * gedp->ged_wdbp->dbip->dbi_local2base;
	    pt4[1] = atof( argv[13] ) * gedp->ged_wdbp->dbip->dbi_local2base;
	    break;

	default:
	    bu_vls_printf(&gedp->ged_result_str, "%s: coordinate must be x, y, or z\n", argv[0]);
	    return BRLCAD_ERROR;
    }

    if (argc < 15) {
	bu_vls_printf(&gedp->ged_result_str, "Enter thickness for this arb: ");
	return BRLCAD_MORE_ARGS;
    }

    if ((thick = (atof( argv[14] ))) == 0.0) {
	bu_vls_printf(&gedp->ged_result_str, "%s: thickness = 0.0\n", argv[0]);
	return BRLCAD_ERROR;
    }

    RT_INIT_DB_INTERNAL( &internal );
    internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
    internal.idb_type = ID_ARB8;
    internal.idb_meth = &rt_functab[ID_ARB8];
    internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_arb_internal), "rt_arb_internal" );
    aip = (struct rt_arb_internal *)internal.idb_ptr;
    aip->magic = RT_ARB_INTERNAL_MAGIC;

    for (i=0; i<8; i++) {
	VSET( aip->pt[i], 0.0, 0.0, 0.0 );
    }

    for (i=0; i<3; i++) {
	/* the three given vertices */
	VSET( aip->pt[i], atof( argv[i*3+2] )*gedp->ged_wdbp->dbip->dbi_local2base, atof( argv[i*3+3] )*gedp->ged_wdbp->dbip->dbi_local2base, atof( argv[i*3+4] )*gedp->ged_wdbp->dbip->dbi_local2base );
    }

    thick *= gedp->ged_wdbp->dbip->dbi_local2base;

    ndotv = VDOT( aip->pt[0], norm );

    switch ( solve ) {

	case X:
	    /* solve for x-coord of 4th point */
	    aip->pt[3][Y] = pt4[0];		/* y-coord */
	    aip->pt[3][Z] = pt4[1]; 	/* z-coord */
	    aip->pt[3][X] =  ( ndotv
			       - norm[Y] * aip->pt[3][Y]
			       - norm[Z] * aip->pt[3][Z])
		/ norm[X];	/* x-coord */
	    break;

	case Y:
	    /* solve for y-coord of 4th point */
	    aip->pt[3][X] = pt4[0];		/* x-coord */
	    aip->pt[3][Z] = pt4[1]; 	/* z-coord */
	    aip->pt[3][Y] =  ( ndotv
			       - norm[X] * aip->pt[3][X]
			       - norm[Z] * aip->pt[3][Z])
		/ norm[Y];	/* y-coord */
	    break;

	case Z:
	    /* solve for z-coord of 4th point */
	    aip->pt[3][X] = pt4[0];		/* x-coord */
	    aip->pt[3][Y] = pt4[1]; 	/* y-coord */
	    aip->pt[3][Z] =  ( ndotv
			       - norm[X] * aip->pt[3][X]
			       - norm[Y] * aip->pt[3][Y])
		/ norm[Z];	/* z-coord */
	    break;

	default:
	    bu_vls_printf(&gedp->ged_result_str, "%s: bad coordinate to solve for\n", argv[0]);
	    return BRLCAD_ERROR;
    }

    /* calculate the remaining 4 vertices */
    for (i=0; i<4; i++) {
	VJOIN1( aip->pt[i+4], aip->pt[i], thick, norm );
    }

    if ( (dp = db_diradd( gedp->ged_wdbp->dbip, argv[1], -1L, 0, DIR_SOLID, (genptr_t)&internal.idb_type)) == DIR_NULL )
    {
	bu_vls_printf(&gedp->ged_result_str, "%s: Cannot add %s to the directory\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    if ( rt_db_put_internal( dp, gedp->ged_wdbp->dbip, &internal, &rt_uniresource ) < 0 )
    {
	rt_db_free_internal( &internal, &rt_uniresource );
	bu_vls_printf(&gedp->ged_result_str, "%s: Database write error, aborting\n", argv[0]);
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
