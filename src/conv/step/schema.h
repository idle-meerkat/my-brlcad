#ifndef  SCHEMA_H
#define  SCHEMA_H
// This file was generated by fedex_plus.  You probably don't want to edit
// it since your modifications will be lost if fedex_plus is used to
// regenerate it.
/* $Id$ */
#ifdef SCL_LOGGING
#include <sys/time.h>
#endif

#ifdef __OSTORE__
#include <ostore/ostore.hh>    // Required to access ObjectStore Class Library
#endif

#ifdef __O3DB__
#include <OpenOODB.h>
#endif

#include <sdai.h>


#include <Registry.h>

#include <STEPaggregate.h>

#include <STEPundefined.h>

#include <ExpDict.h>

#include <STEPattribute.h>

#include <Sdaiclasses.h>
extern void SchemaInit (Registry &);
extern void InitSchemasAndEnts (Registry &);
#include <SdaiCONFIG_CONTROL_DESIGN.h> 
extern void SdaiCONFIG_CONTROL_DESIGNInit (Registry & r);

#ifdef __OSTORE__
#include <osdb_SdaiCONFIG_CONTROL_DESIGN.h> 
#endif


#include <complexSupport.h>
ComplexCollect *gencomplex();

SCLP23(Model_contents_ptr) GetModelContents(char *schemaName);
#endif