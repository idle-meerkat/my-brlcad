/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647

	Thanks to Edwin O. Davisson and Robert Shnidman for contributions
	to the refraction algorithm.
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif
/*
	Originally extracted from SCCS archive:
		SCCS id:	@(#) reflect.c	2.3
		Modified: 	1/30/87 at 17:20:16	G S M
		Retrieved: 	2/4/87 at 08:52:40
		SCCS archive:	/vld/moss/src/lgt/s.reflect.c
*/

#include <stdio.h>
#include <signal.h>
#include <std.h>
#include <fcntl.h>
#include <fb.h>
#include <math.h>
#include <machine.h>
#include <vmath.h>
#include <raytrace.h>
#include <mat_db.h>
#include <vecmath.h>
#include <lgt.h>
#include "./tree.h"
#include "./extern.h"
#include "./screen.h"

#define TWO_PI		6.28318530717958647692528676655900576839433879875022
#define RI_AIR		1.0    /* Refractive index of air.		*/

static Mat_Db_Entry	mat_tmp_entry =
				{
				0,		/* Material id.		*/
				4,		/* Shininess.		*/
				0.6,		/* Specular weight.	*/
				0.3,		/* Diffuse weight.	*/
				0.0,		/* Reflectivity.	*/
				0.0,		/* Transmission.	*/
				1.0,		/* Refractive index.	*/
				255, 255, 255,	/* Diffuse RGB values.	*/
				MF_USED,	/* Mode flag.		*/
				"(default)"	/* Material name.	*/
				};

/* Collect statistics on refraction.					*/
static int		refract_missed;
static int		refract_inside;
static int		refract_total;

/* Collect statistics on shadowing.					*/
static int		hits_shadowed;
static int		hits_lit;

/* Local communication with worker().					*/
static int curr_scan;		/* Current scan line number.		*/
static int last_scan;		/* Last scan.				*/
#ifdef PARALLEL
static int nworkers;		/* Number of workers now running.	*/
#endif
static int a_gridsz;
static fastf_t	grid_dh[3], grid_dv[3];
static struct application ag;	/* Global application structure.	*/

#ifdef cray
struct taskcontrol {
	int	tsk_len;
	int	tsk_id;
	int	tsk_value;
} taskcontrol[MAX_PSW];
#endif

_LOCAL_ fastf_t		ipow();
_LOCAL_ fastf_t		correct_Lgt();
_LOCAL_ fastf_t		*mirror_Reflect();

/* "Hit" application routines to pass to "rt_shootray()".		*/
_LOCAL_ int		do_Model(), do_Probe(), do_Shadow();
/* "Miss" application routines to pass to "rt_shootray()".		*/
_LOCAL_ int		do_Backgr(), do_Error(), do_Lit();

_LOCAL_ int		refract();

_LOCAL_ void		model_Reflectance();
_LOCAL_ void		glass_Refract();
_LOCAL_ void		view_pix(), view_eol(), view_end();

void			cons_Vector();
void			render_Model();
#if defined( BSD ) || defined( sgi )
int	abort_RT();
#else
void	abort_RT();
#endif


/*	r e n d e r _ M o d e l ( )					*/
void
render_Model()
	{
#ifdef alliant
	register int	d7;	/* known to be in d7 */
#endif
	int		a, x;
	if( tty )
		{
		rt_prep_timer();
		prnt_Event( "Raytracing..." );
		SCROLL_DL_MOVE();
		(void) fflush( stdout );
		}
#ifdef PARALLEL
	pix_buffered = B_LINE;
#endif
	if( aperture_sz < 1 )
		aperture_sz = 1;
	if( ir_mapping & IR_OCTREE )
		{
		ag.a_hit = do_IR_Model;
		ag.a_miss = do_IR_Backgr;
		}
	else
		{
		ag.a_hit = do_Model;
		ag.a_miss = do_Backgr;
		}
	ag.a_rt_i = &rt_i;
	ag.a_onehit = max_bounce > 0 ? 0 : 1;
	ag.a_rbeam = modl_radius / grid_sz;
	ag.a_diverge = 0.0;

	/* Compute light source positions.				*/
	if( ! setup_Lgts() )
		return;

	/* Compute grid vectors of magnitude of one cell.
		These will be the delta vectors between adjacent cells.
	 */
	a_gridsz = anti_aliasing ? grid_sz * aperture_sz : grid_sz;
	if( save_view_flag )
		{ /* Saved view from GED, match view size.		*/
		if( rel_perspective != 0.0 )
			/* Animation sequence, perspective gridding.	*/
			cell_sz = EYE_SIZE / (fastf_t) a_gridsz;
		else
			cell_sz = view_size / (fastf_t) a_gridsz;
		}
	else
		cell_sz = modl_radius * 2.0/ (fastf_t) a_gridsz * grid_scale;
	Scale2Vec( grid_hor, cell_sz, grid_dh );
	Scale2Vec( grid_ver, cell_sz, grid_dv );

	/* Statistics for refraction tuning.				*/ 
	refract_missed = 0;
	refract_inside = 0;
	refract_total = 0;

	/* Statistics for shadowing.					*/
	hits_shadowed = 0;
	hits_lit = 0;

	fatal_error = FALSE;

	/* Get starting and ending scan line number.			*/
	curr_scan = grid_y_org;
	last_scan = grid_y_fin;

#ifdef PARALLEL
	/*
	 *  Parallel case.
	 *  The workers are started and terminated here.
	 */
	nworkers = 0;
#ifdef cray
	/* Create any extra worker tasks */
RES_ACQUIRE( &rt_g.res_worker );
	for( x=1; x<npsw; x++ ) {
		taskcontrol[x].tsk_len = 3;
		taskcontrol[x].tsk_value = x;
		TSKSTART( &taskcontrol[x], render_Scan, x );
	}
for( x=0; x<1000000; x++ ) a=x+1;	/* take time to get started */
RES_RELEASE( &rt_g.res_worker );
	render_Scan(0);	/* avoid wasting this task */
	/* Wait for them to finish */
	for( x=1; x<npsw; x++ )  {
		TSKWAIT( &taskcontrol[x] );
	}
#endif
#ifdef alliant
	{
		asm("	movl		_npsw,d0");
		asm("	subql		#1,d0");
		asm("	cstart		d0");
		asm("super_loop:");
		render_Scan(d7);	/* d7 has current index, like magic */
		asm("	crepeat		super_loop");
	}
#endif
	/* Ensure that all the workers are REALLY dead */
	x = 0;
	while( nworkers > 0 )  x++;
	if( x > 0 )
		rt_log( "Termination took %d extra loops\n", x );
#else
	/*
	 * SERIAL case -- one CPU does all the work.
	 */
	render_Scan( 0 );
#endif
	view_end();
	return;
	}

render_Scan( cpu )
int	cpu;
	{	fastf_t		grid_y_inc[3], grid_x_inc[3];
		RGBpixel	scanbuf[1024];
		RGBpixel	*scanp;	/* Pointer into line buffer.	*/
		
	/* Must have local copy of application structure for parallel
		threads of execution, so make copy.			*/
		struct application	a;

		register int com;
#ifdef PARALLEL
	RES_ACQUIRE( &rt_g.res_worker );
	com = nworkers++;
	RES_RELEASE( &rt_g.res_worker );

	a.a_resource = &resource[cpu];
	resource[cpu].re_cpu = cpu;
#else
	a.a_resource = RESOURCE_NULL;
#endif
	for( ; ! user_interrupt; )
		{
		RES_ACQUIRE( &rt_g.res_worker );
		com = curr_scan++;
		RES_RELEASE( &rt_g.res_worker );

		if( com > last_scan )
			break;
		a.a_x = grid_x_org;
		a.a_y = com;
		a.a_hit = ag.a_hit;
		a.a_miss = ag.a_miss;
		a.a_rt_i = ag.a_rt_i;
		a.a_rbeam = ag.a_rbeam;
		a.a_diverge = ag.a_diverge;
		if( anti_aliasing )
			{
			a.a_x *= aperture_sz;
			a.a_y *= aperture_sz;
			}
		for(	;
			! user_interrupt
		     &&	a.a_y < (com+1) * aperture_sz;
			view_eol( &a, (RGBpixel *) scanbuf, scanp ), a.a_y++
			)
			{
			/* Compute vectors from center to origin (bottom-left) of grid.	*/
			Scale2Vec( grid_dv, (fastf_t)(-a_gridsz/2)+a.a_y, grid_y_inc );
			Scale2Vec( grid_dh, (fastf_t)(-a_gridsz/2)+a.a_x, grid_x_inc );
			scanp = scanbuf;
			for(	;
				! user_interrupt
			     &&	a.a_x < (grid_x_fin+1) * aperture_sz;
				view_pix( &a, scanp ), a.a_x++
				)
				{	fastf_t		aim_pt[3];
				if( anti_aliasing )
					{
					if( a.a_x > grid_x_org * aperture_sz
					    &&	a.a_x % aperture_sz == 0
						)
						scanp++;
					}
				else
					scanp++;
				if( rel_perspective == 0.0 )
					{ /* Parallel rays emanating from grid.	*/
					Add2Vec( grid_loc, grid_y_inc, aim_pt );
					Add2Vec( aim_pt, grid_x_inc, a.a_ray.r_pt );
					Scale2Vec( lgts[0].dir, -1.0, a.a_ray.r_dir );
					}
				else	
				/* Fire a ray at model from the zeroth point-light-
					source position "lgts[0].loc" through each
					grid cell. The closer the source is to the
					grid, the more perspective there will be;
				 */
					{
					VMOVE( a.a_ray.r_pt, lgts[0].loc );
					/* Compute ray direction.		*/
					Add2Vec( grid_loc, grid_y_inc, aim_pt );
					AddVec( aim_pt, grid_x_inc );
					Diff2Vec( aim_pt, lgts[0].loc, a.a_ray.r_dir );
					VUNITIZE( a.a_ray.r_dir );
					}
				a.a_level = 0;	 /* Recursion level (bounces).	*/
				if( ir_mapping & IR_OCTREE )
					{
					if( ir_shootray_octree( &a ) == -1 )
						{
						/* Fatal error in application routine.	*/
						rt_log( "Fatal error: raytracing aborted.\n" );
						return;
						}
					}
				else
				if( rt_shootray( &a ) == -1 && fatal_error )
					{
					/* Fatal error in application routine.	*/
					rt_log( "Fatal error: raytracing aborted.\n" );
					return;
					}
				AddVec( grid_x_inc, grid_dh )
				}
			}
		}
#ifdef PARALLEL
	RES_ACQUIRE( &rt_g.res_worker );
	nworkers--;
	RES_RELEASE( &rt_g.res_worker );
#endif
	return;
	}

/*	d o _ M o d e l ( )
	'Hit' application specific routine for 'rt_shootray()' from
	observer or a bounced ray.

 */
_LOCAL_ int
do_Model( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
	{	register struct partition	*pp;
		register Mat_Db_Entry		*entry;
		register struct soltab		*stp;
		register struct hit		*ihitp, *ohitp;
		int				material_id;
		fastf_t				rgb_coefs[3];
	for(	pp = PartHeadp->pt_forw;
		pp != PartHeadp
	    &&	pp->pt_outhit->hit_dist < 0.1;
		pp = pp->pt_forw
		) 
		;
	if( pp == PartHeadp || pp->pt_outhit->hit_dist < 0.1 )
		return	ap->a_miss( ap );
	stp = pp->pt_inseg->seg_stp;
	ihitp = pp->pt_inhit;
	RT_HIT_NORM( ihitp, stp, &(ap->a_ray) );

	/* Check for flipped normal and fix.				*/
	if( pp->pt_inflip )
		{
		ScaleVec( ihitp->hit_normal, -1.0 );
		pp->pt_inflip = 0;
		}
	stp = pp->pt_outseg->seg_stp;
	ohitp = pp->pt_outhit;
	RT_HIT_NORM( ohitp, stp, &(ap->a_ray) );
	if( pp->pt_outflip )
		{
		ScaleVec( ohitp->hit_normal, -1.0 );
		pp->pt_outflip = 0;
		}

	{	fastf_t f = Dot( ap->a_ray.r_dir, ihitp->hit_normal );
	if( f >= 0.0 )
		{
		ScaleVec( ihitp->hit_normal, -1.0 );
		if( rt_g.debug )
			rt_log( "Fixed flipped entry normal\n" );
		}
	}

	/* See if we hit a light source.				*/
	{	register int	i;
	for(	i = 1;
		i < lgt_db_size && stp != lgts[i].stp;
		i++
		)
		;
	if( i < lgt_db_size && lgts[i].energy > 0.0 )
		{ /* Maximum light coming from light source.		*/
		ap->a_color[0] = lgts[i].rgb[0];
		ap->a_color[1] = lgts[i].rgb[1];
		ap->a_color[2] = lgts[i].rgb[2];
		return	1;
		}
	}

	/* Get material id as index into material database.		*/
	if( icon_mapping && strncmp( stp->st_name, "TM_", 3 ) == 0 )
		{ /* Solid has a texture map.				*/
			struct uvcoord	uv;
		rt_functab[stp->st_id].ft_uv( ap, stp, ihitp, &uv );
		material_id = texture_Val( &uv );
		}
	else
		material_id = (int)(pp->pt_regionp->reg_gmater);

	/* Get material database entry.					*/
	if( ir_mapping )
		{ /* We are mapping temperatures into an octree.	*/
			Trie		*triep;
			Octree		*octreep;
			int		fahrenheit;
		if( ir_offset )
			{	RGBpixel	pixel;
				int	x = ap->a_x + x_fb_origin - ir_mapx;
				int	y = ap->a_y + y_fb_origin - ir_mapy;
			/* Map temperature from IR image using offsets.	*/
			if(	x < 0 || y < 0
			    ||	fb_seek( fbiop, x, y ) == -1
			    ||	fb_rpixel( fbiop, pixel ) == -1
				)
				fahrenheit = AMBIENT-1;
			else
				fahrenheit = pixel_To_Temp( pixel );
			}
		else
		if( ir_paint_flag )
			/* User specified temp. of current rectangle.	*/
			fahrenheit = ir_paint;
		else
			/* Unknown temperature, use out-of-band value.	*/
			fahrenheit = AMBIENT-1;
		if( ir_table == PIXEL_NULL )
			{
			rt_log( "Must read real IR data or IR data base first.\n" );
			fatal_error = TRUE;
			return	-1;
			}
		entry = &mat_tmp_entry;
		if( ir_mapping & IR_READONLY )
			{	int	ir_level = 0;
			octreep = find_Octant(	&ir_octree,
						ihitp->hit_point,
						&ir_level
						);
			}
		else
		if( ir_mapping & IR_EDIT )
			{
			triep = add_Trie( pp->pt_regionp->reg_name, &reg_triep );
			octreep = add_Region_Octree(	&ir_octree,
							ihitp->hit_point,
							triep,
							fahrenheit,
							0
							);
			if( octreep != OCTREE_NULL )
				append_Octp( triep, octreep );
			else
			if( fatal_error )
				return	-1;
			}
		if( octreep != OCTREE_NULL )
			{	register int	index;
			index = octreep->o_temp - ir_min;
			index = index < 0 ? 0 : index;
			COPYRGB( entry->df_rgb, ir_table[index] );
			}
		}
	else
	if( fb_mapping && strncmp( stp->st_name, "FB_", 3 ) == 0 )
		{ /* Solid has a frame buffer image map.		*/
			struct uvcoord	uv;
		rt_functab[stp->st_id].ft_uv( ap, stp, ihitp, &uv );
		if( (entry = fb_Entry( &uv )) == MAT_DB_NULL )
			entry = &mat_dfl_entry;
		}
	else	/* Get material attributes from database.		*/
	if(	(entry = mat_Get_Db_Entry( material_id )) == MAT_DB_NULL
	   || ! (entry->mode_flag & MF_USED)
		)
		{
		rt_log( "No material database entry for %d, using default.\n",
			material_id
			);
		entry = &mat_dfl_entry;
		}

	if( lgts[0].energy < 0.0 )
		{	fastf_t	f = RGB_INVERSE;
			/* Scale RGB values to coeffs (0.0 .. 1.0 )	*/
		/* Negative intensity on ambient light is a flag meaning
			do not calculate lighting effects at all, but
			produce a flat, pseudo-color mapping.
		 */
		ap->a_color[0] = entry->df_rgb[0] * f;
		ap->a_color[1] = entry->df_rgb[1] * f;
		ap->a_color[2] = entry->df_rgb[2] * f;
		return	1;
		}

	/* Compute contribution from this surface.			*/
	{	fastf_t	f;
		register int	i;
		auto fastf_t	view_dir[3];

	/* Calculate view direction.					*/
	Scale2Vec( ap->a_ray.r_dir, -1.0, view_dir );

	rgb_coefs[0] = rgb_coefs[1] = rgb_coefs[2] = 0.0;
	if( (f = 1.0 - (entry->reflectivity + entry->transparency)) > 0.0 )
		{
		for( i = 0; i < lgt_db_size; i++ )
			{ /* All light sources.				*/
			if( lgts[i].energy > 0.0 )
				{
				ap->a_user = i;
				model_Reflectance( ap, pp, entry, &lgts[i],
							view_dir
							);
				VJOIN1(	rgb_coefs, rgb_coefs, f, ap->a_color );
				if( rt_g.debug & DEBUG_SHADOW )
					{
					rt_log( "light %d returns\n", i );
					V_Print( "ap->a_color", ap->a_color, rt_log );
					V_Print( "rgb_coefs", rgb_coefs, rt_log );
					}
				}
			}
		}
	}

	/* Recursion is used to do multiple bounces for mirror reflections
		and transparency.  The level of recursion is stored in
		'a_level' and is controlled via the variable 'max_bounce'.
	 */
	if( ap->a_level + 1 <= max_bounce )
		{
		ap->a_user = material_id;
		if( entry->reflectivity > 0.0 )
			{	register fastf_t *mirror_coefs =
						mirror_Reflect( ap, pp );
			/* Compute mirror reflection.			*/
			VJOIN1(	rgb_coefs,
				rgb_coefs,
				entry->reflectivity,
				mirror_coefs
				);
			if( rt_g.debug & DEBUG_RGB )
				{
				V_Print( "mirror", mirror_coefs, rt_log );
				V_Print( "rgb_coefs", rgb_coefs, rt_log );
				}
			}
		if( entry->transparency > 0.0 )
			{
			glass_Refract( ap, pp, entry );
			/* Compute transmission through glass.		*/
			VJOIN1( rgb_coefs,
				rgb_coefs,
				entry->transparency,
				ap->a_color
				);
			if( rt_g.debug & DEBUG_RGB )
				{
				V_Print( "glass", ap->a_color, rt_log );
				V_Print( "rgb_coefs", rgb_coefs, rt_log );
				}
			}
		}
	/* Pass result in application struct.				*/
	VMOVE( ap->a_color, rgb_coefs );
	if( rt_g.debug & DEBUG_RGB )
		{
		V_Print( "ap->a_color", ap->a_color, rt_log );
		}
	return	1;
	}

/*	c o r r e c t _ L g t ( )
	Shoot a ray to the light source to determine if surface
	is shadowed, return corrected light source intensity.
 */
_LOCAL_ fastf_t
correct_Lgt( ap, pp, lgt_entry )
register struct application	*ap;
register struct partition	*pp;
register Lgt_Source		*lgt_entry;
	{	struct application	ap_hit;
	/* Set up application struct for 'rt_shootray()' to light source.	*/
	ap_hit = *ap;
	ap_hit.a_onehit = 0;	  /* Go all the way to the light.	*/
	ap_hit.a_hit = do_Shadow; /* Handle shadowed pixels.		*/
	ap_hit.a_miss = do_Lit;   /* Handle illuminated pixels.		*/
	ap_hit.a_level++;	  /* Increment recursion level.		*/

	if( rt_g.debug & DEBUG_SHADOW )
		{
		rt_log( "correct_Lgt()\n" );
		V_Print( "light loc", lgt_entry->loc, rt_log );
		}
	/* Vector to light src from surface contact pt.	 		*/
	Diff2Vec(	lgt_entry->loc,
			pp->pt_inhit->hit_point,
			ap_hit.a_ray.r_dir
			);
	VUNITIZE( ap_hit.a_ray.r_dir );

	/* Set up ray origin at surface contact point.			*/
	VMOVE( ap_hit.a_ray.r_pt, pp->pt_inhit->hit_point );

	if( rt_g.debug & DEBUG_SHADOW )
		{
		V_Print( "ray to light", ap_hit.a_ray.r_dir, rt_log );
		V_Print( "origin of ray", ap_hit.a_ray.r_pt, rt_log );
		}
	/* Fetch attenuated light intensity into "ap_hit.a_diverge".	*/
	(void) rt_shootray( &ap_hit );
	if( ap_hit.a_diverge == 0.0 )
		/* Shadowed by opaque object(s).			*/
		return	0.0;
	/* Light is either full intensity or attenuated by transparent
		object(s).
	 */
	if( lgt_entry->beam )
		/* Apply gaussian intensity distribution.		*/
		{	fastf_t lgt_cntr[3];
			fastf_t ang_dist, rel_radius;
			fastf_t	cos_angl;
			fastf_t	gauss_Wgt_Func();
		if( lgt_entry->stp == SOLTAB_NULL )
			cons_Vector( lgt_cntr, lgt_entry->azim, lgt_entry->elev );
		else
			{
			Diff2Vec( lgt_entry->loc, modl_cntr, lgt_cntr );
			VUNITIZE( lgt_cntr );
			}
		cos_angl = Dot( lgt_cntr, ap_hit.a_ray.r_dir );
		if( NEAR_ZERO( cos_angl, EPSILON ) )
			/* Negligable intensity.			*/
			return	0.0;
		ang_dist = sqrt( 1.0 - Sqr( cos_angl ) );
		rel_radius = lgt_entry->radius / pp->pt_inhit->hit_dist;
		if( rt_g.debug & DEBUG_GAUSS )
			{
			rt_log( "cos_angl=%g\n", cos_angl );
			rt_log( "ang_dist=%g\n", ang_dist );
			rt_log( "rel_radius=%g\n", rel_radius );
			rt_log( "rel_dist=%g\n", ang_dist/rel_radius );
			}
		/* Return weighted and attenuated light intensity.	*/
		return	gauss_Wgt_Func( ang_dist/rel_radius ) *
			lgt_entry->energy * ap_hit.a_diverge;
		}
	else	/* Return attenuated light intensity.			*/
		return	lgt_entry->energy * ap_hit.a_diverge;
	}

/*	m i r r o r _ R e f l e c t ( )					*/
_LOCAL_ fastf_t *
mirror_Reflect( ap, pp )
register struct application	*ap;
register struct partition	*pp;
	{	fastf_t			r_dir[3];
		struct application	ap_hit;
	ap_hit = *ap;		/* Same as initial application.		*/
	ap_hit.a_level++;	/* Increment recursion level.		*/

	if( rt_g.debug & DEBUG_RGB )
		rt_log( "mirror_Reflect()\n" );

	/* Calculate reflected incident ray.				*/
	Scale2Vec( ap->a_ray.r_dir, -1.0, r_dir );
	{	fastf_t	f = 2.0	* Dot( r_dir, pp->pt_inhit->hit_normal );
		fastf_t tmp_dir[3];
	Scale2Vec( pp->pt_inhit->hit_normal, f, tmp_dir );
	Diff2Vec( tmp_dir, r_dir, ap_hit.a_ray.r_dir );
	}
	/* Set up ray origin at surface contact point.			*/
	VMOVE( ap_hit.a_ray.r_pt, pp->pt_inhit->hit_point );
	(void) rt_shootray( &ap_hit );
	return	ap_hit.a_color;
	}

/*	g l a s s _ R e f r a c t ( )					*/
_LOCAL_ void
glass_Refract( ap, pp, entry )
register struct application	*ap;
register struct partition	*pp;
register Mat_Db_Entry		*entry;
	{	struct application	ap_hit;	/* To shoot ray beyond.	*/
		struct application	ap_ref; /* For getting thru.	*/
	/* Application structure for refracted ray.			*/
	ap_ref = *ap;
	ap_ref.a_hit =  do_Probe;	/* Find exit from glass.	*/
	ap_ref.a_miss = do_Error;	/* Bad news.			*/
	ap_ref.a_level++;		/* Increment recursion level.	*/

	/* Application structure for exiting ray.			*/
	ap_hit = *ap;
	ap_hit.a_level++;

	if( rt_g.debug & DEBUG_REFRACT )
		{
		rt_log( "Entering glass_Refract(), level %d grid <%d,%d>\n",
			ap->a_level, ap->a_x, ap->a_y
			);
		V_Print( "\tincident ray pnt", ap->a_ray.r_pt, rt_log );
		V_Print( "\tincident ray dir", ap->a_ray.r_dir, rt_log );
		}
	refract_total++;

	if( entry->refrac_index == RI_AIR )
		{ /* No refraction necessary.				*/
		if( rt_g.debug & DEBUG_REFRACT )
			rt_log( "\tNo refraction on entry.\n" );
		/* Ray direction stays the same, origin becomes exit pt.*/
		VMOVE( ap_hit.a_ray.r_dir, ap->a_ray.r_dir );
		VMOVE( ap_hit.a_ray.r_pt, pp->pt_outhit->hit_point );
		goto	exiting_ray;
		}
	else
		/* Set up ray-trace to find new exit point.		*/
		{ /* Calculate refraction at entrance.			*/
		if( pp->pt_inhit->hit_dist < 0.0 )
			{
			if( rt_g.debug & DEBUG_REFRACT )
				rt_log( "\tRefracting inside solid.\n" );
			VMOVE( ap_ref.a_ray.r_pt, ap->a_ray.r_pt );
			VMOVE( ap_ref.a_ray.r_dir, ap->a_ray.r_dir );
			goto	inside_ray;
			}
		if( ! refract(	ap->a_ray.r_dir,   /* Incident ray.	*/
				pp->pt_inhit->hit_normal,
				RI_AIR,		   /* Air ref. index.	*/
				entry->refrac_index,
				ap_ref.a_ray.r_dir /* Refracted ray.	*/
				)
			)
			{ /* Past critical angle, reflected back out.	*/
			VMOVE( ap_hit.a_ray.r_pt, pp->pt_inhit->hit_point );
			VMOVE( ap_hit.a_ray.r_dir, ap_ref.a_ray.r_dir );
			if( rt_g.debug & DEBUG_REFRACT )
				rt_log( "\tPast critical angle on entry!\n" );
			goto	exiting_ray;
			}
		}
	/* Fire from entry point.					*/
	VMOVE( ap_ref.a_ray.r_pt, pp->pt_inhit->hit_point );

inside_ray :
	/* Find exit of refracted ray, exit normal (reversed) returned in
		ap_ref.a_uvec, and exit point returned in ap_ref.a_color.
	 */
	if( ! rt_shootray( &ap_ref ) )
		{ /* Refracted ray missed, solid!  This should not occur,
			but if it does we will skip the refraction.
		   */
		if( rt_g.debug & DEBUG_REFRACT )
			{
			rt_log( "\tRefracted ray missed!\n" );
			V_Print( "\trefracted ray pnt", ap_ref.a_ray.r_pt, rt_log );
			V_Print( "\trefracted ray dir", ap_ref.a_ray.r_dir, rt_log );
			}
		refract_missed++;
		VMOVE( ap_hit.a_ray.r_pt, pp->pt_outhit->hit_point );
		VMOVE( ap_hit.a_ray.r_dir, ap->a_ray.r_dir );
		goto	exiting_ray;
		}
	else
		{
		if( rt_g.debug & DEBUG_REFRACT )
			rt_log( "\tRefracted ray hit.\n" );
		}

	/* Calculate refraction at exit.				*/
	if( ap_ref.a_level <= max_bounce )
		{
		/* Reversed exit normal in a_uvec.			*/
		if( ! refract(	ap_ref.a_ray.r_dir,
				ap_ref.a_uvec,
				entry->refrac_index,
				RI_AIR,
				ap_hit.a_ray.r_dir
				)
			)
			{ /* Past critical angle, internal reflection.	*/
			if( rt_g.debug & DEBUG_REFRACT )
				rt_log( "\tInternal reflection, recursion level (%d)\n", ap_ref.a_level );
			ap_ref.a_level++;
			VMOVE( ap_ref.a_ray.r_dir, ap_hit.a_ray.r_dir );
			/* Refracted ray exit point in a_color.		*/
			VMOVE( ap_ref.a_ray.r_pt, ap_ref.a_color );
			goto	inside_ray;
			}
		}
	else
		{ /* Exceeded max bounces, total absorbtion of light.	*/
		ap->a_color[0] = ap->a_color[1] = ap->a_color[2] = 0.0;
		if( rt_g.debug & DEBUG_REFRACT )
			rt_log( "\tExceeded max bounces with internal reflections, recursion level (%d)\n", ap_ref.a_level );
		refract_inside++;
		return;
		}
	/* Refracted ray exit point in a_color.				*/
	VMOVE( ap_hit.a_ray.r_pt, ap_ref.a_color );

exiting_ray :
	/* Shoot from exit point in direction of refracted ray.		*/
	(void) rt_shootray( &ap_hit );
	VMOVE( ap->a_color, ap_hit.a_color );
	return;
	}

/*	d o _ B a c k g r ( )
	'Miss' application specific routine for 'rt_shootray()' from
	observer or a bounced ray.
 */
_LOCAL_ int
do_Backgr( ap )
register struct application *ap;
	{	register int	i;
	/* Base-line color is same as background.			*/
	VMOVE( ap->a_color, bg_coefs );

	if( rt_g.debug & DEBUG_RGB )
		{
		rt_log( "do_Backgr()\n" );
		V_Print( "bg_coefs", ap->a_color, rt_log );
		}

	/* If this is a reflection, we may see each light source.	*/
	if( ap->a_level )
		{	Mat_Db_Entry	*mdb_entry =
					mat_Get_Db_Entry( ap->a_user );
		for( i = 1; i < lgt_db_size; i++ )
			{	auto fastf_t		real_l_1[3];
				register fastf_t	specular;
				fastf_t			cos_s;
			if( lgts[i].energy <= 0.0 )
				continue;
			Diff2Vec( lgts[i].loc, ap->a_ray.r_pt, real_l_1 );
			VUNITIZE( real_l_1 );
			if(	(cos_s = Dot( ap->a_ray.r_dir, real_l_1 ))
				> 0.0
			    &&	cos_s <= 1.0
				)
				{
				specular = mdb_entry->wgt_specular *
					lgts[i].energy *
					ipow( cos_s, mdb_entry->shine );
				/* Add reflected light source.		*/
				VJOIN1(	ap->a_color,
					ap->a_color,
					specular,
					lgts[i].coef );
				}
			}
		}
	if( rt_g.debug & DEBUG_RGB )
		{
		V_Print( "ap->color final", ap->a_color, rt_log );
		}
	return	0;
	}

/*	d o _ E r r o r ( )						*/
/*ARGSUSED*/
_LOCAL_ int
do_Error( ap )
register struct application *ap;
	{
	if( rt_g.debug & DEBUG_RGB )
		rt_log( "do_Error()\n" );
	return	0;
	}

/*	d o _ L i t ( )
	'Miss' application specific routine for 'rt_shootray()' to
	light source for shadowing.  Return full intensity in "ap->a_diverge".
 */
_LOCAL_ int
do_Lit( ap )
register struct application *ap;
	{	
	if( rt_g.debug & DEBUG_SHADOW )
		rt_log( "do_Lit()\n" );
	ap->a_diverge = 1.0;
	hits_lit++;
	return	0;
	}

/*	d o _ P r o b e ( )						*/
_LOCAL_ int
do_Probe( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
	{	register struct partition	*pp;
		register struct hit		*hitp;
		register struct soltab		*stp;
	if( rt_g.debug & DEBUG_RGB )
		rt_log( "do_Probe()\n" );
	for(	pp = PartHeadp->pt_forw;
		pp != PartHeadp
	    &&	pp->pt_outhit->hit_dist < 0.1;
		pp = pp->pt_forw
		) 
		;
	if( pp == PartHeadp || pp->pt_outhit->hit_dist < 0.1 )
		{
		if( rt_g.debug & DEBUG_REFRACT )
			rt_log( "partition behind ray origin, no exit\n" );
		return	ap->a_miss( ap );
		}
	stp = pp->pt_outseg->seg_stp;
	hitp = pp->pt_outhit;
	RT_HIT_NORM( hitp, stp, &(ap->a_ray) );
	VMOVE( ap->a_uvec, hitp->hit_normal );
	VMOVE( ap->a_color, hitp->hit_point );
	if( ! pp->pt_outflip )
		{ /* For refraction, want exit normal to point inward.	*/
		ScaleVec( ap->a_uvec, -1.0 );
		}
	return	1;
	}

/*	r e f r a c t ( )
	Compute the refracted ray 'v_2' from the incident ray 'v_1' with
	the refractive indices 'ri_2' and 'ri_1' respectively.

	Using Schnell's Law

		theta_1 = angle of v_1 with surface normal
		theta_2 = angle of v_2 with reversed surface normal
		ri_1 * sin( theta_1 ) = ri_2 * sin( theta_2 )

		sin( theta_2 ) = ri_1/ri_2 * sin( theta_1 )
		
	The above condition is undefined for ri_1/ri_2 * sin( theta_1 )
	being greater than 1, and this represents the condition for total
	reflection, the 'critical angle' is the angle theta_1 for which
	ri_1/ri_2 * sin( theta_1 ) equals 1.
 */
_LOCAL_ int
refract( v_1, norml, ri_1, ri_2, v_2 )
register fastf_t	*v_1, *norml;
fastf_t			ri_1, ri_2;
register fastf_t	*v_2;
	{	fastf_t	w[3], u[3];	/* Intermediate vectors.	*/
		fastf_t	beta;		/* Intermediate scalar.		*/
	if( rt_g.debug & DEBUG_REFRACT )
		{
		V_Print( "Entering refract(), incident ray", v_1, rt_log );
		V_Print( "\tentrance normal", norml, rt_log );
		rt_log( "\trefractive indices leaving:%g, entering:%g\n", ri_1, ri_2 );
		}
	if( ri_2 == 0.0 )
		{ /* User probably forgot to specify refractive index.	*/
		rt_log( "\tZero refractive index." );
		VMOVE( v_2, v_1 ); /* Just return ray unchanged.	*/
		return	1;
		}
	beta = ri_1 / ri_2;
	Scale2Vec( v_1, beta, w );	
	CrossProd( w, norml, u );
	/*	|w X norml| = |w||norml| * sin( theta_1 )
		        |u| = ri_1/ri_2 * sin( theta_1 ) = sin( theta_2 )
	 */
	if( (beta = Dot( u, u )) > 1.0 ) /* beta = sin( theta_2 )^^2.	*/
		{ /* Past critical angle, total reflection.
			Calculate reflected (bounced) incident ray.
		   */
		Scale2Vec( v_1, -1.0, u );
		beta = 2.0 * Dot( u, norml );
		Scale2Vec( norml, beta, w );
		Diff2Vec( w, u, v_2 );
		if( rt_g.debug & DEBUG_REFRACT )
			{
			V_Print( "\tdeflected refracted ray", v_2, rt_log );
			}
		return	0;
		}
	else
		{
		/* 1 - beta = 1 - sin( theta_2 )^^2
			    = cos( theta_2 )^^2.
		       beta = -1.0 * cos( theta_2 ) - Dot( w, norml ).
		 */
		beta = -sqrt( 1.0 - beta ) - Dot( w, norml );
		Scale2Vec( norml, beta, u );
		Add2Vec( w, u, v_2 );
		if( rt_g.debug & DEBUG_REFRACT )
			{
			V_Print( "\trefracted ray", v_2, rt_log );
			}
		return	1;
		}
	/*NOTREACHED*/
	}

/*	d o _ S h a d o w ( )
	'Hit' application specific routine for 'rt_shootray()' to
	light source for shadowing. Returns attenuated light intensity in
	"ap->a_diverge".
 */
_LOCAL_ int
do_Shadow( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
	{	register struct partition	*pp;
		register Mat_Db_Entry		*entry;
	for(	pp = PartHeadp->pt_forw;
		pp != PartHeadp
	    &&	pp->pt_outhit->hit_dist < 0.1;
		pp = pp->pt_forw
		) 
		;
	if( pp == PartHeadp || pp->pt_outhit->hit_dist < 0.1 )
		{
		if( rt_g.debug & DEBUG_SHADOW )
			rt_log( "partition behind ray origin, no shadow\n" );
		return	ap->a_miss( ap );
		}
	if( rt_g.debug & DEBUG_SHADOW )
		{	register struct hit	*ihitp, *ohitp;
			register struct soltab	*istp, *ostp;
		rt_log( "Shadowed by :\n" );
		istp = pp->pt_inseg->seg_stp;
		ihitp = pp->pt_inhit;
		ostp = pp->pt_outseg->seg_stp;
		ohitp = pp->pt_outhit;
		RT_HIT_NORM( ihitp, istp, &(ap->a_ray) );
		RT_HIT_NORM( ohitp, ostp, &(ap->a_ray) );
		V_Print( "entry normal", ihitp->hit_normal, rt_log );
		V_Print( "exit normal", ohitp->hit_normal, rt_log );
		V_Print( "entry point", ihitp->hit_point, rt_log );
		V_Print( "exit point", ohitp->hit_point, rt_log );
		rt_log( "partition[start %g end %g]\n",
			ihitp->hit_dist, ohitp->hit_dist
			);
		rt_log( "solid name (%s)\n", pp->pt_inseg->seg_stp->st_name );
		}
	ap->a_diverge = 1.0;
	if( pp->pt_inseg->seg_stp == lgts[ap->a_user].stp )
		{ /* Have hit the EXPLICIT light source, no shadow.	*/
		if( rt_g.debug & DEBUG_SHADOW )
			rt_log( "Unobstructed path to explicit light.\n" );
		return	ap->a_miss( ap );
		}
	for( ; pp != PartHeadp; pp = pp->pt_forw )
		{
		if( pp->pt_inseg->seg_stp == lgts[ap->a_user].stp )
			/* Have hit the EXPLICIT light source.		*/
			break;
		entry = mat_Get_Db_Entry( (int)(pp->pt_regionp->reg_gmater) );
		if( (ap->a_diverge -= 1.0 - entry->transparency) <= 0.0 )
			/* Light is totally eclipsed.			*/
			{
			ap->a_diverge = 0.0;
			break;
			}
		}
	if( ap->a_diverge != 1.0 )
		/* Light source is obstructed, object shadowed.		*/
		{
		if( rt_g.debug & DEBUG_SHADOW )
			rt_log( "Lgt source obstructed, object shadowed\n" );
		hits_shadowed++;
		return	1;
		}
	else	/* Full intensity of light source.			*/
		{
		if( rt_g.debug & DEBUG_SHADOW )
			rt_log( "Full intensity of light source, no shadow\n" );
		return	ap->a_miss( ap );
		}
	}

/*	m o d e l _ R e f l e c t a n c e ( )
	This is the heart of the lighting model which is based on a model
	developed by Bui-Tuong Phong, [see Wm M. Newman and R. F. Sproull,
	"Principles of Interactive Computer Graphics", 	McGraw-Hill, 1979]
	
	Er = Ra(m)*cos(Ia) + Rd(m)*cos(Il) + W(Il,m)*cos(s)^^n
	where,
 
	Er	is the energy reflected in the observer's direction.
	Ra	is the diffuse reflectance coefficient at the point
		of intersection due to ambient lighting.
	Ia	is the angle of incidence associated with the ambient
		light source (angle between ray direction (negated) and
		surface normal).
	Rd	is the diffuse reflectance coefficient at the point
		of intersection due to lighting.
	Il	is the angle of incidence associated with the
		light source (angle between light source direction and
		surface normal).
	m	is the material identification code.
	W	is the specular reflectance coefficient,
		a function of the angle of incidence, range 0.0 to 1.0,
		for the material.
	s	is the angle between the reflected ray and the observer.
	n	'Shininess' of the material,  range 1 to 10.

	The RGB result is returned implicitly in "ap->a_color".
 */
_LOCAL_ void
model_Reflectance( ap, pp, mdb_entry, lgt_entry, view_dir )
register struct application	*ap;
struct partition		*pp;
Mat_Db_Entry			*mdb_entry;
register Lgt_Source		*lgt_entry;
fastf_t				*view_dir;
	{	/* Compute attenuation of light source intensity.	*/
		register fastf_t	*norml = pp->pt_inhit->hit_normal;
		register fastf_t	ff;		/* temp */
		fastf_t			lgt_energy;
		fastf_t			cos_il; /* Cos. incident angle.	*/
		auto fastf_t		lgt_dir[3];

	if( rt_g.debug & DEBUG_RGB )
		rt_log( "model_Reflectance()\n" );

	if( ap->a_user == 0 )		/* Ambient lighting.		*/
		{
		lgt_energy = lgt_entry->energy;
		VMOVE( lgt_dir, view_dir );
		}
	else
		{	
		/* Compute attenuated light intensity due to shadowing.	*/
		if( (lgt_energy = correct_Lgt( ap, pp, lgt_entry )) == 0.0 )
			{
			/* Shadowed by an opaque object.		*/
			ap->a_color[0] = ap->a_color[1] = ap->a_color[2] = 0.0;
			return;
			}
		/* Direction unit vector to light source from hit pt.	*/
		Diff2Vec( lgt_entry->loc, pp->pt_inhit->hit_point, lgt_dir );
		VUNITIZE( lgt_dir );
		}

	/* Calculate diffuse reflectance from light source.		*/
	if( (cos_il = Dot( norml, lgt_dir )) < 0.0 )
		cos_il = 0.0;
	/* Facter in light source intensity and diffuse weighting.	*/
	ff = cos_il * lgt_energy * mdb_entry->wgt_diffuse;
	/* Facter in light source color.				*/
	Scale2Vec( lgt_entry->coef, ff, ap->a_color );
	if( rt_g.debug & DEBUG_RGB )
		{
		V_Print( "norml", norml, rt_log );
		V_Print( "lgt_dir", lgt_dir, rt_log );
		rt_log( "cos_il=%g\n", cos_il );
		rt_log( "lgt_energy=%g\n", lgt_energy );
		rt_log( "wgt_diffuse=%g\n", mdb_entry->wgt_diffuse );
		V_Print( "diffuse reflectance", ap->a_color, rt_log );
		}
	/* Facter in material color (diffuse reflectance coeffs)	*/
	ff = RGB_INVERSE; /* Scale RGB values to coeffs (0.0 .. 1.0 )	*/
	ap->a_color[0] *= mdb_entry->df_rgb[0] * ff;
	ap->a_color[1] *= mdb_entry->df_rgb[1] * ff;
	ap->a_color[2] *= mdb_entry->df_rgb[2] * ff;

	if( ap->a_user != 0 )
		/* Calculate specular reflectance, if not ambient light.
		 	Reflected ray = (2 * cos(i) * Normal) - Incident ray.
		 	Cos(s) = dot product of Reflected ray with Incident ray.
		 */
		{	auto fastf_t		lgt_reflect[3], tmp_dir[3];
			register fastf_t	specular;
			fastf_t			cos_s;
		ff = 2 * cos_il;
		Scale2Vec( norml, ff, tmp_dir );
		Diff2Vec( tmp_dir, lgt_dir, lgt_reflect );
		if( rt_g.debug & DEBUG_RGB )
			{
			V_Print( "view_dir", view_dir, rt_log );
			V_Print( "lgt_reflect", lgt_reflect, rt_log );
			}
		if(	(cos_s = Dot( view_dir, lgt_reflect )) > 0.0
		    &&	cos_s <= 1.0
			)
			{ /* We have a significant specular component.	*/
			specular = mdb_entry->wgt_specular * lgt_energy *
					ipow( cos_s, mdb_entry->shine );
			/* Add specular component.			*/
			VJOIN1( ap->a_color, ap->a_color, specular, lgt_entry->coef );
			if( rt_g.debug & DEBUG_RGB )
				{
				rt_log( "cos_s=%g\n", cos_s );
				rt_log( "specular=%g\n", specular );
				V_Print( "w/ specular", ap->a_color, rt_log );
				}
			}
		else
		if( cos_s > 1.0 )
			{
			rt_log( "model_Reflectance() : cos(s) > 1.0 (%g)!\n", cos_s );
			V_Print( "norml", norml, rt_log );
			}
		}
	return;
	}

/*	c o n s _ V e c t o r ( )
	Construct a direction vector out of azimuth and elevation angles
	in radians, allocating storage for it and returning its address.
 */
void
cons_Vector( vec, azim, elev )
register fastf_t	*vec;
fastf_t	azim, elev;
	{ /* Store cosine of the elevation to save calculating twice.	*/
		fastf_t	cosE;
	cosE = cos( elev );
	vec[0] = cos( azim ) * cosE;
	vec[1] = sin( azim ) * cosE;
	vec[2] = sin( elev );
	return;
	}

/*	a b o r t _ R T ( )						*/
#if defined( BSD ) || defined( sgi )
int
#else
/*ARGSUSED*/
void
#endif
abort_RT( sig )
int	sig;
	{
	(void) signal( SIGINT, abort_RT );
	if( tty )
		{
		prnt_Event( "Aborted raytrace." );
		(void) fflush( stdout );
		}
	(void) fb_flush( fbiop );
	user_interrupt = 1;
#if defined( BSD )
	return	sig;
#else
	return;
#endif
	}

/*	i p o w ( )
	Integer exponent pow() function.
	Returns 'd' to the 'n'th power.
 */
_LOCAL_ fastf_t
ipow( d, n )
register fastf_t	d;
register int	n;
	{	register fastf_t	result = 1.0;
	if( d == 0.0 )
		return	0.0;
	while( n-- > 0 )
		result *= d;
	return	result;
	}

/*	v i e w _ p i x ( )						*/
_LOCAL_ void
view_pix( ap, scanp )
register struct application	*ap;
register RGBpixel		*scanp;
	{	RGBpixel	pixel;
		int		x = ap->a_x + x_fb_origin;
		int		y = ap->a_y + y_fb_origin;
	if( rt_g.debug && tty )
		{
		RES_ACQUIRE( &rt_g.res_malloc );
		prnt_Timer( (char *) NULL );
		(void) SetStandout();
		GRID_PIX_MOVE();
		(void) printf( " [%04d-", ap->a_x/aperture_sz );
		(void) ClrStandout();
		IDLE_MOVE();
		(void) fflush( stdout );
		RES_RELEASE( &rt_g.res_malloc );
		}
	/* Clip relative intensity on each gun to range 0.0 to 1.0;
		then scale to RGB values.				*/
	pixel[RED] = ap->a_color[0] > 1.0 ? 255 :
			(ap->a_color[0] < 0.0 ? 0 : ap->a_color[0] * 255);
	pixel[GRN] = ap->a_color[1] > 1.0 ? 255 :
			(ap->a_color[1] < 0.0 ? 0 : ap->a_color[1] * 255);
	pixel[BLU] = ap->a_color[2] > 1.0 ? 255 :
			(ap->a_color[2] < 0.0 ? 0 : ap->a_color[2] * 255);
	if( anti_aliasing )
		{
		x = ap->a_x / aperture_sz + x_fb_origin;
		y = ap->a_y / aperture_sz + y_fb_origin;
		pixel[RED] = (int)((fastf_t) pixel[RED] / sample_sz);
		pixel[GRN] = (int)((fastf_t) pixel[GRN] / sample_sz);
		pixel[BLU] = (int)((fastf_t) pixel[BLU] / sample_sz);
		if( ap->a_x % aperture_sz || ap->a_y % aperture_sz )
			{	RGBpixel	tpixel;
			/* Read accumulator pixel per buffering scheme.	*/
			switch( pix_buffered )
				{
			case B_PIO :
				if( fb_read( fbiop, x, y, tpixel, 1 ) == -1 )
					{
					rt_log( "Read failed from pixel <%d,%d>.\n",
						x, y
						);
					return;
					}
				break;
			case B_PAGE :
				if(	fb_seek( fbiop, x, y ) == -1
				    ||	fb_rpixel( fbiop, tpixel ) == -1
					)
					{
					rt_log( "Read failed from pixel <%d,%d>.\n",
						x, y
						);
					return;
					}
				break;
			case B_LINE :
				COPYRGB( tpixel, *scanp );
				break;
			default :
				rt_log( "unknown buffering scheme %d\n",
					pix_buffered );
				return;
				}
			/* Add current RGB values to accumulator pixel.	*/
			pixel[RED] += tpixel[RED];
			pixel[GRN] += tpixel[GRN];
			pixel[BLU] += tpixel[BLU];
			}
		}
	/* Write out pixel, depending on buffering scheme.		*/
	switch( pix_buffered )
		{
	case B_PIO :
		if( fb_write( fbiop, x, y, pixel, 1 ) != -1 )
			/* Programmed I/O to frame buffer (if possible).*/
			return;
		break;
	case B_PAGE :
		if(	fb_seek( fbiop, x, y ) != -1
		    &&	fb_wpixel( fbiop, pixel ) != -1
			)
			/* Buffered writes to frame buffer.		*/
			return;
		break;
	case B_LINE :
		COPYRGB( *scanp, pixel );
		return;
	default :
		rt_log( "unknown buffering scheme %d\n",
			pix_buffered );
		return;
		}
	rt_log( "Write failed to pixel <%d,%d>.\n", x, y );
	return;
	}

/*	v i e w _ e o l ( )						*/
_LOCAL_ void
view_eol( ap, scanbuf, scanp )
register struct application	*ap;
register RGBpixel		*scanbuf, *scanp;
	{	int	x = grid_x_org + x_fb_origin;
		int	y = ap->a_y/aperture_sz + y_fb_origin;
	if( tracking_cursor )
		{
		RES_ACQUIRE( &rt_g.res_stats );
		(void) fb_cursor( fbiop, 1, x_fb_origin, y );
		RES_RELEASE( &rt_g.res_stats );
		}
	/* Reset horizontal pixel position.				*/
	ap->a_x = grid_x_org * aperture_sz;

	if( tty )
		{
		RES_ACQUIRE( &rt_g.res_stats );
		prnt_Timer( (char *) NULL );
		(void) SetStandout();
		GRID_SCN_MOVE();
		(void) printf( "%04d-", ap->a_y/aperture_sz );
		GRID_PIX_MOVE();
		(void) printf( " [%04d-", ap->a_x/aperture_sz );
		(void) ClrStandout();
		IDLE_MOVE();
		(void) fflush( stdout );
		RES_RELEASE( &rt_g.res_stats );
		}
	else
		{	char	grid_y[5];
		RES_ACQUIRE( &rt_g.res_stats );
		(void) sprintf( grid_y, "%04d", ap->a_y/aperture_sz );
		prnt_Timer( grid_y );
		RES_RELEASE( &rt_g.res_stats );
		}
	if( pix_buffered == B_LINE )
		{
		RES_ACQUIRE( &rt_g.res_stats );
		if( fb_write( fbiop, x, y, scanbuf, scanp-scanbuf+1 ) == -1 )
			rt_log( "Write of scan line (%d) failed.\n", ap->a_y );
		RES_RELEASE( &rt_g.res_stats );
		}
	return;
	}

/*	v i e w _ e n d ( )						*/
_LOCAL_ void
view_end()
	{
	if( pix_buffered == B_PAGE )
		fb_flush( fbiop );
	if( rt_g.debug & DEBUG_REFRACT )
		rt_log( "Refraction stats : hits=%d misses=%d inside=%d total=%d\n",
			refract_total-(refract_missed+refract_inside),
			refract_missed, refract_inside, refract_total
			);
	if( rt_g.debug & DEBUG_SHADOW )
		rt_log( "Shadowing stats : lit=%d shadowed=%d total=%d\n",
			hits_lit, hits_shadowed, hits_lit+hits_shadowed
			);
	RES_ACQUIRE( &rt_g.res_malloc );
	prnt_Timer( "VIEW" );
	RES_RELEASE( &rt_g.res_malloc );
	return;
	}

#define	SIGMA	1/(sqrt(2)*1.13794)
/*	g a u s s_ w g t _ f u n c ( ) by Douglas A. Gwyn
	Gaussian weighting function.

	r = distance from center of beam
	B = beam finite radius (contains 90% of the total energy)

	R = r / B	definition

	I = intensity of beam at desired position
	  =  e^(- R^2 / log10(e)) / (log10(e) * Pi)

	Note that the intensity at the center of the beam is not 1 but
	rather log(10)/Pi.  This is because the intensity is normalized
	so that its integral over the infinite plane is 1.
 */
fastf_t
gauss_Wgt_Func( R )
fastf_t	R;
	{
	return	exp( - Sqr( R ) / LOG10E ) / (LOG10E * PI);
	}
