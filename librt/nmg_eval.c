/*
 *			N M G _ E V A L . C
 *
 *	Evaluate boolean operations on NMG objects
 *
 *  Authors -
 *	Michael John Muuss
 *	Lee A. Butler
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSnmg_eval[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"
#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "rtlist.h"
#include "nmg.h"
#include "raytrace.h"

struct nmg_bool_state  {
	struct shell	*bs_dest;
	struct shell	*bs_src;
	int		bs_isA;		/* true if A, else doing B */
	long		**bs_classtab;
	CONST int	*bs_actions;
	CONST struct rt_tol	*bs_tol;
};

static void nmg_eval_shell RT_ARGS( (struct shell *s,
		struct nmg_bool_state *bs));
static void nmg_eval_plot RT_ARGS( (struct nmg_bool_state *bs,
		int num, int delay));


#define BACTION_KILL			1
#define BACTION_RETAIN			2
#define BACTION_RETAIN_AND_FLIP		3

static CONST char	*nmg_baction_names[] = {
	"*undefined 0*",
	"BACTION_KILL",
	"BACTION_RETAIN",
	"BACTION_RETAIN_AND_FLIP",
	"*undefined 4*"
};

#define NMG_CLASS_BAD		8
static CONST char	*nmg_class_names[] = {
	"onAinB",
	"onAonBshared",
	"onAonBanti",
	"onAoutB",
	"inAonB",
	"onAonBshared",
	"onAonBanti",
	"outAonB",
	"*BAD*CLASS*"
};

/*
 *			N M G _ C K _ L U _ O R I E N T A T I O N
 *
 *  Make sure that the lu and fu orientation flags are consistent with
 *  the geometric arrangement of the vertices and the faceuse normal.
 */
void
nmg_ck_lu_orientation( lu, tolp )
struct loopuse		*lu;
CONST struct rt_tol	*tolp;
{
	struct rt_tol	tol;
	int		ccw;
	struct faceuse	*fu;
	plane_t		peqn;

	NMG_CK_LOOPUSE(lu);
	fu = lu->up.fu_p;		/* parent had better be faceuse */
	NMG_CK_FACEUSE(fu);

	NMG_GET_FU_PLANE( peqn, fu );

	if( tolp )  {
		RT_CK_TOL(tolp);
		tol = *tolp;		/* struct copy */
	} else {
		/* Build something suitable, when no user tol is handy */
		tol.magic = RT_TOL_MAGIC;
		tol.dist = 0.005;
		tol.dist_sq = tol.dist * tol.dist;
		tol.perp = 0;
		tol.para = 1;
	}
	ccw = nmg_loop_is_ccw(lu, peqn, &tol);
	if (rt_g.NMG_debug & DEBUG_BOOLEVAL)  {
		rt_log("nmg_ck_lu_orientation(x%x) ccw=%d, fu=%s, lu=%s\n",
			lu,
			ccw,
			nmg_orientation(fu->orientation),
			nmg_orientation(lu->orientation)
		);
	}
	if( ccw == 0 )
		return;		/* can't determine geometric orientation */


	if( ((fu->orientation == OT_SAME) == (lu->orientation == OT_SAME) ) &&
	     ccw != 1 )  {
		rt_log("nmg_ck_lu_orientation() lu=x%x, ccw=%d, fu_orient=%s, lu_orient=%s\n", lu,
			ccw,
			nmg_orientation(fu->orientation),
			nmg_orientation(lu->orientation)
		);
	     	rt_bomb("nmg_ck_lu_orientation() loop orientation flags do not match geometry\n");
	}
}


/*
 *			N M G _ C L A S S _ N A M E
 *
 *  Convert an NMG_CLASS_xxx token into a string name.
 */
CONST char *
nmg_class_name(class)
int	class;
{
	if( class < 0 || class > NMG_CLASS_BAD )  class = NMG_CLASS_BAD;
	return nmg_class_names[class];
}

/*
 *		Action Table for Boolean Operations.
 *
 *  Each table lists what actions are to be taken for topological elements
 *  which have have each kind of classification.
 *
 *  Actions are listed in this order:
 *	(Aon)	onAinB, onAonBshared, onAonBanti-shared, onAoutB,
 *	(Bon)	inAonB, onAonBshared, onAonBanti-shared, outAonB
 */
static CONST int		subtraction_actions[8] = {
	BACTION_KILL,
	BACTION_KILL,		/* shared */
	BACTION_RETAIN,		/* anti-shared */
	BACTION_RETAIN,

	BACTION_RETAIN,		/* (formerly BACTION_RETAIN_AND_FLIP) */
	BACTION_KILL,
	BACTION_KILL,
	BACTION_KILL
};

static CONST int		union_actions[8] = {
	BACTION_KILL,
	BACTION_RETAIN,		/* shared */
	BACTION_KILL,		/* anti-shared */
	BACTION_RETAIN,

	BACTION_KILL,
	BACTION_KILL,
	BACTION_KILL,
	BACTION_RETAIN
};

static CONST int		intersect_actions[8] = {
	BACTION_RETAIN,
	BACTION_RETAIN,		/* shared */
	BACTION_RETAIN,		/* anti-shared ==> non-manifold result */
	BACTION_KILL,

	BACTION_RETAIN,
	BACTION_KILL,
	BACTION_KILL,
	BACTION_KILL
};

/*
 *			N M G _ E V A L U A T E _ B O O L E A N
 *
 *  Evaluate a boolean operation on the two shells "A" and "B",
 *  of the form "answer = A op B".
 *  As input, each element (loop-in-face, wire loop, wire edge, vertex)
 *  in both A and B has been classified as being
 *  "in", "on", or "out" of the other shell.
 *  Using these classifications, operate on the input shells.
 *  At the end, shell A contains the resultant object, and
 *  shell B is destroyed.
 *
 */
void
nmg_evaluate_boolean( sA, sB, op, classlist, tol )
struct shell	*sA;
struct shell	*sB;
int		op;
long		*classlist[8];
CONST struct rt_tol	*tol;
{
	struct loopuse	*lu;
	struct faceuse	*fu;
	int CONST	*actions;
	int		i;
	struct nmg_bool_state	bool_state;

	NMG_CK_SHELL(sA);
	NMG_CK_SHELL(sB);
	RT_CK_TOL(tol);

	if (rt_g.NMG_debug & DEBUG_BOOLEVAL) {
		rt_log("nmg_evaluate_boolean(sA=x%x, sB=x%x, op=%d) START\n",
			sA, sB, op );
	}

	switch( op )  {
	case NMG_BOOL_SUB:
		actions = subtraction_actions;
		nmg_invert_shell(sB, tol);	/* FLIP all faceuse normals */
		break;
	case NMG_BOOL_ADD:
		actions = union_actions;
		break;
	case NMG_BOOL_ISECT:
		actions = intersect_actions;
		break;
	default:
		actions = union_actions;	/* shut up lint */
		rt_log("ERROR nmg_evaluate_boolean() op=%d.\n", op);
		rt_bomb("bad boolean\n");
	}

	bool_state.bs_dest = sA;
	bool_state.bs_src = sB;
	bool_state.bs_classtab = classlist;
	bool_state.bs_actions = actions;
	bool_state.bs_tol = tol;

	bool_state.bs_isA = 1;
	nmg_eval_shell( sA, &bool_state );

	bool_state.bs_isA = 0;
	nmg_eval_shell( sB, &bool_state );

	if (rt_g.NMG_debug & DEBUG_BOOLEVAL) {
		rt_log("nmg_evaluate_boolean(sA=x%x, sB=x%x, op=%d), evaluations done\n",
			sA, sB, op );
	}
	/* Write sA and sB into separate files, if wanted? */

	/* Move everything left in sB into sA.  sB is killed. */
	nmg_js( sA, sB, tol );

	/* Plot the result */
	if (rt_g.NMG_debug & DEBUG_BOOLEVAL && rt_g.NMG_debug & DEBUG_PLOTEM) {
		FILE	*fp;

		if ((fp=fopen("bool_ans.pl", "w")) == (FILE *)NULL) {
			(void)perror("bool_ans.pl");
			exit(-1);
		}
    		rt_log("plotting bool_ans.pl\n");
		nmg_pl_s( fp, sA );
		(void)fclose(fp);
	}

	/* Remove loops/edges/vertices that appear more than once in result */
	nmg_rm_redundancies( sA );

	if (rt_g.NMG_debug & DEBUG_BOOLEVAL) {
		rt_log("nmg_evaluate_boolean(sA=x%x, sB=x%x, op=%d) END\n",
			sA, sB, op );
	}
}

static int	nmg_eval_count = 0;	/* debug -- plot file numbering */

/*
 *			N M G _ E V A L _ S H E L L
 *
 *  Make a life-and-death decision on every element of a shell.
 *  Descend the "great chain of being" from the face to loop to edge
 *  to vertex, saving or demoting along the way.
 *
 *  Note that there is no moving of items from one shell to another.
 */
static void
nmg_eval_shell( s, bs )
register struct shell	*s;
struct nmg_bool_state *bs;
{
	struct faceuse	*fu;
	struct faceuse	*nextfu;
	struct loopuse	*lu;
	struct loopuse	*nextlu;
	struct edgeuse	*eu;
	struct edgeuse	*nexteu;
	struct vertexuse *vu;
	struct vertex	*v;
	int		loops_retained;
	plane_t		peqn;

	NMG_CK_SHELL(s);
	RT_CK_TOL(bs->bs_tol);

	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s->r_p->s_hd, s->r_p );

	/*
	 *  For each face in the shell, process all the loops in the face,
	 *  and then handle the face and all loops as a unit.
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	fu = RT_LIST_FIRST( faceuse, &s->fu_hd );
	while( RT_LIST_NOT_HEAD( fu, &s->fu_hd ) )  {
		NMG_CK_FACEUSE(fu);
		nextfu = RT_LIST_PNEXT( faceuse, fu );

		/* Faceuse mates will be handled at same time as OT_SAME fu */
		if( fu->orientation != OT_SAME )  {
			fu = nextfu;
			continue;
		}
		if( fu->fumate_p == nextfu )
			nextfu = RT_LIST_PNEXT( faceuse, nextfu );

		/* Consider this face */
		NMG_CK_FACE(fu->f_p);
		NMG_GET_FU_PLANE( peqn, fu );

		loops_retained = 0;
		lu = RT_LIST_FIRST( loopuse, &fu->lu_hd );
		while( RT_LIST_NOT_HEAD( lu, &fu->lu_hd ) )  {
			NMG_CK_LOOPUSE(lu);
			nextlu = RT_LIST_PNEXT( loopuse, lu );
			if( lu->lumate_p == nextlu )
				nextlu = RT_LIST_PNEXT( loopuse, nextlu );

			NMG_CK_LOOP( lu->l_p );
#if 0
			if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
#endif
			{
				nmg_ck_lu_orientation( lu, bs->bs_tol );
			}
			switch( nmg_eval_action( (genptr_t)lu->l_p, bs ) )  {
			case BACTION_KILL:
				/* Kill by demoting loop to edges */
				if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) == NMG_VERTEXUSE_MAGIC )  {
					/* loop of single vertex */
					(void)nmg_klu( lu );
				} else if( nmg_demote_lu( lu ) == 0 )  {
					nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
				}
				lu = nextlu;
				continue;
			case BACTION_RETAIN:
				loops_retained++;
				break;
			default:
				rt_bomb("nmg_eval_shell() bad BACTION\n");
			}
			lu = nextlu;
		}

		if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
			rt_log("faceuse x%x loops retained=%d\n",
				fu, loops_retained);
		if( rt_g.NMG_debug & DEBUG_VERIFY )
			nmg_vshell( &s->r_p->s_hd, s->r_p );

		/*
		 *  Here, faceuse will have 0 or more loopuses still in it.
		 *  Decide the fate of the face;  if the face dies,
		 *  then any remaining loops, edges, etc, will die too.
		 */
		if( RT_LIST_IS_EMPTY( &fu->lu_hd ) )  {
			if( loops_retained )  rt_bomb("nmg_eval_shell() empty faceuse with retained loops?\n");
			/* faceuse is empty, face & mate die */
			if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
		    		rt_log("faceuse x%x empty, kill\n", fu);
			nmg_kfu( fu );	/* kill face & mate, dequeue from shell */
			if( rt_g.NMG_debug & DEBUG_VERIFY )
				nmg_vshell( &s->r_p->s_hd, s->r_p );
			nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
			fu = nextfu;
			continue;
		}

		if( loops_retained <= 0 )  {
			nmg_pr_fu(fu, (char *)NULL);
			rt_bomb("nmg_eval_shell() non-empty faceuse, no loops retained?\n");
		}
		fu = nextfu;
	}
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s->r_p->s_hd, s->r_p );

	/*
	 *  For each loop in the shell, process.
	 *  Each loop is either a wire-loop, or a vertex-with-self-loop.
	 *  Only consider wire loops here.
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	lu = RT_LIST_FIRST( loopuse, &s->lu_hd );
	while( RT_LIST_NOT_HEAD( lu, &s->lu_hd ) )  {
		NMG_CK_LOOPUSE(lu);
		nextlu = RT_LIST_PNEXT( loopuse, lu );
		if( lu->lumate_p == nextlu )
			nextlu = RT_LIST_PNEXT( loopuse, nextlu );

		if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) == NMG_VERTEXUSE_MAGIC )  {
			/* ignore vertex-with-self-loop */
			lu = nextlu;
			continue;
		}
		NMG_CK_LOOP( lu->l_p );
		switch( nmg_eval_action( (genptr_t)lu->l_p, bs ) )  {
		case BACTION_KILL:
			/* Demote the loopuse into wire edges */
			/* kill loop & mate */
			if( nmg_demote_lu( lu ) == 0 )
				nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
			lu = nextlu;
			continue;
		case BACTION_RETAIN:
			break;
		default:
			rt_bomb("nmg_eval_shell() bad BACTION\n");
		}
		lu = nextlu;
	}
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s->r_p->s_hd, s->r_p );

	/*
	 *  For each wire-edge in the shell, ...
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	eu = RT_LIST_FIRST( edgeuse, &s->eu_hd );
	while( RT_LIST_NOT_HEAD( eu, &s->eu_hd ) )  {
		NMG_CK_EDGEUSE(eu);
		nexteu = RT_LIST_PNEXT( edgeuse, eu );	/* may be head */
		if( eu->eumate_p == nexteu )
			nexteu = RT_LIST_PNEXT( edgeuse, nexteu );

		/* Consider this edge */
		NMG_CK_EDGE( eu->e_p );
		switch( nmg_eval_action( (genptr_t)eu->e_p, bs ) )  {
		case BACTION_KILL:
			/* Demote the edegeuse (and mate) into vertices */
			if( nmg_demote_eu( eu ) == 0 )
				nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
			eu = nexteu;
			continue;
		case BACTION_RETAIN:
			break;
		default:
			rt_bomb("nmg_eval_shell() bad BACTION\n");
		}
		eu = nexteu;
	}

	/*
	 *  For each lone vertex-with-self-loop, process.
	 *  Note that these are intermixed in the loop list.
	 *  Each loop is either a wire-loop, or a vertex-with-self-loop.
	 *  Only consider cases of vertex-with-self-loop here.
	 *
	 *  This case has to be handled separately, because a wire-loop
	 *  may be demoted to a set of wire-edges above, some of which
	 *  may be retained.  The non-retained wire-edges may in turn
	 *  be demoted into vertex-with-self-loop objects above,
	 *  which will be processed here.
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	lu = RT_LIST_FIRST( loopuse, &s->lu_hd );
	while( RT_LIST_NOT_HEAD( lu, &s->lu_hd ) )  {
		NMG_CK_LOOPUSE(lu);
		nextlu = RT_LIST_PNEXT( loopuse, lu );

		if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_VERTEXUSE_MAGIC )  {
			/* ignore any remaining wire-loops */
			lu = nextlu;
			continue;
		}
		if( nextlu == lu->lumate_p )
			nextlu = RT_LIST_PNEXT(loopuse, nextlu);
		vu = RT_LIST_PNEXT( vertexuse, &lu->down_hd );
		NMG_CK_VERTEXUSE( vu );
		NMG_CK_VERTEX( vu->v_p );
		switch( nmg_eval_action( (genptr_t)vu->v_p, bs ) )  {
		case BACTION_KILL:
			/* Eliminate the loopuse, and mate */
			nmg_klu( lu );
			lu = nextlu;
			continue;
		case BACTION_RETAIN:
			break;
		default:
			rt_bomb("nmg_eval_shell() bad BACTION\n");
		}
		lu = nextlu;
	}
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s->r_p->s_hd, s->r_p );

	/*
	 * Final case:  shell of a single vertexuse
	 */
	if( vu = s->vu_p )  {
		NMG_CK_VERTEXUSE( vu );
		NMG_CK_VERTEX( vu->v_p );
		switch( nmg_eval_action( (genptr_t)vu->v_p, bs ) )  {
		case BACTION_KILL:
			nmg_kvu( vu );
			nmg_eval_plot( bs, nmg_eval_count++, 0 );	/* debug */
			s->vu_p = (struct vertexuse *)0;	/* sanity */
			break;
		case BACTION_RETAIN:
			break;
		default:
			rt_bomb("nmg_eval_shell() bad BACTION\n");
		}
	}
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s->r_p->s_hd, s->r_p );
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
}

/*
 *			N M G _ E V A L _ A C T I O N
 *
 *  Given a pointer to some NMG data structure,
 *  search the 4 classification lists to determine it's classification.
 *  (XXX In the future, this should be done with one big array).
 *  Then, return the action code for an item of that classification.
 */
int
nmg_eval_action( ptr, bs )
long				*ptr;
register struct nmg_bool_state	*bs;
{
	register int	ret;
	register int	class;
	int		index;

	RT_CK_TOL(bs->bs_tol);

	index = nmg_index_of_struct(ptr);
	if( bs->bs_isA )  {
		if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_AinB], index) )  {
			class = NMG_CLASS_AinB;
			ret = bs->bs_actions[NMG_CLASS_AinB];
			goto out;
		}
		if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_AonBshared], index) )  {
			class = NMG_CLASS_AonBshared;
			ret = bs->bs_actions[NMG_CLASS_AonBshared];
			goto out;
		}
		if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_AonBanti], index) )  {
			class = NMG_CLASS_AonBanti;
			ret = bs->bs_actions[NMG_CLASS_AonBanti];
			goto out;
		}
		if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_AoutB], index) )  {
			class = NMG_CLASS_AoutB;
			ret = bs->bs_actions[NMG_CLASS_AoutB];
			goto out;
		}
		rt_log("nmg_eval_action(ptr=x%x) %s has no A classification, retaining\n",
			ptr, rt_identify_magic( *((long *)ptr) ) );
		class = NMG_CLASS_BAD;
		ret = BACTION_RETAIN;
		goto out;
	}

	/* is B */
	if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_BinA], index) )  {
		class = NMG_CLASS_BinA;
		ret = bs->bs_actions[NMG_CLASS_BinA];
		goto out;
	}
	if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_BonAshared], index) )  {
		class = NMG_CLASS_BonAshared;
		ret = bs->bs_actions[NMG_CLASS_BonAshared];
		goto out;
	}
	if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_BonAanti], index) )  {
		class = NMG_CLASS_BonAanti;
		ret = bs->bs_actions[NMG_CLASS_BonAanti];
		goto out;
	}
	if( NMG_INDEX_VALUE(bs->bs_classtab[NMG_CLASS_BoutA], index) )  {
		class = NMG_CLASS_BoutA;
		ret = bs->bs_actions[NMG_CLASS_BoutA];
		goto out;
	}
	rt_log("nmg_eval_action(ptr=x%x) %s has no B classification, retaining\n",
		ptr, rt_identify_magic( *((long *)ptr) ) );
	class = NMG_CLASS_BAD;
	ret = BACTION_RETAIN;
out:
	if (rt_g.NMG_debug & DEBUG_BOOLEVAL) {
		rt_log("nmg_eval_action(ptr=x%x) index=%d %s %s %s %s\n",
			ptr, index,
			bs->bs_isA ? "A" : "B",
			rt_identify_magic( *((long *)ptr) ),
			nmg_class_names[class],
			nmg_baction_names[ret] );
	}
	return(ret);
}

/*
 *			N M G _ E V A L _ P L O T
 *
 *  Called from nmg_eval_shell
 *
 *  Located here because definition of nmg_bool_state is local to this module.
 */
static void
nmg_eval_plot( bs, num, delay )
struct nmg_bool_state	*bs;
int		num;
int		delay;
{
	FILE	*fp;
	char	fname[128];
	struct faceuse	*fu;
	int	do_plot = 0;
	int	do_anim = 0;

	if (rt_g.NMG_debug & DEBUG_BOOLEVAL && rt_g.NMG_debug & DEBUG_PLOTEM)
		do_plot = 1;
	if( rt_g.NMG_debug & DEBUG_PL_ANIM )  do_anim = 1;

	if( !do_plot && !do_anim )  return;

	RT_CK_TOL(bs->bs_tol);

	if( do_plot )  {
		sprintf(fname, "nmg_eval%d.pl", num);
		if( (fp = fopen(fname,"w")) == NULL )  {
			perror(fname);
			return;
		}
		rt_log("Plotting %s\n", fname);

		nmg_pl_s( fp, bs->bs_dest );
		nmg_pl_s( fp, bs->bs_src );

		fclose(fp);
	}

	if( do_anim )  {
		extern void (*nmg_vlblock_anim_upcall)();
		struct rt_vlblock	*vbp;

		vbp = rt_vlblock_init();

		nmg_vlblock_s( vbp, bs->bs_dest, 0 );
		nmg_vlblock_s( vbp, bs->bs_src, 0 );

		/* Cause animation of boolean operation as it proceeds! */
		if( nmg_vlblock_anim_upcall )  {
			/* if requested, delay 1/4 second */
			(*nmg_vlblock_anim_upcall)( vbp,
				(rt_g.NMG_debug&DEBUG_PL_SLOW) ? 250000 : 0,
				0 );
		} else {
			rt_log("null nmg_vlblock_anim_upcall, no animation\n");
		}
		rt_vlblock_free(vbp);
	}
}
