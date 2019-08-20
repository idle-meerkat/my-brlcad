/*                        C D T . C P P
 * BRL-CAD
 *
 * Copyright (c) 2007-2019 United States Government as represented by
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
/** @addtogroup libbrep */
/** @{ */
/** @file cdt.cpp
 *
 * Constrained Delaunay Triangulation of NURBS B-Rep objects.
 *
 */

#include "common.h"
#include "bg/chull.h"
#include "./cdt.h"

#define BREP_PLANAR_TOL 0.05
#define MAX_TRIANGULATION_ATTEMPTS 5

static int
full_retriangulation(struct ON_Brep_CDT_Face_State *f)
{

    // TODO - for now, don't reset before returning the error - we want to see the
    // failed mesh for debugging
    ON_Brep_CDT_Face_Reset(f, 0);

    p2t::CDT *cdt = NULL;
    bool outer = build_poly2tri_polylines(f, &cdt, 0);
    if (outer) {
	std::cerr << "Error: Face(" << f->ind << ") cannot evaluate its outer loop and will not be facetized." << std::endl;
	return -1;
    }

    std::set<ON_2dPoint *>::iterator p_it;
    for (p_it = f->on_surf_points->begin(); p_it != f->on_surf_points->end(); p_it++) {
	ON_2dPoint *p = *p_it;
	p2t::Point *tp = new p2t::Point(p->x, p->y);
	(*f->p2t_to_on2_map)[tp] = p;
	cdt->AddPoint(tp);
    }

    // All preliminary steps are complete, perform the triangulation using
    // Poly2Tri's triangulation.  NOTE: it is important that the inputs to
    // Poly2Tri satisfy its constraints - failure here could cause a crash.
    cdt->Triangulate(true, -1);

    // Copy triangles to set
    std::vector<p2t::Triangle*> cdt_tris = cdt->GetTriangles();
    for (size_t i = 0; i < cdt_tris.size(); i++) {
	p2t::Triangle *t = cdt_tris[i];
	f->tris->insert(new p2t::Triangle(*t));
    }
    delete cdt;

    populate_3d_pnts(f);

    // Validate that all points have a corresponding 3D point
    std::set<p2t::Triangle*>::iterator  s_it;
    for (s_it = f->tris->begin(); s_it != f->tris->end(); s_it++) {
	p2t::Triangle *t = *s_it;
	for (size_t j = 0; j < 3; j++) {
	    p2t::Point *tp = t->GetPoint(j);
	    if (f->p2t_to_on3_map->find(tp) == f->p2t_to_on3_map->end()) {
		bu_log("Error - triangle created with invalid 3D mapping!\n");
	    }
	}
    }

    // Identify any singular points
    f->singularities->clear();
    if (f->has_singular_trims) {
	std::set<p2t::Triangle *>::iterator tr_it;
	for (tr_it = f->tris->begin(); tr_it != f->tris->end(); tr_it++) {
	    p2t::Triangle *t = *tr_it;
	    for (size_t j = 0; j < 3; j++) {
		ON_3dPoint *p3d = (*f->p2t_to_on3_map)[t->GetPoint(j)];
		if (f->s_cdt->singular_vert_to_norms->find(p3d) != f->s_cdt->singular_vert_to_norms->end()) {
		    f->singularities->insert(p3d);
		}
	    }
	}
    }

    return 0;
}

int
refine_triangulation(struct ON_Brep_CDT_Face_State *f, int cnt, int rebuild)
{
    std::set<p2t::Triangle *> active_tris;
    std::set<cdt_mesh::triangle_t> active_ctris;
    int ret = 0;

    if (cnt > MAX_TRIANGULATION_ATTEMPTS) {
	std::cerr << "Error: even after " << MAX_TRIANGULATION_ATTEMPTS << " iterations could not successfully refine triangulate face " << f->ind << " for solidity criteria\n";
	return 0;
    }

    // If a previous pass has made changes in which points are active in the
    // surface set, we need to rebuild the whole triangulation.

    if (rebuild) {
	ret = full_retriangulation(f);
	if (ret < 0) {
	    bu_log("Fatal failure attempting full retriangulation of face\n");
	    return -1;
	}
    }

    // Check the triangles around edges first - these may require
    // the removal of points from the surface set
    ret = triangles_check_edge_tris(f);
    if (ret) {
	bu_log("Pass %d: surface points removed, need full retriangulation\n", cnt);
	return refine_triangulation(f, cnt+1, 1);
    }

    // Now, the hard part - create local subsets, remesh them, and replace the original
    // triangles with the new ones.

    f->fmesh.set_brep_data(f->s_cdt->brep->m_F[f->ind].m_bRev, f->s_cdt->edge_pnts, &f->s_cdt->fedges, f->singularities, f->on3_to_norm_map);
    f->fmesh.f_id = f->ind;
    f->fmesh.build(f->tris, f->p2t_to_on3_map);

    ret = (f->fmesh.repair()) ? 1 : -1;
    if (ret == -1) {
	bu_log("Face %d: triangulation FAILED!\n", f->ind);
	return -1;
    }
    ret = (f->fmesh.valid()) ? 1 : -1;

    if (ret > 0) {
	bu_log("Face %d: successful triangulation after %d passes\n", f->ind, cnt);
    } else {
	bu_log("Face %d: triangulation produced invalid mesh!\n", f->ind);
    }
    return ret;
}

static int
do_triangulation(struct ON_Brep_CDT_Face_State *f)
{
    ON_Brep_CDT_Face_Reset(f, 1);

    // Build the polylines in the poly2tri data container
    p2t::CDT *cdt = NULL;
    bool outer = build_poly2tri_polylines(f, &cdt, 1);
    if (outer) {
	std::cerr << "Error: Face(" << f->ind << ") cannot evaluate its outer loop and will not be facetized." << std::endl;
	return -1;
    }

    // Sample the surface, independent of the trimming curves, to get points that
    // will tie the mesh to the interior surface.
    getSurfacePoints(f);
    std::set<ON_2dPoint *>::iterator p_it;
    for (p_it = f->on_surf_points->begin(); p_it != f->on_surf_points->end(); p_it++) {
	ON_2dPoint *p = *p_it;
	p2t::Point *tp = new p2t::Point(p->x, p->y);
	(*f->p2t_to_on2_map)[tp] = p;
	cdt->AddPoint(tp);
    }

    // All preliminary steps are complete, perform the initial triangulation
    // using Poly2Tri's triangulation.  NOTE: it is important that the inputs
    // to Poly2Tri satisfy its constraints - failure here could cause a crash.

    // TODO - wrap all this inside of a libbg routine that catches the crash and
    // bails out more gracefully...
    cdt->Triangulate(true, -1);

    // Copy generated triangles to set for easier manipulation
    std::vector<p2t::Triangle*> cdt_tris = cdt->GetTriangles();
    for (size_t i = 0; i < cdt_tris.size(); i++) {
	p2t::Triangle *t = cdt_tris[i];
	f->tris->insert(new p2t::Triangle(*t));
    }
    delete cdt;

    /* Calculate any 3D points we don't already have */
    populate_3d_pnts(f);


    // Identify any singular 3D points
    f->singularities->clear();
    if (f->has_singular_trims) {
	std::set<p2t::Triangle *>::iterator tr_it;
	for (tr_it = f->tris->begin(); tr_it != f->tris->end(); tr_it++) {
	    p2t::Triangle *t = *tr_it;
	    for (size_t j = 0; j < 3; j++) {
		ON_3dPoint *p3d = (*f->p2t_to_on3_map)[t->GetPoint(j)];
		if (f->s_cdt->singular_vert_to_norms->find(p3d) != f->s_cdt->singular_vert_to_norms->end()) {
		    f->singularities->insert(p3d);
		}
	    }
	}
    }

    /* The poly2tri triangulation is not guaranteed to have all the properties
     * we want out of the box - trigger a series of checks */
    return refine_triangulation(f, 0, 0);
}

static int
ON_Brep_CDT_Face(struct ON_Brep_CDT_Face_State *f)
{
    struct ON_Brep_CDT_State *s_cdt = f->s_cdt;
    int face_index = f->ind;
    ON_BrepFace &face = f->s_cdt->brep->m_F[face_index];
    const ON_Surface *s = face.SurfaceOf();
    int loop_cnt = face.LoopCount();
    ON_SimpleArray<BrepTrimPoint> *brep_loop_points = (f->face_loop_points) ? f->face_loop_points : new ON_SimpleArray<BrepTrimPoint>[loop_cnt];
    f->face_loop_points = brep_loop_points;

    // If this face is using at least one singular trim, set the flag
    for (int li = 0; li < loop_cnt; li++) {
	ON_BrepLoop *l = face.Loop(li);
	int trim_count = l->TrimCount();
	for (int lti = 0; lti < trim_count; lti++) {
	    ON_BrepTrim *trim = l->Trim(lti);
	    if (trim->m_type == ON_BrepTrim::singular) {
		f->has_singular_trims = 1;
		break;
	    }
	}
	if (f->has_singular_trims) {
	    break;
	}
    }


    // Use the edge curves and loops to generate an initial set of trim polygons.
    for (int li = 0; li < loop_cnt; li++) {
	Process_Loop_Edges(f, li);
    }

    // Handle a variety of situations that complicate loop handling on closed surfaces
    if (s->IsClosed(0) || s->IsClosed(1)) {
	bool verbose = false;
	PerformClosedSurfaceChecks(s_cdt, s, face, brep_loop_points, BREP_SAME_POINT_TOLERANCE, verbose);
    }

    // Find for this face, find the minimum and maximum edge polyline segment lengths
    (*s_cdt->max_edge_seg_len)[face.m_face_index] = 0.0;
    (*s_cdt->min_edge_seg_len)[face.m_face_index] = DBL_MAX;
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	if (num_loop_points > 1) {
	    ON_3dPoint *p1 = (brep_loop_points[li])[0].p3d;
	    ON_3dPoint *p2 = NULL;
	    for (int i = 1; i < num_loop_points; i++) {
		p2 = p1;
		p1 = (brep_loop_points[li])[i].p3d;
		fastf_t dist = p1->DistanceTo(*p2);
		if (dist > (*s_cdt->max_edge_seg_len)[face.m_face_index]) {
		    (*s_cdt->max_edge_seg_len)[face.m_face_index] = dist;
		}
		if ((dist > SMALL_FASTF) && (dist < (*s_cdt->min_edge_seg_len)[face.m_face_index]))  {
		    (*s_cdt->min_edge_seg_len)[face.m_face_index] = dist;
		}
	    }
	}
    }

    // Populate fedges
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	for (int i = 1; i < num_loop_points; i++) {
	    // map point to last entry to 3d point
	    f->s_cdt->fedges.insert(std::make_pair((brep_loop_points[li])[i - 1].p3d, (brep_loop_points[li])[i].p3d));
	}
	f->s_cdt->fedges.insert(std::make_pair((brep_loop_points[li])[num_loop_points-1].p3d, (brep_loop_points[li])[0].p3d));
    }

    // TODO - we may need to add 2D points on trims that the edges didn't know
    // about.  Since 3D points must be shared along edges and we're using
    // synchronized numbers of parametric domain ordered 2D and 3D points to
    // make that work, we will need to track new 2D points and update the
    // corresponding 3D edge based data structures.  More than that - we must
    // also update all 2D structures in all other faces associated with the
    // edge in question to keep things in overall sync.

    // TODO - if we are going to apply clipper boolean resolution to sets of
    // face loops, that would come here - once we have assembled the loop
    // polygons for the faces. That also has the potential to generate "new" 3D
    // points on edges that are driven by 2D boolean intersections between
    // trimming loops, and may require another update pass as above.

    return do_triangulation(f);
}

static ON_3dVector
calc_trim_vnorm(ON_BrepVertex& v, ON_BrepTrim *trim)
{
    ON_3dPoint t1, t2;
    ON_3dVector v1 = ON_3dVector::UnsetVector;
    ON_3dVector v2 = ON_3dVector::UnsetVector;
    ON_3dVector trim_norm = ON_3dVector::UnsetVector;

    ON_Interval trange = trim->Domain();
    ON_3dPoint t_2d1 = trim->PointAt(trange[0]);
    ON_3dPoint t_2d2 = trim->PointAt(trange[1]);

    ON_Plane fplane;
    const ON_Surface *s = trim->SurfaceOf();
    if (s->IsPlanar(&fplane, BREP_PLANAR_TOL)) {
	trim_norm = fplane.Normal();
	if (trim->Face()->m_bRev) {
	    trim_norm = trim_norm * -1;
	}
    } else {
	int ev1 = 0;
	int ev2 = 0;
	if (surface_EvNormal(s, t_2d1.x, t_2d1.y, t1, v1)) {
	    if (trim->Face()->m_bRev) {
		v1 = v1 * -1;
	    }
	    if (v.Point().DistanceTo(t1) < ON_ZERO_TOLERANCE) {
		ev1 = 1;
		trim_norm = v1;
	    }
	}
	if (surface_EvNormal(s, t_2d2.x, t_2d2.y, t2, v2)) {
	    if (trim->Face()->m_bRev) {
		v2 = v2 * -1;
	    }
	    if (v.Point().DistanceTo(t2) < ON_ZERO_TOLERANCE) {
		ev2 = 1;
		trim_norm = v2;
	    }
	}
	// If we got both of them, go with the closest one
	if (ev1 && ev2) {
#if 0
	    if (ON_DotProduct(v1, v2) < 0) {
		bu_log("Vertex %d(%f %f %f), trim %d: got both normals\n", v.m_vertex_index, v.Point().x, v.Point().y, v.Point().z, trim->m_trim_index);
		bu_log("v1(%f)(%f %f)(%f %f %f): %f %f %f\n", v.Point().DistanceTo(t1), t_2d1.x, t_2d1.y, t1.x, t1.y, t1.z, v1.x, v1.y, v1.z);
		bu_log("v2(%f)(%f %f)(%f %f %f): %f %f %f\n", v.Point().DistanceTo(t2), t_2d2.x, t_2d2.y, t2.x, t2.y, t2.z, v2.x, v2.y, v2.z);
	    }
#endif
	    trim_norm = (v.Point().DistanceTo(t1) < v.Point().DistanceTo(t2)) ? v1 : v2;
	}
    }

    return trim_norm;
}

static void
calc_singular_vert_norm(struct ON_Brep_CDT_State *s_cdt, int index)
{
    ON_BrepVertex& v = s_cdt->brep->m_V[index];
    int have_calculated = 0;
    ON_3dVector vnrml(0.0, 0.0, 0.0);

    if (s_cdt->singular_vert_to_norms->find((*s_cdt->vert_pnts)[index]) != (s_cdt->singular_vert_to_norms->end())) {
	// Already processed this one
	return;
    }

    //bu_log("Processing vert %d (%f %f %f)\n", index, v.Point().x, v.Point().y, v.Point().z);

    for (int eind = 0; eind != v.EdgeCount(); eind++) {
	ON_3dVector trim1_norm = ON_3dVector::UnsetVector;
	ON_3dVector trim2_norm = ON_3dVector::UnsetVector;
	ON_BrepEdge& edge = s_cdt->brep->m_E[v.m_ei[eind]];
	if (edge.TrimCount() != 2) {
	    // Don't know what to do with this yet... skip.
	    continue;
	}
	ON_BrepTrim *trim1 = edge.Trim(0);
	ON_BrepTrim *trim2 = edge.Trim(1);

	if (trim1->m_type != ON_BrepTrim::singular) {
	    trim1_norm = calc_trim_vnorm(v, trim1);
	}
	if (trim2->m_type != ON_BrepTrim::singular) {
	    trim2_norm = calc_trim_vnorm(v, trim2);
	}

	// If one of the normals is unset and the other comes from a plane, use it
	if (trim1_norm == ON_3dVector::UnsetVector && trim2_norm != ON_3dVector::UnsetVector) {
	    const ON_Surface *s2 = trim2->SurfaceOf();
	    if (!s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
		continue;
	    }
	    trim1_norm = trim2_norm;
	}
	if (trim1_norm != ON_3dVector::UnsetVector && trim2_norm == ON_3dVector::UnsetVector) {
	    const ON_Surface *s1 = trim1->SurfaceOf();
	    if (!s1->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
		continue;
	    }
	    trim2_norm = trim1_norm;
	}

	// If we have disagreeing normals and one of them is from a planar surface, go
	// with that one
	if (NEAR_EQUAL(ON_DotProduct(trim1_norm, trim2_norm), -1, VUNITIZE_TOL)) {
	    const ON_Surface *s1 = trim1->SurfaceOf();
	    const ON_Surface *s2 = trim2->SurfaceOf();
	    if (!s1->IsPlanar(NULL, ON_ZERO_TOLERANCE) && !s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
		// Normals severely disagree, no planar surface to fall back on - can't use this
		continue;
	    }
	    if (s1->IsPlanar(NULL, ON_ZERO_TOLERANCE) && s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
		// Two disagreeing planes - can't use this
		continue;
	    }
	    if (s1->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
		trim2_norm = trim1_norm;
	    }
	    if (s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
		trim1_norm = trim2_norm;
	    }
	}

	// Add the normals to the vnrml total
	vnrml += trim1_norm;
	vnrml += trim2_norm;
	have_calculated = 1;

    }
    if (!have_calculated) {
	return;
    }

    // Average all the successfully calculated normals into a new unit normal
    vnrml.Unitize();

    // We store this as a point to keep C++ happy...  If we try to
    // propagate the ON_3dVector type through all the CDT logic it
    // triggers issues with the compile.
    (*s_cdt->vert_avg_norms)[index] = new ON_3dPoint(vnrml);
    s_cdt->w3dnorms->push_back((*s_cdt->vert_avg_norms)[index]);

    // If we have a vertex normal, add it to the map which will allow us
    // to ascertain if a given point has such a normal.  This will allow
    // a point-based check even if we don't know a vertex index locally
    // in the code.
    (*s_cdt->singular_vert_to_norms)[(*s_cdt->vert_pnts)[index]] = (*s_cdt->vert_avg_norms)[index];

}

int
ON_Brep_CDT_Tessellate(struct ON_Brep_CDT_State *s_cdt, int face_cnt, int *faces)
{

    ON_wString wstr;
    ON_TextLog tl(wstr);

    // Check for any conditions that are show-stoppers
    ON_wString wonstr;
    ON_TextLog vout(wonstr);
    if (!s_cdt->orig_brep->IsValid(&vout)) {
	bu_log("brep is NOT valid, cannot produce watertight mesh\n");
	//return -1;
    }

    // For now, edges must have 2 and only 2 trims for this to work.
    for (int index = 0; index < s_cdt->orig_brep->m_E.Count(); index++) {
        ON_BrepEdge& edge = s_cdt->orig_brep->m_E[index];
        if (edge.TrimCount() != 2) {
	    bu_log("Edge %d trim count: %d - can't (yet) do watertight meshing\n", edge.m_edge_index, edge.TrimCount());
            return -1;
        }
    }

    // We may be changing the ON_Brep data, so work on a copy
    // rather than the original object
    if (!s_cdt->brep) {

	s_cdt->brep = new ON_Brep(*s_cdt->orig_brep);

	// Attempt to minimize situations where 2D and 3D distances get out of sync
	// by shrinking the surfaces down to the active area of the face
	s_cdt->brep->ShrinkSurfaces();

    }

    ON_Brep* brep = s_cdt->brep;

    // If this is the first time through, there are a number of once-per-conversion
    // operations to take care of.
    if (!s_cdt->w3dpnts->size()) {

	// Translate global relative tolerances into physical dimensions based
	// on the BRep bounding box
	cdt_tol_global_calc(s_cdt);

	// Reparameterize the face's surface and transform the "u" and "v"
	// coordinates of all the face's parameter space trimming curves to
	// minimize distortion in the map from parameter space to 3d.
	for (int face_index = 0; face_index < brep->m_F.Count(); face_index++) {
	    ON_BrepFace *face = brep->Face(face_index);
	    const ON_Surface *s = face->SurfaceOf();
	    double surface_width, surface_height;
	    if (s->GetSurfaceSize(&surface_width, &surface_height)) {
		face->SetDomain(0, 0.0, surface_width);
		face->SetDomain(1, 0.0, surface_height);
	    }
	}

	/* We want to use ON_3dPoint pointers and BrepVertex points, but
	 * vert->Point() produces a temporary address.  If this is our first time
	 * through, make stable copies of the Vertex points. */
	for (int index = 0; index < brep->m_V.Count(); index++) {
	    ON_BrepVertex& v = brep->m_V[index];
	    (*s_cdt->vert_pnts)[index] = new ON_3dPoint(v.Point());
	    CDT_Add3DPnt(s_cdt, (*s_cdt->vert_pnts)[index], -1, v.m_vertex_index, -1, -1, -1, -1);
	    // topologically, any vertex point will be on edges
	    s_cdt->edge_pnts->insert((*s_cdt->vert_pnts)[index]);
	}

	/* If this is the first time through, check for singular trims.  For
	 * vertices associated with such a trim get vertex normals that are the
	 * average of the surface normals at the junction from faces that don't
	 * use a singular trim to reference the vertex.
	 */
	for (int index = 0; index < brep->m_T.Count(); index++) {
	    ON_BrepTrim &trim = s_cdt->brep->m_T[index];
	    if (trim.m_type == ON_BrepTrim::singular) {
		ON_BrepVertex *v1 = trim.Vertex(0);
		ON_BrepVertex *v2 = trim.Vertex(1);
		calc_singular_vert_norm(s_cdt, v1->m_vertex_index);
		calc_singular_vert_norm(s_cdt, v2->m_vertex_index);
	    }
	}

#if 1
	// EXPERIMENT - see if we can generate polygons from the loops 
	// For all faces, and each face loop in those faces, build the
	// initial polygons strictly based on trim start/end points

	// First, set up the edge containers that will manage the edge subdivision.  Loop
	// ordering is not the job of these containers - that's handled by the trim loop
	// polygons.  These containers maintain the association between trims in different
	// faces and the 3D edge curve information used to drive shared points.
	for (int index = 0; index < brep->m_E.Count(); index++) {
	    ON_BrepEdge& edge = brep->m_E[index];
	    cdt_mesh::bedge_seg_t *bseg = new cdt_mesh::bedge_seg_t;
	    bseg->edge_ind = edge.m_edge_index;
	    s_cdt->e2polysegs[edge.m_edge_index].insert(bseg);
	}

	// Next, for each face and each loop in each face define the initial
	// loop polygons.  Note there is no splitting of edges at this point -
	// we are simply establishing the initial closed polygons.
	for (int face_index = 0; face_index < brep->m_F.Count(); face_index++) {
	    ON_BrepFace &face = s_cdt->brep->m_F[face_index];
	    int loop_cnt = face.LoopCount();
	    cdt_mesh::cdt_mesh_t *fmesh = &s_cdt->fmeshes[face_index];
	    cdt_mesh::cpolygon_t *cpoly = NULL;

	    for (int li = 0; li < loop_cnt; li++) {
		const ON_BrepLoop *loop = face.Loop(li);
		bool is_outer = (face.OuterLoop()->m_loop_index == loop->m_loop_index) ? true : false;
		if (is_outer) {
		    cpoly = &fmesh->outer_loop;
		} else {
		    cpoly = new cdt_mesh::cpolygon_t;
		    fmesh->inner_loops[li] = cpoly;
		}
		cpoly->cdt_mesh = fmesh;
		int trim_count = loop->TrimCount();

		ON_2dPoint cp(0,0);

		long cv, pv, fv;
		for (int lti = 0; lti < trim_count; lti++) {
		    ON_BrepTrim *trim = loop->Trim(lti);
		    ON_Interval range = trim->Domain();
		    if (lti == 0) {
			// Polygon first
			cp = trim->PointAt(range.m_t[0]);
			pv = cpoly->add_point(cp);
			ON_3dPoint *op3d = (*s_cdt->vert_pnts)[trim->Vertex(0)->m_vertex_index];
			cpoly->add_point(op3d);
			fv = pv;

			// Let cdt_mesh know about new information
			ON_3dVector norm = ON_3dVector::UnsetVector;
			if (trim->m_type != ON_BrepTrim::singular) {
			    ON_3dPoint tmp1;
			    surface_EvNormal(trim->SurfaceOf(), cp.x, cp.y, tmp1, norm);
			}
			fmesh->add_point(cp);
			fmesh->add_point(op3d);
			fmesh->add_normal(new ON_3dPoint(norm));

		    } else {
			pv = cv;
		    }

		    // NOTE: Singularities have a segment in 2D but not 3D - we're adding extra copies of pointers to
		    // points in the arrays to deal with this non-uniqueness to keep a 1-1 relationship
		    // between the two array indices in the polygon.  For the 3D p2ind mapping, this will mean that the
		    // ON_3dPoint pointer will always point to the highest index value in the vector
		    // to be assigned that particular pointer.  For tests which are concerned with 3D point
		    // uniqueness, a 2d->ind->3d->ind lookup will be needed to "canonicalize"
		    // the 3D index value.  (TODO In particular, this will be needed for triangle
		    // comparisons.)
		    //
		    //
		    cp = trim->PointAt(range.m_t[1]);
		    cv = cpoly->add_point(cp);
		    ON_3dPoint *cp3d = (*s_cdt->vert_pnts)[trim->Vertex(1)->m_vertex_index];
		    cpoly->add_point(cp3d);

		    // Let cdt_mesh know about new information
		    ON_3dVector norm = ON_3dVector::UnsetVector;
		    if (trim->m_type != ON_BrepTrim::singular) {
			// 3D points are globally unique, but normals are not - the same edge point may
			// have different normals from two faces at a sharp edge.  Calculate the
			// face normal for this point on this surface.
			ON_3dPoint tmp1;
			surface_EvNormal(trim->SurfaceOf(), cp.x, cp.y, tmp1, norm);
		    }
		    long find = fmesh->add_point(cp);
		    fmesh->add_point(cp3d);
		    fmesh->add_normal(new ON_3dPoint(norm));

		    cpoly->p2f[cv] = find;

		    struct cdt_mesh::edge_t lseg(pv, cv);
		    cdt_mesh::cpolyedge_t *ne = cpoly->add_edge(lseg);
		    ne->trim_ind = trim->m_trim_index;
		    ne->trim_start = range.m_t[0];
		    ne->trim_end = range.m_t[1];
		    if (trim->m_ei >= 0) {
			cdt_mesh::bedge_seg_t *eseg = *s_cdt->e2polysegs[trim->m_ei].begin();
			if (eseg->tseg1 && eseg->tseg2) {
			    bu_log("error - more than two trims associated with an edge\n");
			    return -1;
			}
			if (eseg->tseg1) {
			    eseg->tseg2 = ne;
			} else {
			    eseg->tseg1 = ne;
			}
		    } else {
			// A null eseg will indicate a singularity and a need for special case
			// splitting of the 2D edge only
			ne->eseg = NULL;
		    }
		}
		struct cdt_mesh::edge_t last_seg(cv, fv);
		cpoly->add_edge(last_seg);
	    }
	}

	// Process the non-linear edges first - we will need information
	// from them to handle the linear edges
	for (int index = 0; index < brep->m_E.Count(); index++) {
	    ON_BrepEdge& edge = brep->m_E[index];

	    const ON_Curve* crv = edge.EdgeCurveOf();
	    if (!crv->IsLinear(BN_TOL_DIST)) {
		std::set<cdt_mesh::bedge_seg_t *> &epsegs = s_cdt->e2polysegs[edge.m_edge_index];
		std::set<cdt_mesh::bedge_seg_t *>::iterator e_it;
		for (e_it = epsegs.begin(); e_it != epsegs.end(); e_it++) {
		    cdt_mesh::bedge_seg_t *b = *e_it;
		    if (!b->tseg1 || !b->tseg2) {
			std::cout << "don't have trims\n";
		    }
		}
	    }
	}

	for (int face_index = 0; face_index < brep->m_F.Count(); face_index++) {
	    ON_BrepFace &face = s_cdt->brep->m_F[face_index];
	    int loop_cnt = face.LoopCount();
	    cdt_mesh::cdt_mesh_t *fmesh = &s_cdt->fmeshes[face_index];
	    cdt_mesh::cpolygon_t *cpoly = NULL;

	    for (int li = 0; li < loop_cnt; li++) {
		const ON_BrepLoop *loop = face.Loop(li);
		bool is_outer = (face.OuterLoop()->m_loop_index == loop->m_loop_index) ? true : false;
		if (is_outer) {
		    cpoly = &fmesh->outer_loop;
		} else {
		    cpoly = fmesh->inner_loops[li];
		}
		std::cout << "Face: " << face_index << ", Loop: " << li << "\n";
		cpoly->print();

		struct bu_vls fname = BU_VLS_INIT_ZERO;
		bu_vls_sprintf(&fname, "%d-%d-poly3d.plot3", face_index, li);
		cpoly->polygon_plot_3d(bu_vls_cstr(&fname));
		cpoly->cdt();
		bu_vls_sprintf(&fname, "%d-%d-cdt.plot3", face_index, li);
		cpoly->cdt_mesh->tris_set_plot(cpoly->tris, bu_vls_cstr(&fname));
		bu_vls_free(&fname);

	    }
	}
#endif


	/* To generate watertight meshes, the faces *must* share 3D edge points.  To ensure
	 * a uniform set of edge points, we first sample all the edges and build their
	 * point sets */

	Get_Edge_Points(s_cdt);

    } else {
	/* Clear the mesh state, if this container was previously used */
	s_cdt->tri_brep_face->clear();
    }

    // Process all of the faces we have been instructed to process, or (default) all faces.
    // Keep track of failures and successes.
    int face_failures = 0;
    int face_successes = 0;
    int fc = ((face_cnt == 0) || !faces) ? s_cdt->brep->m_F.Count() : face_cnt;
    for (int i = 0; i < fc; i++) {
	int fi = ((face_cnt == 0) || !faces) ? i : faces[i];
	if (fi < s_cdt->brep->m_F.Count()) {
	    if (s_cdt->faces->find(fi) == s_cdt->faces->end()) {
		struct ON_Brep_CDT_Face_State *f = ON_Brep_CDT_Face_Create(s_cdt, fi);
		(*s_cdt->faces)[fi] = f;
	    }
	    if (ON_Brep_CDT_Face((*s_cdt->faces)[fi])) {
		face_failures++;
	    } else {
		face_successes++;
	    }
	}
    }

    // If we only tessellated some of the faces, we don't have the
    // full solid mesh yet (by definition).  Return accordingly.
    if (face_failures || !face_successes || face_successes < s_cdt->brep->m_F.Count()) {
	return (face_successes) ? 1 : -1;
    }

    /* We've got face meshes and no reported failures - check to see if we have a
     * solid mesh */
    int valid_fcnt, valid_vcnt;
    int *valid_faces = NULL;
    fastf_t *valid_vertices = NULL;

    if (ON_Brep_CDT_Mesh(&valid_faces, &valid_fcnt, &valid_vertices, &valid_vcnt, NULL, NULL, NULL, NULL, s_cdt, 0, NULL) < 0) {
	return -1;
    }

    struct bg_trimesh_solid_errors se = BG_TRIMESH_SOLID_ERRORS_INIT_NULL;
    int invalid = bg_trimesh_solid2(valid_vcnt, valid_fcnt, valid_vertices, valid_faces, &se);

    if (invalid) {
	trimesh_error_report(s_cdt, valid_fcnt, valid_vcnt, valid_faces, valid_vertices, &se);
    }

    bu_free(valid_faces, "faces");
    bu_free(valid_vertices, "vertices");

    if (invalid) {
	return 1;
    }

    return 0;

}


// Generate a BoT with normals.
int
ON_Brep_CDT_Mesh(
	int **faces, int *fcnt,
	fastf_t **vertices, int *vcnt,
	int **face_normals, int *fn_cnt,
	fastf_t **normals, int *ncnt,
	struct ON_Brep_CDT_State *s_cdt,
	int exp_face_cnt, int *exp_faces)
{
    size_t triangle_cnt = 0;
    if (!faces || !fcnt || !vertices || !vcnt || !s_cdt) {
	return -1;
    }

    /* We can ignore the face normals if we want, but if some of the
     * return variables are non-NULL they all need to be non-NULL */
    if (face_normals || fn_cnt || normals || ncnt) {
	if (!face_normals || !fn_cnt || !normals || !ncnt) {
	    return -1;
	}
    }

    std::vector<int> active_faces;
    if (!exp_face_cnt || !exp_faces) {
	for (int face_index = 0; face_index < s_cdt->brep->m_F.Count(); face_index++) {
	    active_faces.push_back(face_index);
	}
    } else {
	for (int i = 0; i < exp_face_cnt; i++) {
	    active_faces.push_back(exp_faces[i]);
	}
    }

#if 0
    /* For the non-zero-area triangles sharing an edge with a non-trivially
     * degenerate zero area triangle, we need to build new polygons from each
     * triangle and the "orphaned" points along the edge(s).  We then
     * re-tessellate in the triangle's parametric space.
     *
     * An "involved" triangle is a triangle with two of its three points in the
     * face's degen_pnts set (i.e. it shares an edge with a non-trivially
     * degenerate zero-area triangle.)
     */
    for (int fi = 0; fi != s_cdt->brep->m_F.Count(); fi++) {
	struct ON_Brep_CDT_Face_State *f = (*s_cdt->faces)[fi];
	if (f) {
	    triangles_rebuild_involved(f);
	}
    }
#endif

    /* We know now the final triangle set.  We need to build up the set of
     * unique points and normals to generate a mesh containing only the
     * information actually used by the final triangle set. */
    std::set<ON_3dPoint *> vfpnts;
    std::set<ON_3dPoint *> vfnormals;
    for (size_t fi = 0; fi < active_faces.size(); fi++) {
	struct ON_Brep_CDT_Face_State *f = (*s_cdt->faces)[active_faces[fi]];
	if (!f) continue;
	std::map<ON_3dPoint *, ON_3dPoint *> *normalmap = f->on3_to_norm_map;
	std::set<cdt_mesh::triangle_t>::iterator tr_it;
	for (tr_it = f->fmesh.tris.begin(); tr_it != f->fmesh.tris.end(); tr_it++) {
	    for (size_t j = 0; j < 3; j++) {
		ON_3dPoint *p3d = f->fmesh.pnts[(*tr_it).v[j]];
		vfpnts.insert(p3d);
		ON_3dPoint *onorm = NULL;
		if (s_cdt->singular_vert_to_norms->find(p3d) != s_cdt->singular_vert_to_norms->end()) {
		    // Use calculated normal for singularity points
		    onorm = (*s_cdt->singular_vert_to_norms)[p3d];
		} else {
		    onorm = (*normalmap)[p3d];
		}
		if (onorm) {
		    vfnormals.insert(onorm);
		}
	    }
	}
    }

    // Get the final triangle count
    for (size_t fi = 0; fi < active_faces.size(); fi++) {
	struct ON_Brep_CDT_Face_State *f = (*s_cdt->faces)[active_faces[fi]];
	if (!f) continue;
	triangle_cnt += f->fmesh.tris.size();
    }

    bu_log("tri_cnt: %zd\n", triangle_cnt);

    // We know how many faces, points and normals we need now - initialize BoT containers.
    *fcnt = (int)triangle_cnt;
    *faces = (int *)bu_calloc(triangle_cnt*3, sizeof(int), "new faces array");
    *vcnt = (int)vfpnts.size();
    *vertices = (fastf_t *)bu_calloc(vfpnts.size()*3, sizeof(fastf_t), "new vert array");
    if (normals) {
	*ncnt = (int)vfnormals.size();
	*normals = (fastf_t *)bu_calloc(vfnormals.size()*3, sizeof(fastf_t), "new normals array");
	*fn_cnt = (int)triangle_cnt;
	*face_normals = (int *)bu_calloc(triangle_cnt*3, sizeof(int), "new face_normals array");
    }

    // Populate the arrays and map the ON containers to their corresponding BoT array entries
    std::set<ON_3dPoint *>::iterator p_it;

    // Index vertex points and assign them to the BoT array
    std::map<ON_3dPoint *, int> on_pnt_to_bot_pnt;
    int pnt_ind = 0;
    for (p_it = vfpnts.begin(); p_it != vfpnts.end(); p_it++) {
	ON_3dPoint *vp = *p_it;
	(*vertices)[pnt_ind*3] = vp->x;
	(*vertices)[pnt_ind*3+1] = vp->y;
	(*vertices)[pnt_ind*3+2] = vp->z;
	on_pnt_to_bot_pnt[vp] = pnt_ind;
	(*s_cdt->bot_pnt_to_on_pnt)[pnt_ind] = vp;
	pnt_ind++;
    }

    // Index vertex normal vectors and assign them to the BoT array.  Normal
    // vectors are not always uniquely mapped to vertices (consider, for
    // example, the triangles joining at a sharp edge of a box), but what we
    // are doing here is establishing unique integer identifiers for all normal
    // vectors.  In other words, this is not a unique vertex to normal mapping,
    // but a normal vector to unique index mapping.
    //
    // The mapping of 2D triangle point to its associated normal is the
    // responsibility of the  p2t_to_on3_norm_map container
    std::map<ON_3dPoint *, int> on_norm_to_bot_norm;
    size_t norm_ind = 0;
    if (normals) {
	for (p_it = vfnormals.begin(); p_it != vfnormals.end(); p_it++) {
	    ON_3dPoint *vn = *p_it;
	    (*normals)[norm_ind*3] = vn->x;
	    (*normals)[norm_ind*3+1] = vn->y;
	    (*normals)[norm_ind*3+2] = vn->z;
	    on_norm_to_bot_norm[vn] = norm_ind;
	    norm_ind++;
	}
    }

    // Iterate over faces, adding points and faces to BoT container.  Note: all
    // 3D points should be geometrically unique in this final container.
    int face_cnt = 0;
    triangle_cnt = 0;
    for (size_t fi = 0; fi < active_faces.size(); fi++) {
	struct ON_Brep_CDT_Face_State *f = (*s_cdt->faces)[active_faces[fi]];
	if (!f) continue;
	std::map<ON_3dPoint *, ON_3dPoint *> *normalmap = f->on3_to_norm_map;
	std::set<cdt_mesh::triangle_t>::iterator tr_it;
	triangle_cnt += f->fmesh.tris.size();
	int active_tris = 0;
	for (tr_it = f->fmesh.tris.begin(); tr_it != f->fmesh.tris.end(); tr_it++) {
	    active_tris++;
	    for (size_t j = 0; j < 3; j++) {
		ON_3dPoint *op = f->fmesh.pnts[(*tr_it).v[j]];
		(*faces)[face_cnt*3 + j] = on_pnt_to_bot_pnt[op];
		if (normals) {
		    ON_3dPoint *onorm = (*normalmap)[op];
		    (*face_normals)[face_cnt*3 + j] = on_norm_to_bot_norm[onorm];
		}
	    }
#if 0
	    // If we have a reversed face we need to adjust the triangle vertex
	    // ordering.
	    if (s_cdt->brep->m_F[active_faces[fi]].m_bRev) {
		int ftmp = (*faces)[face_cnt*3 + 1];
		(*faces)[face_cnt*3 + 1] = (*faces)[face_cnt*3 + 2];
		(*faces)[face_cnt*3 + 2] = ftmp;
		if (normals) {
		    // Keep the normals in sync with the vertex points
		    int fntmp = (*face_normals)[face_cnt*3 + 1];
		    (*face_normals)[face_cnt*3 + 1] = (*face_normals)[face_cnt*3 + 2];
		    (*face_normals)[face_cnt*3 + 2] = fntmp;
		}
	    }
#endif

	    face_cnt++;
	}
    }
    //bu_log("tri_cnt_1: %zd\n", triangle_cnt);
    //bu_log("face_cnt: %d\n", face_cnt);

    return 0;
}


/** @} */

// Local Variables:
// mode: C++
// tab-width: 8
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

