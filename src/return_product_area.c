/*
 * NAME:
 * return_product_area.c
 *
 * PURPOSE:
 * To return a square box surrounding a station position defined by latitude
 * and longitude. If the dimension of the requested data structure is 1x1 only
 * a single point is returned. 
 *
 * NOTES:
 * Only quadratic regions with odd dimension are supported yet. The
 * function should return viewing geometry angles as well in time.
 *
 * BUGS:
 * NA
 *
 * RETURN VALUES:
 * 0 - normal and correct ending
 * 1 - i/o problem
 * 2 - memory problem
 * 3 - other
 * 10 - no valid data
 *
 * DEPENDENCIES:
 *
 * AUTHOR:
 * Øystein Godøy, DNMI/FOU, 07/11/2000
 *
 * VERSION:
 * $Id$
 */

#include <return_product_area.h>

#define OUTOFIMAGE -40100
#define MISVAL -99999

int return_product_area(fmgeopos gpos, 
    PRODhead header, float *data, s_data *a) {

    char *where="return_product_area";
    int dx, dy, i, j, k;
    long l, maxsize;
    int nodata = 1;
    
    fmucsref uref;
    fmucspos upos;
    fmindex xyp;

    uref.Bx = header.Bx;
    uref.By = header.By;
    uref.Ax = header.Ax;
    uref.Ay = header.Ay;
    uref.iw = header.iw;
    uref.ih = header.ih;
    maxsize = header.iw*header.ih;

    upos = fmgeo2ucs(gpos, MI);
    xyp = fmucs2ind(uref, upos);

    /*
    printf("%.2f %.2f\n", gpos.lat, gpos.lon);
    printf("%.2f %.2f %.2f %.2f\n", uref.Bx, uref.By, uref.Ax, uref.Ay);
    printf("%4d %4d\n", uref.iw, uref.ih);
    printf("%.2f %.2f\n", upos.eastings, upos.northings);
    */

    if ((*a).iw == 1 && (*a).ih == 1) {
	*((*a).data) = data[fmivec(xyp.col,xyp.row,header.iw)];
	return(FM_OK);
    }

    if ((*a).iw%2 == 0 || (*a).ih%2 == 0) {
	fmerrmsg(where,
	"The area specified must contain an odd number of pixels.");
	return(FM_IO_ERR); 
    }

    dx = (int) floorf((float) (*a).iw/2.);
    dy = (int) floorf((float) (*a).ih/2.);
    /*
    printf("dy: %4d dx: %4d\n", dy, dx);
    */

    k = 0;
    for (i=(xyp.row-dy); i<=(xyp.row+dy); i++) {
	for (j=(xyp.col-dx); j<=(xyp.col+dx); j++) {
	    l = (int) fmivec(j,i,header.iw);
	    if (l >= maxsize) {
		fmerrmsg(where,
		"Image size (%d) exceeded when subsetting image (%d)",
		maxsize, l);
		return(FM_IO_ERR);
	    }
	    /*
	    printf("%4d %4d %3d %3d %.2f\n", k, l, i, j, data[l]);
	    */
	    if ((int) (floorf (data[l]*100.)) != OUTOFIMAGE &&
		(int) (floorf (data[l]*100.)) != MISVAL) {
		nodata = 0;
	    }
	    (*a).data[k] = data[l];
	    k++;
	}
    }

    if (nodata) {
	fmerrmsg(where,"No data were found.");
	return(FM_IO_ERR);
    }

    return(FM_OK);
}

