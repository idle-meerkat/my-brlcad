/*
 *		I - D . C
 *
 *  Convert shorts to doubles.
 *
 *	% i-d [-n || scale]
 *
 *	-n will normalize the data (scale between -1.0 and +1.0).
 *
 *  Phil Dykstra - 5 Nov 85.
 */
#include "conf.h"

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "externs.h"		/* For atof, if math.h doesn't have it */

short	ibuf[512];
double	obuf[512];

static char usage[] = "\
Usage: i-d [-n || scale] < shorts > doubles\n";

main( argc, argv )
int	argc;
char	**argv;
{
	int	i, num;
	double	scale;

	scale = 1.0;

	if( argc > 1 ) {
		if( strcmp( argv[1], "-n" ) == 0 )
			scale = 1.0 / 32768.0;
		else
			scale = atof( argv[1] );
		argc--;
	}

	if( argc > 1 || scale == 0 || isatty(fileno(stdin)) ) {
		fputs( usage, stderr );
		exit( 1 );
	}

	while( (num = fread( &ibuf[0], sizeof( ibuf[0] ), 512, stdin)) > 0 ) {
		if( scale == 1.0 ) {
			for( i = 0; i < num; i++ )
				obuf[i] = ibuf[i];
		} else {
			for( i = 0; i < num; i++ )
				obuf[i] = (double)ibuf[i] * scale;
		}
		fwrite( &obuf[0], sizeof( obuf[0] ), num, stdout );
	}
}
