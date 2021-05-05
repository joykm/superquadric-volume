#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <iostream>
#include <fstream>
using namespace std;

// setting the size of the grid
#define XMIN     -1.
#define XMAX      1.
#define YMIN     -1.
#define YMAX      1.

// setting value of the exponent
#define N	      .7

// setting the number of nodes for measuring volume:
#ifndef NUMNODES
#define NUMNODES    1000
#endif

// setting the number of threads:
#ifndef NUMT
#define NUMT        10
#endif

// how many tries to discover the maximum performance:
#ifndef NUMTRIES
#define NUMTRIES	10
#endif

float Height( int, int );	// function prototype

float Height( int iu, int iv )	// iu,iv = 0 .. NUMNODES-1
{
	float x = -1.  +  2.*(float)iu /(float)(NUMNODES-1);	// -1. to +1.
	float y = -1.  +  2.*(float)iv /(float)(NUMNODES-1);	// -1. to +1.

	float xn = pow( fabs(x), (double)N );
	float yn = pow( fabs(y), (double)N );
	float r = 1. - xn - yn;
	if( r <= 0. )
	        return 0.;
	float height = pow( r, 1./(float)N );
	return height;
}

// call this if you want to force your program to use
// a different random number sequence every time you run it:
void TimeOfDaySeed( )
{
	struct tm y2k = { 0 };
	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time_t  timer;
	time( &timer );
	double seconds = difftime( timer, mktime(&y2k) );
	unsigned int seed = (unsigned int)( 1000.*seconds );    // milliseconds
	srand( seed );
}

int main( int argc, char *argv[ ] )
{
    #ifndef _OPENMP
	    fprintf( stderr, "No OpenMP support!\n" );
	    return 1;
    #endif

	// the area of a single full-sized tile
	float fullTileArea = (  ( ( XMAX - XMIN )/(float)(NUMNODES-1) )  *
				( ( YMAX - YMIN )/(float)(NUMNODES-1) )  );    

    // seed the random number generator
    TimeOfDaySeed( );		

    // set number of threads to use for the for loop
    omp_set_num_threads( NUMT );

    // total volume of the superquadric
    float volume = 0.;
    float maxVolume = 0.;

    double maxPerformance = 0.;

    for (int tries = 0; tries < NUMTRIES; tries++) {
        
        double time0 = omp_get_wtime( );

        volume = 0.;

        // sum up the weighted heights into the variable "volume"  
        #pragma omp parallel for collapse(2) default(none), shared(fullTileArea), reduction(+:volume)
        for( int iv = 0; iv < NUMNODES; iv++ )
        {
            for( int iu = 0; iu < NUMNODES; iu++ )
            {
                float z = Height( iu, iv );

                // logic for fullTileArea
                if ((iv == 0 && iu == 0) || (iv == 0 && iu == NUMNODES - 1) || 
                    (iv == NUMNODES - 1 && iu == NUMNODES - 1) || 
                    (iv == NUMNODES - 1 && iu == 0)) {
                    volume = volume + ((fullTileArea * z) / 4);
                } else if (iv == 0 || iv == NUMNODES - 1 || iu == 0 ||
                    iu == NUMNODES - 1) {
                    volume = volume + ((fullTileArea * z) / 2);
                } else {
                    volume = volume + (fullTileArea * z);
                }
            }
        }

        double time1 = omp_get_wtime();
        double megaNodesPerSecond = (double)(NUMNODES * NUMNODES) / ( time1 - time0 ) / 1000000.;

        if( megaNodesPerSecond > maxPerformance )
			maxPerformance = megaNodesPerSecond;

        if( volume > maxVolume )
			maxVolume = volume;
    }

    fprintf(stderr, "%2d threads : %8d nodes ; volume = %.6f ; meganodes/sec = %6.2lf\n",
		NUMT, NUMNODES, volume * 2, maxPerformance);
}


