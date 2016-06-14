/*                  R H I N O _ R E A D . C P P
 * BRL-CAD
 *
 * Copyright (c) 2016 United States Government as represented by
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
/** @file rhino_read.cpp
 *
 * Brief description
 *
 */


#include "common.h"

#include "bu/path.h"
#include "gcv/api.h"
#include "gcv/util.h"
#include "wdb.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>


#ifndef OBJ_BREP
#error OBJ_BREP is not defined
#endif


namespace
{


template <typename T>
void
autoptr_wrap_bu_free(T *ptr)
{
    bu_free(ptr, "AutoPtr");
}


template <typename T>
void
autoptr_wrap_delete(T *ptr)
{
    delete ptr;
}


template <typename T, void free_fn(T *) = autoptr_wrap_bu_free>
struct AutoPtr {
    explicit AutoPtr(T *vptr = NULL) :
	ptr(vptr)
    {}


    ~AutoPtr()
    {
	if (ptr)
	    free_fn(ptr);
    }


    T *ptr;


private:
    AutoPtr(const AutoPtr &source);
    AutoPtr &operator=(const AutoPtr &source);
};


template<typename T, std::size_t length>
HIDDEN std::size_t array_length(const T(&)[length])
{
    return length;
}


template<typename Target, typename Source>
HIDDEN Target lexical_cast(const Source &value)
{
    std::stringstream interpreter;
    Target result;

    if (!(interpreter << value) ||
	!(interpreter >> result) ||
	!(interpreter >> std::ws).eof())
	throw std::invalid_argument("bad lexical_cast");

    return result;
}


struct UuidCompare {
    bool operator()(const ON_UUID &left, const ON_UUID &right) const
    {
	return ON_UuidCompare(left, right) == -1;
    }
};


class InvalidRhinoModelError : public std::runtime_error
{
public:
    InvalidRhinoModelError(const std::string &value) :
	std::runtime_error(value)
    {}
};


template <template<typename> class Array, typename T>
HIDDEN const T &at(const Array<T> &array, std::size_t index)
{
    if (const T * const result = array.At(static_cast<unsigned>(index)))
	return *result;
    else
	throw InvalidRhinoModelError("invalid index");
}


template <template<typename> class Array, typename T>
HIDDEN T &at(Array<T> &array, std::size_t index)
{
    return const_cast<T &>(at(const_cast<const Array<T> &>(array), index));
}


template <typename T, typename Array>
HIDDEN const T &at(const Array &array, std::size_t index)
{
    if (const T * const result = array.At(static_cast<unsigned>(index)))
	return *result;
    else
	throw InvalidRhinoModelError("invalid index");
}


// ON_CreateUuid() is not implemented for all platforms.
// When it fails, we create a UUIDv4.
HIDDEN ON_UUID
generate_uuid()
{
    ON_UUID result;

    if (ON_CreateUuid(result))
	return result;

    result.Data1 = static_cast<ON__UINT32>(drand48() *
					   std::numeric_limits<ON__UINT32>::max() + 0.5);
    result.Data2 = static_cast<ON__UINT16>(drand48() *
					   std::numeric_limits<ON__UINT16>::max() + 0.5);
    result.Data3 = static_cast<ON__UINT16>(drand48() *
					   std::numeric_limits<ON__UINT16>::max() + 0.5);

    for (std::size_t i = 0; i < array_length(result.Data4); ++i)
	result.Data4[i] = static_cast<unsigned char>(drand48() * 255.0 + 0.5);

    // set the UUIDv4 reserved bits
    result.Data3 = (result.Data3 & 0x0fff) | 0x4000;
    result.Data4[0] = (result.Data4[0] & 0x3f) | 0x80;

    return result;
}


// ONX_Model::Audit() fails to repair UUID issues on platforms
// where ON_CreateUuid() is not implemented.
HIDDEN std::size_t
replace_invalid_uuids(ONX_Model &model)
{
    std::size_t num_repairs = 0;
    std::set<ON_UUID, UuidCompare> seen;
    seen.insert(ON_nil_uuid); // UUIDs can't be nil

#define REPLACE_UUIDS(array, access, member) \
do { \
    for (unsigned i = 0; i < (array).UnsignedCount(); ++i) { \
	while (!seen.insert(at((array), i) access member).second) { \
	    at((array), i) access member = generate_uuid(); \
	    ++num_repairs; \
	} \
    } \
} while (false)

    REPLACE_UUIDS(model.m_bitmap_table, ->, m_bitmap_id);
    REPLACE_UUIDS(model.m_mapping_table, ., m_mapping_id);
    REPLACE_UUIDS(model.m_linetype_table, ., m_linetype_id);
    REPLACE_UUIDS(model.m_layer_table, ., m_layer_id);
    REPLACE_UUIDS(model.m_group_table, ., m_group_id);
    REPLACE_UUIDS(model.m_font_table, ., m_font_id);
    REPLACE_UUIDS(model.m_dimstyle_table, ., m_dimstyle_id);
    REPLACE_UUIDS(model.m_light_table, ., m_attributes.m_uuid);
    REPLACE_UUIDS(model.m_hatch_pattern_table, ., m_hatchpattern_id);
    REPLACE_UUIDS(model.m_idef_table, ., m_uuid);
    REPLACE_UUIDS(model.m_object_table, ., m_attributes.m_uuid);
    REPLACE_UUIDS(model.m_history_record_table, ->, m_record_id);
    REPLACE_UUIDS(model.m_userdata_table, ., m_uuid);

#undef REPLACE_UUIDS

    if (num_repairs)
	model.DestroyCache();

    return num_repairs;
}


HIDDEN void
clean_name(std::map<ON_wString, std::size_t> &seen,
	   const std::string &default_name, ON_wString &name)
{
    name = ON_String(name);

    if (name.IsEmpty())
	name = default_name.c_str();

    name.ReplaceWhiteSpace('_');
    name.Replace('/', '_');

    if (const std::size_t number = seen[name]++)
	name += ('_' + lexical_cast<std::string>(number + 1)).c_str();
}


HIDDEN void
load_model(const gcv_opts &gcv_options, const std::string &path,
	   ONX_Model &model, std::string &root_name)
{
    if (!model.Read(path.c_str()))
	throw InvalidRhinoModelError("ONX_Model::Read() failed");

    int num_problems;
    int num_repairs = replace_invalid_uuids(model);

    {
	int temp;
	num_problems = model.Audit(true, &temp, NULL, NULL);
	num_repairs += temp;
    }

    if (num_problems)
	throw InvalidRhinoModelError("repair failed");
    else if (num_repairs && gcv_options.verbosity_level)
	std::cerr << "repaired " << num_repairs << " model issues\n";

    // clean and remove duplicate names
    std::map<ON_wString, std::size_t> seen;
    {
	ON_wString temp = root_name.c_str();
	clean_name(seen, gcv_options.default_name, temp);
	root_name = ON_String(temp).Array();
    }

#define REPLACE_NAMES(array, member) \
do { \
    for (unsigned i = 0; i < (array).UnsignedCount(); ++i) \
	clean_name(seen, gcv_options.default_name, at((array), i).member); \
} while (false)

    REPLACE_NAMES(model.m_layer_table, m_name);
    REPLACE_NAMES(model.m_idef_table, m_name);
    REPLACE_NAMES(model.m_object_table, m_attributes.m_name);
    REPLACE_NAMES(model.m_light_table, m_attributes.m_name);

#undef REPLACE_NAMES
}


HIDDEN void
write_geometry(rt_wdb &wdb, const std::string &name, const ON_Brep &brep)
{
    if (mk_brep(&wdb, name.c_str(), const_cast<ON_Brep *>(&brep)))
	throw std::runtime_error("mk_brep() failed");
}


HIDDEN void
write_geometry(rt_wdb &wdb, const std::string &name, ON_Mesh mesh)
{
    mesh.ConvertQuadsToTriangles();
    mesh.CombineIdenticalVertices();
    mesh.Compact();

    const std::size_t num_vertices = mesh.m_V.UnsignedCount();
    const std::size_t num_faces = mesh.m_F.UnsignedCount();

    if (!num_vertices || !num_faces)
	return;

    unsigned char orientation;

    switch (mesh.SolidOrientation()) {
	case 0:
	    orientation = RT_BOT_UNORIENTED;
	    break;

	case 1:
	    orientation = RT_BOT_CW;
	    break;

	case -1:
	    orientation = RT_BOT_CCW;
	    break;

	default:
	    throw std::logic_error("unknown orientation");
    }

    std::vector<fastf_t> vertices(3 * num_vertices);

    for (unsigned i = 0; i < num_vertices; ++i) {
	fastf_t * const dest_vertex = &vertices.at(3 * i);
	const ON_3fPoint &source_vertex = at<ON_3fPoint>(mesh.m_V, i);
	VMOVE(dest_vertex, source_vertex);
    }

    std::vector<int> faces(3 * num_faces);

    for (unsigned i = 0; i < num_faces; ++i) {
	int * const dest_face = &faces.at(3 * i);
	const int * const source_face = at(mesh.m_F, i).vi;
	VMOVE(dest_face, source_face);
    }

    unsigned char mode;
    {
	rt_bot_internal bot;
	std::memset(&bot, 0, sizeof(bot));
	bot.magic = RT_BOT_INTERNAL_MAGIC;
	bot.orientation = orientation;
	bot.num_vertices = num_vertices;
	bot.num_faces = num_faces;
	bot.vertices = &vertices.at(0);
	bot.faces = &faces.at(0);
	mode = gcv_bot_is_solid(&bot) ? RT_BOT_SOLID : RT_BOT_PLATE;
    }

    std::vector<fastf_t> thicknesses;
    AutoPtr<bu_bitv, bu_bitv_free> bitv;

    if (mode == RT_BOT_PLATE) {
	const fastf_t plate_thickness = 1.0;

	thicknesses.assign(num_faces, plate_thickness);
	bitv.ptr = bu_bitv_new(num_faces);
    }

    if (mesh.m_FN.UnsignedCount() != num_faces) {
	if (mk_bot(&wdb, name.c_str(), mode, orientation, 0, num_vertices,
		   num_faces, &vertices.at(0), &faces.at(0),
		   thicknesses.empty() ? NULL :  &thicknesses.at(0), bitv.ptr))
	    throw std::runtime_error("mk_bot() failed");

	return;
    }

    mesh.UnitizeFaceNormals();
    std::vector<fastf_t> normals(3 * mesh.m_FN.UnsignedCount());

    for (unsigned i = 0; i < mesh.m_FN.UnsignedCount(); ++i) {
	fastf_t * const dest_normal = &normals.at(3 * i);
	const ON_3fVector &source_normal = at<ON_3fVector>(mesh.m_FN, i);
	VMOVE(dest_normal, source_normal);
    }

    std::vector<int> face_normals(3 * mesh.m_FN.UnsignedCount());

    for (unsigned i = 0; i < mesh.m_FN.UnsignedCount(); ++i) {
	int * const dest_face_normal = &face_normals.at(3 * i);
	VSETALL(dest_face_normal, i);
    }

    if (mk_bot_w_normals(&wdb, name.c_str(), mode, orientation,
			 RT_BOT_HAS_SURFACE_NORMALS | RT_BOT_USE_NORMALS, num_vertices,
			 num_faces, &vertices.at(0), &faces.at(0),
			 thicknesses.empty() ? NULL : &thicknesses.at(0), bitv.ptr,
			 num_faces, &normals.at(0), &face_normals.at(0)))
	throw std::runtime_error("mk_bot_w_normals() failed");
}


HIDDEN bool
write_geometry(rt_wdb &wdb, const std::string &name,
	       const ON_Geometry &geometry)
{
    if (const ON_Brep * const brep = ON_Brep::Cast(&geometry)) {
	write_geometry(wdb, name, *brep);
    } else if (const ON_Mesh * const mesh = ON_Mesh::Cast(&geometry)) {
	write_geometry(wdb, name, *mesh);
    } else if (geometry.HasBrepForm()) {
	AutoPtr<ON_Brep> temp(geometry.BrepForm());
	write_geometry(wdb, name, *temp.ptr);
    } else
	return false;

    return true;
}


typedef std::pair<std::string, std::string> Shader;


HIDDEN Shader
get_shader(const ON_Material &material)
{
    std::ostringstream temp;

    temp << "{"
	 << " tr " << material.m_transparency
	 << " re " << material.m_reflectivity
	 << " sp " << 0
	 << " di " << 0.3
	 << " ri " << material.m_index_of_refraction
	 << " ex " << 0
	 << " sh " << material.m_shine
	 << " em " << material.m_emission
	 << " }";

    return std::make_pair("plastic", temp.str());
}


HIDDEN void
get_object_material(const ON_3dmObjectAttributes &attributes,
		    const ONX_Model &model, Shader &out_shader, unsigned char *out_rgb,
		    bool &out_own_shader, bool &out_own_rgb)
{
    ON_Material temp;
    model.GetRenderMaterial(attributes, temp);
    out_shader = get_shader(temp);
    out_own_shader = attributes.MaterialSource() != ON::material_from_parent;

    out_rgb[0] = model.WireframeColor(attributes).Red();
    out_rgb[1] = model.WireframeColor(attributes).Green();
    out_rgb[2] = model.WireframeColor(attributes).Blue();
    out_own_rgb = attributes.ColorSource() != ON::color_from_parent;

    if (!out_rgb[0] && !out_rgb[1] && !out_rgb[2]) {
	ON_Material material;
	model.GetRenderMaterial(attributes, material);

	out_rgb[0] = material.m_diffuse.Red();
	out_rgb[1] = material.m_diffuse.Green();
	out_rgb[2] = material.m_diffuse.Blue();
	out_own_rgb = out_own_shader;
    }
}


HIDDEN void
write_comb(rt_wdb &wdb, const std::string &name,
	   const std::set<std::string> &members, const mat_t matrix = NULL,
	   const char *shader_name = NULL, const char *shader_options = NULL,
	   const unsigned char *rgb = NULL)
{
    wmember wmembers;
    BU_LIST_INIT(&wmembers.l);

    for (std::set<std::string>::const_iterator it = members.begin();
	 it != members.end(); ++it)
	mk_addmember(it->c_str(), &wmembers.l, const_cast<fastf_t *>(matrix),
		     WMOP_UNION);

    if (mk_comb(&wdb, name.c_str(), &wmembers.l, false, shader_name, shader_options,
		rgb, 0, 0, 0, 0, false, false, false))
	throw std::runtime_error("mk_comb() failed");
}


HIDDEN void
import_object(rt_wdb &wdb, const std::string &name,
	      const ON_InstanceRef &instance_ref, const ONX_Model &model,
	      const char *shader_name, const char *shader_options, const unsigned char *rgb)
{
    const ON_InstanceDefinition &idef = at(model.m_idef_table,
					   model.IDefIndex(instance_ref.m_instance_definition_uuid));

    mat_t matrix;

    for (std::size_t i = 0; i < 4; ++i)
	for (std::size_t j = 0; j < 4; ++j)
	    matrix[4 * i + j] = instance_ref.m_xform[i][j];

    std::set<std::string> members;
    members.insert(ON_String(idef.m_name).Array());

    write_comb(wdb, name, members, matrix, shader_name, shader_options, rgb);
}


HIDDEN void
write_attributes(rt_wdb &wdb, const std::string &name, const ON_Object &object,
		 const ON_UUID &uuid)
{
    // ON_UuidToString() buffers must be >= 37 chars
    const std::size_t uuid_string_length = 37;
    char temp[uuid_string_length];

    if (db5_update_attribute(name.c_str(), "rhino::type",
			     object.ClassId()->ClassName(), wdb.dbip))
	throw std::runtime_error("db5_update_attribute() failed");

    if (db5_update_attribute(name.c_str(), "rhino::uuid", ON_UuidToString(uuid,
			     temp), wdb.dbip))
	throw std::runtime_error("db5_update_attribute() failed");
}


HIDDEN void
import_model_objects(const gcv_opts &gcv_options, rt_wdb &wdb,
		     const ONX_Model &model)
{
    std::size_t success_count = 0;

    for (unsigned i = 0; i < model.m_object_table.UnsignedCount(); ++i) {
	const ONX_Model_Object &object = at(model.m_object_table, i);
	const std::string name = ON_String(object.m_attributes.m_name).Array();
	const std::string member_name = name + ".s";

	Shader shader;
	unsigned char rgb[3];
	bool own_shader, own_rgb;

	get_object_material(object.m_attributes, model, shader, rgb, own_shader,
			    own_rgb);

	if (const ON_InstanceRef * const temp = ON_InstanceRef::Cast(object.m_object))
	    import_object(wdb, name, *temp, model, own_shader ? shader.first.c_str() : NULL,
			  own_shader ? shader.second.c_str() : NULL, own_rgb ? rgb : NULL);
	else if (write_geometry(wdb, member_name,
				*ON_Geometry::Cast(object.m_object))) {
	    std::set<std::string> members;
	    members.insert(member_name);
	    write_comb(wdb, name, members, NULL, own_shader ? shader.first.c_str() : NULL,
		       own_shader ? shader.second.c_str() : NULL, own_rgb ? rgb : NULL);
	} else {
	    if (gcv_options.verbosity_level)
		std::cerr << "skipped " << object.m_object->ClassId()->ClassName() <<
			  " model object '" << name << "'\n";

	    continue;
	}

	write_attributes(wdb, name, *object.m_object, object.m_attributes.m_uuid);
	++success_count;
    }

    if (gcv_options.verbosity_level)
	if (success_count != model.m_object_table.UnsignedCount())
	    std::cerr << "imported " << success_count << " of " <<
		      model.m_object_table.UnsignedCount() << " objects\n";
}


HIDDEN void
import_idef(rt_wdb &wdb, const ON_InstanceDefinition &idef,
	    const ONX_Model &model)
{
    std::set<std::string> members;

    for (unsigned i = 0; i < idef.m_object_uuid.UnsignedCount(); ++i) {
	const ONX_Model_Object &object = at(model.m_object_table,
					    model.ObjectIndex(at(idef.m_object_uuid, i)));

	members.insert(ON_String(object.m_attributes.m_name).Array());
    }

    const std::string name = ON_String(idef.m_name).Array();

    write_comb(wdb, name, members);
    write_attributes(wdb, name, idef, idef.m_uuid);
}


HIDDEN void
import_model_idefs(rt_wdb &wdb, const ONX_Model &model)
{
    for (unsigned i = 0; i < model.m_idef_table.UnsignedCount(); ++i)
	import_idef(wdb, at(model.m_idef_table, i), model);
}


HIDDEN std::set<std::string>
get_all_idef_members(const ONX_Model &model)
{
    std::set<std::string> result;

    for (unsigned i = 0; i < model.m_idef_table.UnsignedCount(); ++i) {
	const ON_InstanceDefinition &idef = at(model.m_idef_table, i);

	for (unsigned j = 0; j < idef.m_object_uuid.UnsignedCount(); ++j) {
	    const ONX_Model_Object &object = at(model.m_object_table,
						model.ObjectIndex(at(idef.m_object_uuid, j)));
	    result.insert(ON_String(object.m_attributes.m_name).Array());
	}
    }

    return result;
}


HIDDEN std::set<std::string>
get_layer_members(const ON_Layer &layer, const ONX_Model &model)
{
    std::set<std::string> members;

    for (unsigned i = 0; i < model.m_layer_table.UnsignedCount(); ++i) {
	const ON_Layer &current_layer = at(model.m_layer_table, i);

	if (current_layer.m_parent_layer_id == layer.ModelObjectId())
	    members.insert(ON_String(current_layer.m_name).Array());
    }

    for (unsigned i = 0; i < model.m_object_table.UnsignedCount(); ++i) {
	const ONX_Model_Object &object = at(model.m_object_table, i);

	if (object.m_attributes.m_layer_index == layer.m_layer_index)
	    members.insert(ON_String(object.m_attributes.m_name).Array());
    }

    const std::set<std::string> model_idef_members = get_all_idef_members(model);
    std::set<std::string> result;
    std::set_difference(members.begin(), members.end(), model_idef_members.begin(),
			model_idef_members.end(), std::inserter(result, result.end()));

    return result;
}


HIDDEN void
import_layer(rt_wdb &wdb, const ON_Layer &layer, const ONX_Model &model)
{
    const std::string name = ON_String(layer.m_name).Array();

    unsigned char rgb[3];
    rgb[0] = layer.Color().Red();
    rgb[1] = layer.Color().Red();
    rgb[2] = layer.Color().Red();

    const Shader &shader = get_shader(layer.RenderMaterialIndex() != -1 ?
				      at(model.m_material_table, layer.RenderMaterialIndex()) : ON_Material());

    write_comb(wdb, name, get_layer_members(layer, model), NULL,
	       shader.first.c_str(), shader.second.c_str(), rgb);
    write_attributes(wdb, name, layer, layer.m_layer_id);
}


HIDDEN void
import_model_layers(rt_wdb &wdb, const ONX_Model &model,
		    const std::string &root_name)
{
    for (unsigned i = 0; i < model.m_layer_table.UnsignedCount(); ++i)
	import_layer(wdb, at(model.m_layer_table, i), model);

    ON_Layer root_layer;
    root_layer.SetLayerName(root_name.c_str());
    import_layer(wdb, root_layer, model);
}


HIDDEN std::string
get_name(const db_i &db, const std::string &prefix, const std::string &suffix)
{
    std::string end = suffix;
    std::size_t num = 1;

    while (db_lookup(&db, (prefix + end).c_str(), false))
	end = "_" + lexical_cast<std::string>(++num) + suffix;

    return prefix + end;
}


HIDDEN void
move_all(db_i &db, const std::string &name, const std::string &new_name)
{
    directory * const dir = db_lookup(&db, name.c_str(), true);

    if (!dir)
	throw std::runtime_error("db_lookup() failed");

    if (db_rename(&db, dir, new_name.c_str()))
	throw std::runtime_error("db_rename() failed");

    AutoPtr<directory *> comb_dirs;
    std::size_t num_combs = db_ls(&db, DB_LS_COMB, NULL, &comb_dirs.ptr);
    bu_ptbl stack = BU_PTBL_INIT_ZERO;
    AutoPtr<bu_ptbl, bu_ptbl_free> autofree_stack(&stack);

    for (std::size_t i = 0; i < num_combs; ++i)
	if (!db_comb_mvall(comb_dirs.ptr[i], &db, name.c_str(), new_name.c_str(),
			   &stack))
	    throw std::runtime_error("db_comb_mvall() failed");
}


HIDDEN void
polish_output(const gcv_opts &gcv_options, db_i &db)
{
    bu_ptbl found = BU_PTBL_INIT_ZERO;
    AutoPtr<bu_ptbl, db_search_free> autofree_found(&found);

    db_update_nref(&db, &rt_uniresource);

    std::map<std::string, std::string> to_rename;

    // rename shapes after their parent combs
    if (0 > db_search(&found, DB_SEARCH_TREE,
		      "-type shape -below -attr rhino::type=ON_Layer", 0, NULL, &db))
	throw std::runtime_error("db_search() failed");

    if (BU_PTBL_LEN(&found)) {
	const std::string unnamed_pattern = gcv_options.default_name + std::string("*");
	db_full_path **entry;

	for (BU_PTBL_FOR(entry, (db_full_path **), &found)) {
	    if (DB_FULL_PATH_CUR_DIR(*entry)->d_nref != 1)
		continue;

	    for (ssize_t i = (*entry)->fp_len - 1; i >= 0; --i)

		if (bu_fnmatch(unnamed_pattern.c_str(), (*entry)->fp_names[i]->d_namep, 0)
		    && bu_fnmatch("IDef*", (*entry)->fp_names[i]->d_namep, 0)) {
		    if (i != static_cast<ssize_t>((*entry)->fp_len) - 1)
			move_all(db, DB_FULL_PATH_CUR_DIR(*entry)->d_namep, get_name(db,
				 (*entry)->fp_names[i]->d_namep, ".s"));

		    for (ssize_t j = i + 1; j < static_cast<ssize_t>((*entry)->fp_len) - 1; ++j)
			to_rename.insert(std::make_pair((*entry)->fp_names[j]->d_namep,
							(*entry)->fp_names[i]->d_namep));

		    break;
		}
	}
    }

    db_search_free(&found);
    BU_PTBL_INIT(&found);

    if (0 > db_search(&found, DB_SEARCH_RETURN_UNIQ_DP | DB_SEARCH_FLAT,
		      "-attr rhino::type=ON_Layer", 0, NULL, &db))
	throw std::runtime_error("db_search() failed");

    const char * const ignored_attributes[] = {"rhino::type", "rhino::uuid"};
    rt_reduce_db(&db, array_length(ignored_attributes), ignored_attributes, &found);

    // apply region flag
    db_search_free(&found);
    BU_PTBL_INIT(&found);

    if (0 > db_search(&found, DB_SEARCH_RETURN_UNIQ_DP,
		      "-type comb -attr rgb -not -above -attr rgb -or -attr shader -not -above -attr shader",
		      0, NULL, &db))
	throw std::runtime_error("db_search() failed");

    if (BU_PTBL_LEN(&found)) {
	directory **entry;

	for (BU_PTBL_FOR(entry, (directory **), &found))
	    if (db5_update_attribute((*entry)->d_namep, "region", "R", &db))
		throw std::runtime_error("db5_update_attribute() failed");
    }

    // rename combs after their contained shapes
    for (std::map<std::string, std::string>::const_iterator it = to_rename.begin();
	 it != to_rename.end(); ++it)
	if (db_lookup(&db, it->first.c_str(), false))
	    move_all(db, it->first, get_name(db, it->second, ".r"));
}


HIDDEN int
rhino_read(gcv_context *context, const gcv_opts *gcv_options,
	   const void *UNUSED(options_data), const char *source_path)
{
    std::string root_name;
    {
	bu_vls temp = BU_VLS_INIT_ZERO;
	AutoPtr<bu_vls, bu_vls_free> autofree_temp(&temp);

	if (!bu_path_component(&temp, source_path, BU_PATH_BASENAME))
	    return 1;

	root_name = bu_vls_addr(&temp);
    }

    try {
	ONX_Model model;
	load_model(*gcv_options, source_path, model, root_name);
	import_model_layers(*context->dbip->dbi_wdbp, model, root_name);
	import_model_idefs(*context->dbip->dbi_wdbp, model);
	import_model_objects(*gcv_options, *context->dbip->dbi_wdbp, model);
    } catch (const InvalidRhinoModelError &exception) {
	std::cerr << "invalid input file ('" << exception.what() << "')\n";
	return 0;
    }

    polish_output(*gcv_options, *context->dbip);

    return 1;
}


struct gcv_filter gcv_conv_rhino_read = {
    "Rhino Reader", GCV_FILTER_READ, BU_MIME_MODEL_VND_RHINO,
    NULL, NULL, rhino_read
};

static const gcv_filter * const filters[] = {&gcv_conv_rhino_read, NULL};


}


extern "C"
{
    extern const gcv_plugin gcv_plugin_info_s = {filters};
    GCV_EXPORT const struct gcv_plugin *
    gcv_plugin_info()
    {
	return &gcv_plugin_info_s;
    }
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
