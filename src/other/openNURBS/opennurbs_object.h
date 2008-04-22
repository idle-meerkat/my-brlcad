/* $Header$ */
/* $NoKeywords: $ */
//
// Copyright (c) 1993-2000 Robert McNeel & Associates. All rights reserved.
// Rhinoceros is a registered trademark of Robert McNeel & Assoicates.
//
// THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
// ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF
// MERCHANTABILITY ARE HEREBY DISCLAIMED.
//
////////////////////////////////////////////////////////////////
//
//   virtual base class for all openNURBS objects
//
////////////////////////////////////////////////////////////////

#if !defined(OPENNURBS_OBJECT_INC_)
#define OPENNURBS_OBJECT_INC_


class ON_ClassId; // used for runtime class identification

////////////////////////////////////////////////////////////////
//
// Runtime class id
//

// Description:
//   Every class derived from ON_Object has a class id that records
//   its class name, baseclass name, and class uuid.  The
//   ON_OBJECT_DECLARE and ON_OBJECT_IMPLEMENT macros generate
//   the code that creates and initializes class ids.
//
//   The ON_Object::IsKindOf() and ON_Object::Cast() functions
//   use these class ids.
class ON_CLASS ON_ClassId
{
public:

  // Description:
  //   This constructor is called to initialize each class id.
  //   The call is generated by the ON_OBJECT_IMPLEMENT macro.
  //
  // Parameters:
  //   sClassName - [in] name of the class (like ON_Geometry)
  //   sBaseClassName - [in] name of baseclass (like ON_Object)
  //   create - [in] function to create a new object(like CreateNewON_Geometry())
  //   sUUID - [in] UUID in registry format from Windows guidgen.exe
  ON_ClassId(
          const char* sClassName,
          const char* sBaseClassName,
          ON_Object* (*create)(),
          const char* sUUID
           );
  ~ON_ClassId();

  // Description:
  //   Gets a class's ON_ClassId from the class's name.
  // Parameters:
  //   sClassName - [in] name of class
  // Returns:
  //   Pointer to the class's ON_ClassId.
  // Example:
  //   const ON_ClassId* brep_id = ON_CLassId::ClassId("ON_Brep");
  static const ON_ClassId* ClassId(
          const char* sClassName
          );

  // Description:
  //   Gets a class's ON_ClassId from the class's uuid.
  // Parameters:
  //   class_uuid - [in] uuid for the class
  // Returns:
  //   Pointer to the class's ON_ClassId.
  // Example:
  //   ON_UUID brep_uuid = ON_UuidFromString("60B5DBC5-E660-11d3-BFE4-0010830122F0");
  //   const ON_ClassId* brep_id = ON_CLassId::ClassId(brep_uuid);
  static const ON_ClassId* ClassId(
          ON_UUID class_uuid
          );

  // Description:
  //   Each class derived from ON_Object has a corresponding ON_ClassId
  //   stored in a linked list and the class is marked with an integer
  //   value.  ON_ClassId::IncrementMark() increments the value used to
  //   mark new classes and returns the new marking value.
  // Returns:
  //   Value that will be used to mark all future ON_ClassIds.
  static int IncrementMark();

  // Description:
  //   Each class derived from ON_Object has a corresponding
  //   ON_ClassId stored in a linked list.  If a class definition
  //   is going to disappear (which happens when the derived object
  //   definition is in a DLL that uses openNURBS as a DLL and the
  //   DLL containing the derived object's definition is unloaded),
  //   then the class's ON_ClassId needs to be removed from the class
  //   list.  ON_ClassId::Purge( mark ) removes all ON_ClassIds with a
  //   a prescribed mark and returns the number of classes that
  //   were purged.
  // Parameters:
  //   mark - [in] All ON_ClassIds with this mark will be purged.
  // Returns:
  //   Number of classes that were purged.
  // Example:
  //   // Call ON_ClassId::IncrementMark() BEFORE loading MY.DLL.
  //   int my_dll_classid_mark = ON_ClassId::IncrementMark();
  //   load MY.DLL with classes derived from ON_Object
  //   ...
  //   // Call ON_ClassId::Purge() BEFORE unloading MY.DLL.
  //   ON_ClassId::Purge( my_dll_classid_mark );
  //   unload MY.DLL
  static int Purge(
    int mark
    );

  // Description:
  //   Dumps the ON_ClassId list
  // Parameters:
  //   dump - [in] destination for the text dump.
  static void Dump(
    ON_TextLog& dump
    );

  // Description:
  //   OnExitMemoryCleanup() releases all internal memory used by the
  //   openNURBS classes for Cast() and serialization construction.
  //   ON_ClassId::OnExitMemoryCleanup() should only be called when
  //   the application is finished using the openNURBS library.
  static void OnExitMemoryCleanup();

  // Returns:
  //   class name
  const char* ClassName() const;

  // Returns:
  //   base class name
  const char* BaseClassName() const;

  // Returns:
  //   base class id
  const ON_ClassId* BaseClass() const;

  // Description:
  //   Determine if the class associated with this ON_ClassId
  //   is derived from another class.
  // Parameters:
  //   potential_parent - [in] Class to test as parent.
  // Returns:
  //   TRUE if this is derived from potential_parent.
	BOOL IsDerivedFrom(
    const ON_ClassId* potential_parent
    ) const;

  // Descrption:
  //   Create an instance of the class associated with
  //   class id.
  // Returns:
  //   Instance of the class id's class.
  ON_Object* Create() const;

  // Returns:
  //   class uuid
  ON_UUID Uuid() const;

  /*
  Description:
    Opennurbs classes have a mark value of 0.  Core Rhino
    classes have a mark value of 1.  Rhino plug-in classes
    have a mark value of > 1.
  Returns:
    Class mark value
  */
  int Mark() const;

private:
  static ON_ClassId* m_p0;     // first in the linked list of class ids
  static int m_mark0;  // current mark value
  ON_ClassId* m_pNext;         // next in the linked list of class ids
  const ON_ClassId* m_pBaseClassId;  // base class id
  char m_sClassName[80];
  char m_sBaseClassName[80];
  ON_Object* (*m_create)();
  ON_UUID m_uuid;
  int m_mark;

private:
  // no implementaion to prohibit use
  ON_ClassId();
  ON_ClassId( const ON_ClassId&);
  ON_ClassId& operator=( const ON_ClassId&);
};

// Description:
//   Macro to easily get a pointer to the ON_ClassId for a
//   given class;
//
// Example:
//
//          const ON_ClassId* brep_class_id = ON_CLASS_ID("ON_Brep");
//
#define ON_CLASS_ID( cls ) ON_ClassId::ClassId( #cls )


typedef int (*ON_Vtable_func)(void);

struct ON_Vtable
{
  // The actual number of virtual functions depends on the class.
  // The four in f[4] is just there to make it easy to see the
  // first four in the debugger.  There can be fewer or more
  // than four virtual functions.  The function prototype
  // also depends on the class defintion.  In particular,
  // it is probably not int function(void).
  ON_Vtable_func f[4];
};

/*
Description:
  Expert user function to get a pointer to
  a class's vtable.
Parameters:
  pClass - [in] a class that has a vtable.
    If you pass a class that does not have
    a vtable, then the returned pointer is
    garbage.
Returns:
  A pointer to the vtable.
*/
ON_DECL
struct ON_Vtable* ON_ClassVtable(void* p);


/*
All classes derived from ON_Object must have

  ON_OBJECT_DECLARE( <classname> );

as the first line in their class definition an a corresponding

  ON_VIRTUAL_OBJECT_IMPLEMENT( <classname>, <basclassname>, <classuuid> );

or

  ON_OBJECT_IMPLEMENT( <classname>, <basclassname>, <classuuid> );

in a .CPP file.
*/
#define ON_OBJECT_DECLARE( cls )                                \
  protected:                                                    \
    static void* m_s_##cls##_ptr;                               \
  public:                                                       \
    static const ON_ClassId m_##cls##_class_id;                 \
    /*record used for ON_Object runtime type information*/      \
                                                                \
    static cls * Cast( ON_Object* );                            \
    /*Description: Similar to C++ dynamic_cast*/                \
    /*Returns: object on success. NULL on failure*/             \
                                                                \
    static const cls * Cast( const ON_Object* );                \
    /*Description: Similar to C++ dynamic_cast*/                \
    /*Returns: object on success. NULL on failure*/             \
                                                                \
    virtual const ON_ClassId* ClassId() const;                  \
    /*Description:*/                                            \
                                                                \
  private:                                                      \
    virtual ON_Object* DuplicateObject() const;                 \
    /*used by Duplicate to create copy of an object.*/          \
                                                                \
  public:                                                       \
    cls * Duplicate() const;                                    \
    /*Description: Expert level tool - no support available.*/  \
    /*If this class is derived from CRhinoObject, use CRhinoObject::DuplicateRhinoObject instead*/

// Objects derived from ON_Object that do not have a valid new, operator=,
// or copy constructor must use ON_VIRTUAL_OBJECT_IMPLEMENT instead of
// ON_OBJECT_IMPLEMENT.  Objects defined with ON_VIRTUAL_OBJECT_IMPLEMENT
// cannot be serialized using ON_BinaryArchive::ReadObject()/WriteObject()
// or duplicated using ON_Object::Duplicate().
//
// The Cast() and ClassId() members work on objects defined with either
// ON_VIRTUAL_OBJECT_IMPLEMENT or ON_OBJECT_IMPLEMENT.
#define ON_VIRTUAL_OBJECT_IMPLEMENT( cls, basecls, uuid ) \
  void* cls::m_s_##cls##_ptr = 0;\
  const ON_ClassId cls::m_##cls##_class_id(#cls,#basecls,0,uuid);\
  cls * cls::Cast( ON_Object* p) {return(cls *)Cast((const ON_Object*)p);} \
  const cls * cls::Cast( const ON_Object* p) {return(p&&p->IsKindOf(&cls::m_##cls##_class_id))?(const cls *)p:0;} \
  const ON_ClassId* cls::ClassId() const {return &cls::m_##cls##_class_id;} \
  ON_Object* cls::DuplicateObject() const {return 0;} \
  cls * cls::Duplicate() const {return static_cast<cls *>(DuplicateObject());}

// Objects derived from ON_Object that use ON_OBJECT_IMPLEMENT must
// have a valid operator= and copy constructor.  Objects defined with
// ON_OBJECT_IMPLEMENT may be serialized using
// ON_BinaryArchive::ReadObject()/WriteObject()
// and duplicated by calling ON_Object::Duplicate().
#define ON_OBJECT_IMPLEMENT( cls, basecls, uuid ) \
  void* cls::m_s_##cls##_ptr = 0;\
  static ON_Object* CreateNew##cls() {return new cls();} \
  const ON_ClassId cls::m_##cls##_class_id(#cls,#basecls,CreateNew##cls,uuid);\
  cls * cls::Cast( ON_Object* p) {return(cls *)Cast((const ON_Object*)p);} \
  const cls * cls::Cast( const ON_Object* p) {return(p&&p->IsKindOf(&cls::m_##cls##_class_id))?(const cls *)p:0;} \
  const ON_ClassId* cls::ClassId() const {return &cls::m_##cls##_class_id;} \
  ON_Object* cls::DuplicateObject() const {cls* p = new cls(); if (p) *p=*this; return p;} \
  cls * cls::Duplicate() const {return static_cast<cls *>(DuplicateObject());}

#define ON__SET__THIS__PTR(ptr) if (ptr) *((void**)this) = ptr

class ON_UserData;

class ON_CLASS ON_UserString
{
public:
  ON_UserString();
  ~ON_UserString();
  ON_wString m_key;
  ON_wString m_string_value;

  void Dump(ON_TextLog& text_log) const;
  bool Write(ON_BinaryArchive&) const;
  bool Read(ON_BinaryArchive&);
};

#if defined(ON_DLL_TEMPLATE)
// This stuff is here because of a limitation in the way Microsoft
// handles templates and DLLs.  See Microsoft's knowledge base
// article ID Q168958 for details.
#pragma warning( push )
#pragma warning( disable : 4231 )
ON_DLL_TEMPLATE template class ON_CLASS ON_ClassArray<ON_UserString>;
#pragma warning( pop )
#endif

////////////////////////////////////////////////////////////////

// Description:
//   Pure virtual base class for all classes that must provide
//   runtime class id or support object level 3DM serialization
class ON_CLASS ON_Object
{
  /////////////////////////////////////////////////////////////////
  //
  // Any object derived from ON_Object should have a
  //   ON_OBJECT_DECLARE(ON_...);
  // as the last line of its class definition and a
  //   ON_OBJECT_IMPLEMENT( ON_..., ON_baseclass );
  // in a .cpp file.
  //
  // These macros declare and implement public members
  //
  // static ON_ClassId m_ON_Object;
  // static cls * Cast( ON_Object* );
  // static const cls * Cast( const ON_Object* );
  // virtual const ON_ClassId* ClassId() const;
  ON_OBJECT_DECLARE(ON_Object);

public:

#if defined(ON_DLL_EXPORTS) || defined(ON_DLL_IMPORTS)
  // See comments at the top of opennurbs_object.cpp for details.

  // new/delete
  void* operator new(size_t);
  void  operator delete(void*);

  // array new/delete
  void* operator new[] (size_t);
  void  operator delete[] (void*);

  // in place new/delete
  void* operator new(size_t,void*);
  void  operator delete(void*,void*);
#endif

  ON_Object();
  ON_Object( const ON_Object& );
  ON_Object& operator=( const ON_Object& );
  virtual ~ON_Object();

  /*
  Description:
    The MemoryRelocate() function is called when an
    object's location in memory is changed.  For
    example, if an object resides in a chunk of
    memory that is grown by calling a realloc
    that has to allocate a new chunk and
    copy the contents of the old chunk to the
    new chunk, then the location of the object's
    memory changes.  In practice this happens when
    classes derived from ON_Object are stored
    in dynamic arrays, like the default implementation
    of ON_ObjectArray<>'s that use realloc to grow
    the dynamic array.
  */
  virtual
  void MemoryRelocate();

  /*
  Description:
    Low level tool to test if an object is derived
    from a specified class.
  Parameters:
    pClassId - [in] use classname::ClassId()
  Returns:
    TRUE if the instantiated object is derived from the
    class whose id is passed as the argument.
  Example:

          ON_Object* p = ....;
          if ( p->IsKindOf( ON_NurbsCurve::ClassId() ) )
          {
            it's a NURBS curve
          }

  Remarks:
    The primary reason for IsKindOf() is to support the
    static Cast() members declared in the ON_OBJECT_DECLARE
    macro.  If we determine that dynamic_cast is properly
    supported and implemented by all supported compilers,
    then IsKindOf() may dissappear.  If an application needs
    to determine if a pointer points to a class derived from
    ON_SomeClassName, then call
    ON_SomeClassName::Cast(mystery pointer) and check for
    a non-null return.
  */
  BOOL IsKindOf(
        const ON_ClassId* pClassId
        ) const;

  /*
  Description:
    Tests an object to see if its data members are correctly
    initialized.
  Parameters:
    text_log - [in] if the object is not valid and text_log
        is not NULL, then a brief englis description of the
        reason the object is not valid is appened to the log.
        The information appended to text_log is suitable for
        low-level debugging purposes by programmers and is
        not intended to be useful as a high level user
        interface tool.
  Returns:
    @untitled table
    TRUE     object is valid
    FALSE    object is invalid, uninitialized, etc.
  */
  virtual
  BOOL IsValid( ON_TextLog* text_log = NULL ) const = 0;

  /*
  Description:
    Creates a text dump of the object.
  Remarks:
    Dump() is intended for debugging and is not suitable
    for creating high quality text descriptions of an
    object.

    The default implementations of this virtual function
    prints the class's name.
  */
  virtual
  void Dump( ON_TextLog& ) const;

  /*
  Returns:
    An estimate of the amount of memory the class uses in bytes.
  */
  virtual
  unsigned int SizeOf() const;

  /*
  Description:
    Returns a CRC calculated from the information that defines
    the object.  This CRC can be used as a quick way to see
    if two objects are not identical.
  Parameters:
    current_remainder - [in];
  Returns:
    CRC of the information the defines the object.
  */
  virtual
  ON__UINT32 DataCRC(ON__UINT32 current_remainder) const;

  /*
  Description:
    Low level archive writing tool used by ON_BinaryArchive::WriteObject().
  Parameters:
    binary_archive - archive to write to
  Returns:
    Returns TRUE if the write is successful.
  Remarks:
    Use ON_BinaryArchive::WriteObject() to write objects.
    This Write() function should just write the specific definition of
    this object.  It should not write and any chunk typecode or length
    information.

    The default implementation of this virtual function returns
    FALSE and does nothing.
  */
  virtual
  BOOL Write(
         ON_BinaryArchive& binary_archive
       ) const;

  /*
  Description:
    Low level archive writing tool used by ON_BinaryArchive::ReadObject().
  Parameters:
    binary_archive - archive to read from
  Returns:
    Returns TRUE if the read is successful.
  Remarks:
    Use ON_BinaryArchive::ReadObject() to read objects.
    This Read() function should read the objects definition back into
    its data members.

    The default implementation of this virtual function returns
    FALSE and does nothing.
  */
  virtual
  BOOL Read(
         ON_BinaryArchive& binary_archive
       );

  /*
  Description:
    Useful for switch statements that need to differentiate
    between basic object types like points, curves, surfaces,
    and so on.

  Returns:
    ON::object_type enum value.

    @untitled table
    ON::unknown_object_type      unknown object
    ON::point_object             derived from ON_Point
    ON::pointset_object          some type of ON_PointCloud, ON_PointGrid, ...
    ON::curve_object             derived from ON_Curve
    ON::surface_object           derived from ON_Surface
    ON::brep_object              derived from ON_Brep
    ON::mesh_object              derived from ON_Mesh
    ON::layer_object             derived from ON_Layer
    ON::material_object          derived from ON_Material
    ON::light_object             derived from ON_Light
    ON::annotation_object        derived from ON_Annotation,
    ON::userdata_object          derived from ON_UserData
    ON::text_dot                 derived from ON_TextDot

  Remarks:
    The default implementation of this virtual function returns
    ON::unknown_object_type
  */
  virtual
  ON::object_type ObjectType() const;



  /*
  Description:
    All objects in an opennurbs model have an id
    ( ON_Layer.m_layer_id, ON_Font.m_font_id,
      ON_Material.m_material_id, ON_3dmObjectAttributes.m_uuid
      ).
  Returns:
    The id used to identify the object in the openurbs model.
  */
  virtual
  ON_UUID ModelObjectId() const;

  /////////////////////////////////////////////////////////////////
  //
  // User data provides a standard way for extra information to
  // be attached to any class derived from ON_Object.  The attached
  // information can persist and be transformed.  If you use user
  // data, please carefully read all the comments from here to the
  // end of the file.
  //

  /*
  Description:
    Attach a user string to an object.  This information will
    perisist through copy construction, operator=, and file IO.
  Parameters:
    key - [in] id used to retrieve this string.
    string_value - [in]
      If NULL, the string with this id will be removed.
  Returns:
    True if successful.
  */
  bool SetUserString(
    const wchar_t* key,
    const wchar_t* string_value
    );

  /*
  Description:
    Get user string from an object.
  Parameters:
    key - [in] id used to retrieve the string.
    string_value - [out]
  Returns:
    True if a string with id was found.
  */
  bool GetUserString(
    const wchar_t* key,
    ON_wString& string_value
    ) const;

  /*
  Description:
    Get a list of all user strings on an object.
  Parameters:
    user_strings - [out]
      user strings are appended to this list.
  Returns:
    Number of elements appended to the user_strings list.
  */
  int GetUserStrings(
    ON_ClassArray<ON_UserString>& user_strings
    ) const;

  /*
  Description:
    Get a list of all user string keys on an object.
  Parameters:
    user_string_keys - [out]
      user string keys are appended to this list.
  Returns:
    Number of elements appended to the user_strings list.
  */
  int GetUserStringKeys(
    ON_ClassArray<ON_wString>& user_string_keys
    ) const;

  /*
  Description:
    Attach user data to an object.
  Parameters:
    pUserData - [in] user data to attach to object.
        The ON_UserData pointer passed to AttachUserData()
        must be created with new.
  Returns:
    If TRUE is returned, then ON_Object will delete the user
    data when appropriate.  If FALSE is returned, then data
    could not be attached and caller must delete.
  Remarks:
    AttachUserData() will fail if the user data's m_userdata_uuid
    field is nil or not unique.
  */
  BOOL AttachUserData(
          ON_UserData* pUserData
          );

  /*
  Description:
    Remove user data from an object.
  Parameters:
    pUserData - [in] user data to attach to object.
        The ON_UserData pointer passed to DetachUserData()
        must have been previously attached using
        AttachUserData().
  Returns:
    If TRUE is returned, then the user data was
    attached to this object and it was detached.  If FALSE
    is returned, then the user data was not attached to this
    object to begin with.  In all cases, you can be assured
    that the user data is no longer attached to "this".
  Remarks:
    Call delete pUserData if you want to destroy the user data.
  */
  BOOL DetachUserData(
          ON_UserData* pUserData
          );


  /*
  Description:
    Get a pointer to user data.
  Parameters:
    userdata_uuid - [in] value of the user data's
       m_userdata_uuid field.
  Remarks:
    The returned user data is still attached to the object.
    Deleting the returned user data will automatically remove
    the user data from the object.
  */
  ON_UserData* GetUserData(
          const ON_UUID& userdata_uuid
          ) const;

  /*
  Description:
    PurgeUserData() removes all user data from object.
  Remarks:
    Use delete GetUserData(...) to destroy a single piece
    of user data.
  */
  void PurgeUserData();

  /*
  Description:
    User data is stored as a linked list of ON_UserData
    classes.  FirstUserData gets the first item in the
    linked list.  This is the most recent item attached
    using AttachUserData().
  Remark:
    To iterate through all the user data on an object,
    call FirstUserData() and then use ON_UserData::Next()
    to traverse the list.
  */
  ON_UserData* FirstUserData() const;

  /*
  Description:
    Objects derived from ON_Geometry must call
    TransformUserData() in their Transform() member function.
  Parameters:
    xform - [in] transformation to apply to user data
  */
  void TransformUserData(
    const ON_Xform& xform
    );

  /*
  Description:
    Expert user tool that copies user data that has a positive
    m_userdata_copycount from the source_object to this.
  Parameters:
    source_object - [in] source of user data to copy
  Remarks:
    Generally speaking you don't need to use CopyUserData().
    Simply rely on ON_Object::operator=() or the copy constructor
    to do the right thing.
  */
  void CopyUserData(
    const ON_Object& source_object
    );

  /*
  Description:
    Expert user tool Moves user data from source_object
    to this, including user data with a nil m_userdata_copycount.
    Deletes any source user data with a duplicate m_userdata_uuid
    on this.
  */
  void MoveUserData(
    ON_Object& source_object
    );


  /////////////////////////////////////////////////////////////////
  //
  // Expert interface
  //

  /*
  Description:
    Expert user function.  If you are using openNURBS in its
    default configuration to read and write 3dm archives,
    you never need to call this function.
    Many objects employ lazy creation of (runtime) caches
    that save information to help speed geometric calculations.
    This function will destroy all runtime information.
  Parameters:
    bDelete - [in] if true, any cached information is properly
                   deleted.  If false, any cached information
                   is simply discarded.  This is useful when
                   the cached information may be in alternate
                   memory pools that are managed in nonstandard
                   ways.
  */
  virtual
  void DestroyRuntimeCache( bool bDelete = true );

private:
  friend int ON_BinaryArchive::ReadObject( ON_Object** );
  friend bool ON_BinaryArchive::WriteObject( const ON_Object& );
  friend bool ON_BinaryArchive::ReadObjectUserData( ON_Object& );
  friend bool ON_BinaryArchive::WriteObjectUserData( const ON_Object& );
  friend class ON_UserData;
  ON_UserData* m_userdata_list;
};

#endif

