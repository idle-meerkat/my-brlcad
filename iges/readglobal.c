/*
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 *  Source -
 *	VLD/ASB Building 1065
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */

/*	Read Global Section 	*/

#include <stdio.h>
#include "./iges_struct.h"
#include "./iges_extern.h"

/* Conversion Factors (to mm)	*/
fastf_t cnv[]={
/*	default,inch, mm, ? , feet,  miles  ,meters,kilometers,mils  ,microns,
		cm ,microinches	*/
	1.0,    25.4,1.0,1.0,304.8,1609344.0,1000.0, 1000000.0,0.0254,0.001,
		10.0,0.0000254	};

/* IGES Version */
char *version[]={
	" ",
	"1.0 [NBS80]",
	"ANSI Y14.26M -1981 [ANSI81]",
	"2.0 [NBS83]",
	"3.0 [NBS86]",
	"ANSI Y14.26M - 1987 [ANSI88]",
	"4.0 [NBSIR 88-3813]" };

Readglobal()
{

	int field=2,i;
	fastf_t a;
	char *name;


	/* Get End-of-field delimiter */
	if( card[counter] != ',' )
	{
		counter--;
		while( card[++counter] == ' ' );
		if( card[counter] != '1' || card[++counter] != 'H' )
		{
			fprintf( stderr , "Error in new delimiter\n" );
			fprintf( stderr , "%s\n" , card );
			for( i=0 ; i<counter-1 ; i++ )
				putc( ' ' , stderr );
			putc( '^' , stderr );
			fprintf( stderr , "\n" );
			exit( 1 );
		}
		eof = card[++counter];
		while( card[++counter] != eof );
	}
	else
		eof = ',';


	/* Get End-of-record delimiter */
	if( card[++counter] != eof )
	{
		counter--;
		while( card[++counter] == ' ' );
		if( card[counter] != '1' || card[++counter] != 'H' )
		{
			fprintf( stderr , "Error in new record delimiter\n" );
			exit( 1 );
		}
		eor = card[++counter];
		while( card[++counter] != eof );
	}
	else
		eor = ';';


	/* Read all the fields in the Global Section */
	counter++;
	while( field < 23 )
	{
	   if( card[counter-1] == eor )
	   {
		Readrec( ++currec );
		break;
	   }

	   switch( ++field )
		{
		case 3:		Readname( &name , "Product ID: ");
				mk_id( fdout , name );
				free( name );
				break;
		case 4:		Readstrg( "File Name: " );
				break;
		case 5:		Readstrg( "System ID: " );
				break;
		case 6:		Readstrg( "Version: " );
				break;
		case 7:		Readint( &i , "Integer Bits: ");
				break;
		case 8:		Readint( &i , "Float Exponent Bits: " );
				break;
		case 9:		Readint( &i , "Float Mantissa Bits: " );
				break;
		case 10:	Readint( &i , "Double Precision Exp Bits: " );
				break;
		case 11:	Readint( &i , "Double Precision Mantissa: " );
				break;
		case 12:	Readstrg( "Product ID: ");
				break;
		case 13:	Readflt( &scale , "Scale: " );
				if( scale == 0.0 )
					scale = 1.0;
				inv_scale = 1.0/scale;
				break;
		case 14:	Readint( &units , "Units: " );
				if( units == 0 || units == 3 || units > 11 )
				{
					printf( "Unrecognized units, assuming 'mm'\n" );
					conv_factor = 1.0;
				}
				else
					conv_factor = cnv[units];
				/* make "conv_factor" account for both units and
					scale factor */
				conv_factor *= inv_scale;
				break;
		case 15:	Readstrg( "Units: " );
				break;
		case 16:	Readint( &i , "Line Weight Gradations: " );
				break;
		case 17:	Readflt( &a , "Line Width: " );
				break;
		case 18:	Readtime();
				break;
		case 19:	Readflt( &a , "Resolution: " );
				break;
		case 20:	Readflt( &a , "Maximum value: " );
				break;
		case 21:	Readstrg( "Author: " );
				break;
		case 22:	Readstrg( "Organization: " );
				break;
		case 23:	Readint( &i , "" );
				if( i<1 || i>6 )
					printf( "Unrecognized IGES version\n" );
				else
					printf( "IGES version: %s\n" , version[i] );
				break;
		}
	}
}

