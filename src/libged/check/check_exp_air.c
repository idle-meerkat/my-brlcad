/*                C H E C K _ E X P _ A I R . C
 * BRL-CAD
 *
 * Copyright (c) 2018 United States Government as represented by
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

#include "common.h"

#include "ged.h"

#include "../ged_private.h"
#include "./check_private.h"


struct exp_air_context {
    struct region_pair *exposedAirList;
    FILE *plot_exp_air;
    int expAir_color[3];
};

HIDDEN void 
exposed_air(struct partition *pp,
	    point_t last_out_point,
	    point_t pt,
	    point_t opt,
	    void* callback_data)
{
    struct exp_air_context *context = (struct exp_air_context*) callback_data;
    /* this shouldn't be air */
    bu_semaphore_acquire(GED_SEM_LIST);
    add_unique_pair(context->exposedAirList,
	    pp->pt_regionp,
	    (struct region *)NULL,
	    pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist, /* thickness */
	    last_out_point); /* location */
    bu_semaphore_release(GED_SEM_LIST);

    if (context->plot_exp_air) {
	bu_semaphore_acquire(BU_SEM_SYSCALL);
	pl_color(context->plot_exp_air, V3ARGS(context->expAir_color));
	pdv_3line(context->plot_exp_air, pt, opt);
	bu_semaphore_release(BU_SEM_SYSCALL);
    }
}


int check_exp_air(struct current_state *state,
		  struct db_i *dbip,
		  char **tobjtab,
		  int tnobjs,
		  struct check_parameters *options)
{
    int flags = 0;

    FILE *plot_exp_air = NULL;
    char *name = "exp_air.plot3";
    struct exp_air_context callbackdata;
    int expAir_color[3] = { 255, 128, 255 }; /* magenta */

    /**
     * list of exposed air
     */
    static struct region_pair exposedAirList = {
	{
	    BU_LIST_HEAD_MAGIC,
	    (struct bu_list *)&exposedAirList,
	    (struct bu_list *)&exposedAirList
	},
	{ "Exposed Air" },
	(struct region *)NULL,
	(unsigned long)0,
	(double)0.0,
	{0.0, 0.0, 0.0, }
    };

    if (options->plot_files) {
	if ((plot_exp_air=fopen(name, "wb")) == (FILE *)NULL) {
	    bu_vls_printf(_ged_current_gedp->ged_result_str, "cannot open plot file %s\n", name);
	}
    }

    callbackdata.exposedAirList = &exposedAirList;
    callbackdata.plot_exp_air = plot_exp_air;
    VMOVE(callbackdata.expAir_color,expAir_color);

    flags |= ANALYSIS_EXP_AIR;
    analyze_register_exp_air_callback(state, exposed_air, &callbackdata);

    if (perform_raytracing(state, dbip, tobjtab, tnobjs, flags) == ANALYZE_ERROR){
	return GED_ERROR;
    }

    check_list_report(&exposedAirList, options->units);

    if (plot_exp_air) {
	fclose(plot_exp_air);
	bu_vls_printf(_ged_current_gedp->ged_result_str, "\nplot file saved as %s",name);
    }

    return GED_OK;
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
