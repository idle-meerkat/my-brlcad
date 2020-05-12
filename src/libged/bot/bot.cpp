/*                         B O T . C P P
 * BRL-CAD
 *
 * Copyright (c) 2020 United States Government as represented by
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
/** @file libged/bot/bot.cpp
 *
 * The LIBGED bot command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "bu/cmd.h"
#include "bu/color.h"
#include "bu/opt.h"
#include "bg/chull.h"
#include "bg/trimesh.h"
#include "rt/geom.h"
#include "wdb.h"

#include "./ged_bot.h"

int
_bot_obj_setup(struct _ged_bot_info *gb, const char *name)
{
    gb->dp = db_lookup(gb->gedp->ged_wdbp->dbip, name, LOOKUP_NOISY);
    if (gb->dp == RT_DIR_NULL) {
	bu_vls_printf(gb->gedp->ged_result_str, ": %s is not a solid or does not exist in database", name);
	return GED_ERROR;
    } else {
	int real_flag = (gb->dp->d_addr == RT_DIR_PHONY_ADDR) ? 0 : 1;
	if (!real_flag) {
	    /* solid doesn't exist */
	    bu_vls_printf(gb->gedp->ged_result_str, ": %s is not a real solid", name);
	    return GED_ERROR;
	}
    }

    gb->solid_name = std::string(name);

    BU_GET(gb->intern, struct rt_db_internal);

    GED_DB_GET_INTERNAL(gb->gedp, gb->intern, gb->dp, bn_mat_identity, &rt_uniresource, GED_ERROR);
    RT_CK_DB_INTERNAL(gb->intern);

    if (gb->intern->idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_vls_printf(gb->gedp->ged_result_str, ": object %s is not of type bot\n", gb->solid_name.c_str());
	return GED_ERROR;
    }

    return GED_OK;
}


int
_bot_cmd_msgs(void *bs, int argc, const char **argv, const char *us, const char *ps)
{
    struct _ged_bot_info *gb = (struct _ged_bot_info *)bs;
    if (argc == 2 && BU_STR_EQUAL(argv[1], HELPFLAG)) {
	bu_vls_printf(gb->gedp->ged_result_str, "%s\n%s\n", us, ps);
	return 1;
    }
    if (argc == 2 && BU_STR_EQUAL(argv[1], PURPOSEFLAG)) {
	bu_vls_printf(gb->gedp->ged_result_str, "%s\n", ps);
	return 1;
    }
    return 0;
}

extern "C" int
_bot_cmd_get(void *bs, int argc, const char **argv)
{
    const char *usage_string = "bot get <faces|minEdge|maxEdge|orientation|type|vertices> <objname>";
    const char *purpose_string = "Report specific information about a BoT shape";
    if (_bot_cmd_msgs(bs, argc, argv, usage_string, purpose_string)) {
	return GED_OK;
    }

    struct _ged_bot_info *gb = (struct _ged_bot_info *)bs;

    argc--; argv++;

    if (argc != 2) {
	bu_vls_printf(gb->gedp->ged_result_str, "%s", usage_string);
	return GED_ERROR;
    }

    if (_bot_obj_setup(gb, argv[1]) == GED_ERROR) {
	return GED_ERROR;
    }

    struct rt_bot_internal *bot = (struct rt_bot_internal *)(gb->intern->idb_ptr);

    fastf_t propVal = rt_bot_propget(bot, argv[0]);

    /* print result string */
    if (!EQUAL(propVal, -1.0)) {

	fastf_t tmp = (int) propVal;

	if (EQUAL(propVal, tmp)) {
	    /* int result */
	    bu_vls_printf(gb->gedp->ged_result_str, "%d", (int) propVal);
	} else {
	    /* float result */
	    bu_vls_printf(gb->gedp->ged_result_str, "%f", propVal);
	}
    } else {
	bu_vls_printf(gb->gedp->ged_result_str, "%s is not a valid argument!", argv[1]);
	return GED_ERROR;
    }

    return GED_OK;
}

extern "C" int
_bot_cmd_chull(void *bs, int argc, const char **argv)
{
    const char *usage_string = "bot [options] chull <objname> [output_bot]";
    const char *purpose_string = "Generate the BoT's convex hull and store it in an object";
    if (_bot_cmd_msgs(bs, argc, argv, usage_string, purpose_string)) {
	return GED_OK;
    }

    argc--; argv++;

    struct _ged_bot_info *gb = (struct _ged_bot_info *)bs;
   
    if (!argc) {
	bu_vls_printf(gb->gedp->ged_result_str, "%s\n%s\n", usage_string, purpose_string);
	return GED_ERROR;
    }


    if (_bot_obj_setup(gb, argv[0]) == GED_ERROR) {
	return GED_ERROR;
    }

    struct rt_bot_internal *bot = (struct rt_bot_internal *)(gb->intern->idb_ptr);
    int retval = 0;
    int fc = 0;
    int vc = 0;
    point_t *vert_array;
    int *faces;
    unsigned char err = 0;

    retval = bg_3d_chull(&faces, &fc, &vert_array, &vc, (const point_t *)bot->vertices, (int)bot->num_vertices);

    if (retval != 3) {
	return GED_ERROR;
    }

    struct bu_vls out_name = BU_VLS_INIT_ZERO;
    if (argc > 1) {
        bu_vls_sprintf(&out_name, "%s", argv[1]);
    } else {
        bu_vls_sprintf(&out_name, "%s.hull", gb->dp->d_namep);
    }

    if (db_lookup(gb->gedp->ged_wdbp->dbip, bu_vls_cstr(&out_name), LOOKUP_QUIET) != RT_DIR_NULL) {
        bu_vls_printf(gb->gedp->ged_result_str, "Object %s already exists!\n", bu_vls_cstr(&out_name));
        bu_vls_free(&out_name);
        return GED_ERROR;
    }

    retval = mk_bot(gb->gedp->ged_wdbp, bu_vls_cstr(&out_name), RT_BOT_SOLID, RT_BOT_CCW, err, vc, fc, (fastf_t *)vert_array, faces, NULL, NULL);

    bu_vls_free(&out_name);
    bu_free(faces, "free faces");
    bu_free(vert_array, "free verts");

    if (retval) {
	return GED_ERROR;
    }

    return GED_OK;
}

extern "C" int
_bot_cmd_isect(void *bs, int argc, const char **argv)
{
    const char *usage_string = "bot [options] isect <objname> <objname2>";
    const char *purpose_string = "(TODO) Test if BoT <objname> intersects with BoT <objname2>";
    if (_bot_cmd_msgs(bs, argc, argv, usage_string, purpose_string)) {
	return GED_OK;
    }

    struct _ged_bot_info *gb = (struct _ged_bot_info *)bs;

    if (argc != 2) {
        bu_vls_printf(gb->gedp->ged_result_str, "%s", usage_string);
        return GED_ERROR;
    }

    if (_bot_obj_setup(gb, argv[0]) == GED_ERROR) {
	return GED_ERROR;
    }

    struct rt_bot_internal *bot = (struct rt_bot_internal *)gb->intern->idb_ptr;
    
    struct directory *bot_dp_2;
    struct rt_db_internal intern_2;
    GED_DB_LOOKUP(gb->gedp, bot_dp_2, argv[1], LOOKUP_NOISY, GED_ERROR & GED_QUIET);
    GED_DB_GET_INTERNAL(gb->gedp, &intern_2, bot_dp_2, bn_mat_identity, &rt_uniresource, GED_ERROR);
    if (intern_2.idb_major_type != DB5_MAJORTYPE_BRLCAD || intern_2.idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_vls_printf(gb->gedp->ged_result_str, ": object %s is not of type bot\n", argv[1]);
	rt_db_free_internal(&intern_2);
	return GED_ERROR;
    }
    struct rt_bot_internal *bot_2 = (struct rt_bot_internal *)intern_2.idb_ptr;

    int fc_1 = (int)bot->num_faces;
    int fc_2 = (int)bot_2->num_faces;
    int vc_1 = (int)bot->num_vertices;
    int vc_2 = (int)bot_2->num_vertices;
    point_t *verts_1 = (point_t *)bot->vertices;
    point_t *verts_2 = (point_t *)bot_2->vertices;
    int *faces_1 = bot->faces;
    int *faces_2 = bot_2->faces;

    (void)bg_trimesh_isect(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	    faces_1, fc_1, verts_1, vc_1, faces_2, fc_2, verts_2, vc_2);

    rt_db_free_internal(&intern_2);

    return GED_OK;
}


extern "C" int
_bot_cmd_help(void *bs, int argc, const char **argv)
{
    struct _ged_bot_info *gb = (struct _ged_bot_info *)bs;
    if (!argc || !argv || BU_STR_EQUAL(argv[0], "help")) {
	bu_vls_printf(gb->gedp->ged_result_str, "bot [options] <objname> subcommand [args]\n");
	if (gb->gopts) {
	    char *option_help = bu_opt_describe(gb->gopts, NULL);
	    if (option_help) {
		bu_vls_printf(gb->gedp->ged_result_str, "Options:\n%s\n", option_help);
		bu_free(option_help, "help str");
	    }
	}
	bu_vls_printf(gb->gedp->ged_result_str, "Available subcommands:\n");
	const struct bu_cmdtab *ctp = NULL;
	int ret;
	const char *helpflag[2];
	helpflag[1] = PURPOSEFLAG;
	size_t maxcmdlen = 0;
	for (ctp = gb->cmds; ctp->ct_name != (char *)NULL; ctp++) {
	    maxcmdlen = (maxcmdlen > strlen(ctp->ct_name)) ? maxcmdlen : strlen(ctp->ct_name);
	}
	for (ctp = gb->cmds; ctp->ct_name != (char *)NULL; ctp++) {
	    bu_vls_printf(gb->gedp->ged_result_str, "  %s%*s", ctp->ct_name, (int)(maxcmdlen - strlen(ctp->ct_name)) + 2, " ");
	    if (!BU_STR_EQUAL(ctp->ct_name, "help")) {
		helpflag[0] = ctp->ct_name;
		bu_cmd(gb->cmds, 2, helpflag, 0, (void *)gb, &ret);
	    } else {
		bu_vls_printf(gb->gedp->ged_result_str, "print help and exit\n");
	    }
	}
    } else {
	int ret;
	const char **helpargv = (const char **)bu_calloc(argc+1, sizeof(char *), "help argv");
	helpargv[0] = argv[0];
	helpargv[1] = HELPFLAG;
	for (int i = 1; i < argc; i++) {
	    helpargv[i+1] = argv[i];
	}
	bu_cmd(gb->cmds, argc+1, helpargv, 0, (void *)gb, &ret);
	bu_free(helpargv, "help argv");
	return ret;
    }

    return GED_OK;
}

const struct bu_cmdtab _bot_cmds[] = {
    { "extrude",    _bot_cmd_extrude},
    { "get",        _bot_cmd_get},
    { "check",      _bot_cmd_check},
    { "chull",      _bot_cmd_chull},
    { "isect",      _bot_cmd_isect},
    { "remesh",     _bot_cmd_remesh},
    { (char *)NULL,      NULL}
};


static int
_ged_bot_opt_color(struct bu_vls *msg, size_t argc, const char **argv, void *set_c)
{
    struct bu_color **set_color = (struct bu_color **)set_c;
    BU_GET(*set_color, struct bu_color);
    return bu_opt_color(msg, argc, argv, (void *)(*set_color));
}

extern "C" int
ged_bot(struct ged *gedp, int argc, const char *argv[])
{
    int help = 0;
    struct _ged_bot_info gb;
    gb.gedp = gedp;
    gb.cmds = _bot_cmds;
    gb.verbosity = 0;
    gb.visualize = 0;
    struct bu_color *color = NULL;

    // Sanity
    if (UNLIKELY(!gedp || !argc || !argv)) {
	return GED_ERROR;
    }

    // Clear results
    bu_vls_trunc(gedp->ged_result_str, 0);

    // We know we're the bot command - start processing args
    argc--; argv++;

    // See if we have any high level options set
    struct bu_opt_desc d[5];
    BU_OPT(d[0], "h", "help",    "",      NULL,                 &help,         "Print help");
    BU_OPT(d[1], "v", "verbose", "",      NULL,                 &gb.verbosity, "Verbose output");
    BU_OPT(d[2], "V", "visualize", "",    NULL,                 &gb.visualize, "Visualize results");
    BU_OPT(d[3], "C", "color",   "r/g/b", &_ged_bot_opt_color,  &color,        "Set plotting color");
    BU_OPT_NULL(d[4]);

    gb.gopts = d;

    if (!argc) {
    	_bot_cmd_help(&gb, 0, NULL);
	return GED_OK;
    }

    // High level options are only defined prior to the subcommand
    int cmd_pos = -1;
    for (int i = 0; i < argc; i++) {
	if (bu_cmd_valid(_bot_cmds, argv[i]) == BRLCAD_OK) {
	    cmd_pos = i;
	    break;
	}
    }

    int acnt = (cmd_pos >= 0) ? cmd_pos : argc;

    int opt_ret = bu_opt_parse(NULL, acnt, argv, d);

    if (help) {
	if (cmd_pos >= 0) {
	    argc = argc - cmd_pos;
	    argv = &argv[cmd_pos];
	    _bot_cmd_help(&gb, argc, argv);
	} else {
	    _bot_cmd_help(&gb, 0, NULL);
	}
	return GED_OK;
    }

    // Must have a subcommand
    if (cmd_pos == -1) {
	bu_vls_printf(gedp->ged_result_str, ": no valid subcommand specified\n");
	_bot_cmd_help(&gb, 0, NULL);
	return GED_ERROR;
    }

    if (opt_ret < 0) {
	_bot_cmd_help(&gb, 0, NULL);
	return GED_ERROR;
    }

    // Jump the processing past any options specified
    for (int i = cmd_pos; i < argc; i++) {
	argv[i - cmd_pos] = argv[i];
    }

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    if (gb.visualize) {
	GED_CHECK_DRAWABLE(gedp, GED_ERROR);
	gb.vbp = rt_vlblock_init();
    }
    gb.color = color;

    int ret = GED_ERROR;
    if (bu_cmd(_bot_cmds, argc, argv, 0, (void *)&gb, &ret) == BRLCAD_OK) {
	ret = GED_OK;
	goto bot_cleanup;
    }

    bu_vls_printf(gedp->ged_result_str, "subcommand %s not defined", argv[0]);

bot_cleanup:
    if (gb.intern) {
	rt_db_free_internal(gb.intern);
	BU_PUT(gb.intern, struct rt_db_internal);
    }
    if (gb.visualize) {
	bn_vlblock_free(gb.vbp);
	gb.vbp = (struct bn_vlblock *)NULL;
    }
    if (color) {
	BU_PUT(color, struct bu_color);
    }
    return ret;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
