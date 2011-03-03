/*
 * NAME:
 * stlist.c
 * 
 * PURPOSE:
 * To create, copy and remove stationslists.
 *
 * NOTES:
 * This software was originally developed by Juergen Schulze, DNMI/FOU, later
 * it has been slightly adapted for the needs in the Ocean and Sea Ice SAF
 * project by Øystein Godøy. It requires a standard text file as input
 * containing the number of stations in the first line, then station name,
 * number, latitude and longitude on the subsequent lines...
 *
 * BUGS:
 * Proper typechecking of input data is not performed yet...
 *
 * RETURN VALUES:
 * 0 - normal and correct ending
 * 1 - i/o problem
 * 2 - memory problem
 * 3 - other
 *
 * DEPENDENCIES:
 *
 * AUTHOR:
 * Øystein Godøy, DNMI/FOU, 01/11/2000
 *
 * MODIFIED:
 * Øystein Godøy, METNO/FOU, 2011-03-03: Changed prototypes and return
 * values.
 *
 * VERSION:
 * $Id$
 */

#include <fluxval_readobs.h>

int decode_stlist(char *filename, stlist *stl) {
    char *dummy;
    int size, i;
    FILE *fp;

    dummy = (char *) malloc(ST_RECLEN*sizeof(char));
    if (!dummy) return(2);

    fp = fopen(filename,"r");
    if (!fp) return(1);

    if (!fgets(dummy, ST_RECLEN, fp)) return(1);
    sscanf(dummy,"%d", &size);
    if (create_stlist(size, stl) != 0) return(3); 
    stl->cnt = size; 

    i = 0;
    printf(" Using stations:\n");
    while (fgets(dummy,ST_RECLEN,fp)) {
	sscanf(dummy,"%s%d%f%f",
	    stl->id[i].name,&(stl->id[i].number),
	    &(stl->id[i].lat),&(stl->id[i].lon));
	printf(" %s %d %.2f %.2f\n", 
		stl->id[i].name,stl->id[i].number,
		stl->id[i].lat,stl->id[i].lon);
	i++;
    }

    fclose(fp);

    free(dummy);

    return(FM_OK);
}

int create_stlist(int size, stlist *pts) {
    int i;
    char *inistr="xxxxxxxxxxxxxxxxxxx\0";

    if (size > 0) {
	pts->id = (stid *) malloc(size*sizeof(stid));
	if (!pts->id) return(2);
	pts->cnt = size;
	for (i = 0; i < pts->cnt; i++){
	    pts->id[i].name = (char *) malloc(ST_NAMELEN*sizeof(char));
	    if (!pts->id[i].name) {
		clear_stlist(pts);
		return(1);
	    }
	    strcpy(pts->id[i].name, inistr);
	    pts->id[i].number = 0;
	    pts->id[i].lat = -999.0;
	    pts->id[i].lon = -999.0;
	}
    } else {
	clear_stlist(pts);
    }
    return(FM_OK);
}

int copy_stlist(stlist *lhs, stlist *rhs) { 
    int size;

    if (lhs->cnt) clear_stlist(lhs);

    if (rhs->cnt) {
	create_stlist(rhs->cnt, lhs);

	for (size = 0;  size < lhs->cnt; size++ ) {
	    lhs->id[size].name = 
		(char *) malloc(sizeof(char)*strlen(rhs->id[size].name)+1);
	    strcpy(lhs->id[size].name,rhs->id[size].name);
	    lhs->id[size].number = rhs->id[size].number;
	}
    } else {
	lhs->id = NULL;
    }

    return(FM_OK);
}

int clear_stlist(stlist *pts) { 
    int i;

    if (!pts->cnt) return(FM_MEMALL_ERR);

    for(i = 0; i < pts->cnt; i++) {
	free(pts->id[i].name);
    }

    pts->cnt = 0;
    free(pts->id);
    pts->id = NULL;
    return(FM_OK);
}

