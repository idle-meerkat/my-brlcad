/*                          D M - Q T . C
 * BRL-CAD
 *
 * Copyright (c) 2013-2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file mged/dm-qt.c
 *
 * Routines specific to MGED's use of LIBDM's X display manager.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "./mged.h"
#include "./sedit.h"
#include "./mged_dm.h"

extern void dm_var_init(struct dm_list *initial_dm_list);		/* defined in attach.c */

static void
dirty_hook(const struct bu_structparse *UNUSED(sdp),
	const char *UNUSED(name),
	void *UNUSED(base),
	const char *UNUSED(value),
	void *data)
{
    struct mged_view_hook_state *hs = (struct mged_view_hook_state *)data;
    *(hs->dirty_global) = 1;
}

static void
zclip_hook(const struct bu_structparse *sdp,
	const char *name,
	void *base,
	const char *value,
	void *data)
{
    struct mged_view_hook_state *hs = (struct mged_view_hook_state *)data;
    hs->vs->vs_gvp->gv_zclip = dm_get_zclip(hs->hs_dmp);
    dirty_hook(sdp, name, base, value, data);
}

struct bu_structparse_map Qt_vparse_map[] = {
    {"bound",           dirty_hook                },
    {"useBound",        dirty_hook                },
    {"zclip",           zclip_hook                },
    {(char *)0,         BU_STRUCTPARSE_FUNC_NULL  }
};

static void *
set_hook_data(struct mged_view_hook_state *hs) {
    hs->hs_dmp = dmp;
    hs->vs = view_state;
    hs->dirty_global = &(dirty);
    return (void *)hs;
}


/*
  This routine is being called from doEvent() to handle Expose events.
*/
static int
qt_doevent(ClientData UNUSED(clientData), XEvent *eventPtr)
{
    if (eventPtr->type == Expose && eventPtr->xexpose.count == 0) {
	dirty = 1;

	/* no further processing of this event */
	return TCL_RETURN;
    }

    /* allow further processing of this event */
    return TCL_OK;
}

static int
qt_dm(int argc, const char *argv[])
{
    if (BU_STR_EQUAL(argv[0], "set")) {
	struct bu_vls vls = BU_VLS_INIT_ZERO;

	if (argc < 2) {
	    /* Bare set command, print out current settings */
	    bu_vls_struct_print2(&vls, "dm_qt internal variables", dm_get_vparse(dmp), (const char *)dmp);
	} else if (argc == 2) {
	    bu_vls_struct_item_named(&vls, dm_get_vparse(dmp), argv[1], (const char *)dmp, COMMA);
	} else {
	    struct bu_vls tmp_vls = BU_VLS_INIT_ZERO;
	    int ret;
	    struct mged_view_hook_state global_hs;
	    void *data = set_hook_data(&global_hs);

	    ret = dm_set_hook(qt_vparse_map, argv[1], data, &mged_dm_hook);

	    bu_vls_printf(&tmp_vls, "%s=\"", argv[1]);
	    bu_vls_from_argv(&tmp_vls, argc-2, (const char **)argv+2);
	    bu_vls_putc(&tmp_vls, '\"');
	    ret = bu_struct_parse(&tmp_vls, dm_get_vparse(dmp), (char *)dmp, (void *)(&mged_dm_hook));
	    bu_vls_free(&tmp_vls);
	    if (ret < 0) {
	      bu_vls_free(&vls);
	      return TCL_ERROR;
	    }
	}

	Tcl_AppendResult(INTERP, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);

	return TCL_OK;
    }

    return common_dm(argc, argv);
}


int
Qt_dm_init(struct dm_list *o_dm_list,
	  int argc,
	  const char *argv[])
{
    struct bu_vls vls = BU_VLS_INIT_ZERO;

    dm_var_init(o_dm_list);

    /* register application provided routines */
    cmd_hook = qt_dm;

    Tk_DeleteGenericHandler(doEvent, (ClientData)NULL);
    if ((dmp = dm_open(INTERP, DM_TYPE_QT, argc-1, argv)) == DM_NULL)
	return TCL_ERROR;

    /* keep display manager in sync */
    dm_set_perspective(dmp, mged_variables->mv_perspective_mode);

    eventHandler = qt_doevent;
    Tk_CreateGenericHandler(doEvent, (ClientData)NULL);

    (void)dm_configure_win(dmp, 0);

    bu_vls_printf(&vls, "mged_bind_dm %s", bu_vls_addr(dm_get_pathname(dmp)));
    Tcl_Eval(INTERP, bu_vls_addr(&vls));
    bu_vls_free(&vls);

    return TCL_OK;
}

void
Qt_fb_open(void)
{
    fbp = dm_get_fb(dmp);
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
