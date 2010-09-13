/*
 * NAME:
 * qc_auto_obs_read.c
 *
 * PURPOSE:
 * To read ASCII files containing observations from automtic stations with
 * global radiation measurements.
 *
 * NOTES:
 * See qc_auto.c
 *
 * BUGS:
 * Year is only specified using two digits - care has to be taken...
 *
 * RETURN VALUES:
 * 0 - normal and correct ending
 * 1 - i/o problem
 * 2 - memory problem
 *
 * DEPENDENCIES:
 *
 * AUTHOR:
 * �ystein God�y, DNMI/FOU, 01/11/2000
 *
 * MODIFIED:
 * �ystein God�y, DNMI/FOU, 07.07.2003
 * �ystein God�y, METNO/FOU, 01.04.2010: Adapted for new format while
 * awaiting new validation setup using libfmcol.
 *
 * VERSION:
 * $Id$
 */
 
#include <fluxval_readobs.h>
#include <string.h>
  
short qc_auto_obs_read(int year, short month, stlist stl, stdata **std) {

    char *infile, *path = DATAPATH, *dummy;
    char *errmsg="ERROR(qc_auto_obs): ";
    char *errmem="Could not allocate memory"; 
    char *errio="Could not read data";
    char *errpl="Incorrect parameterlist";
    /* while testing
    char *pl1="TTM       TTN       TTX       TJM     TJM20     TJM50       ";
    char *pl2="UUM       UUX        RR       FM2       FG2       FX2        ";
    char *pl3="QO        BT       TGM       TGN       TGX        ST        TT";
    */
    char *pl1="TTM   TTN   TTX   TJM TJM20 TJM50 ";
    char *pl2="UUM UUX     RR   FM2   FG2   FX2     ";
    char *pl3="QO   BT  TGM   TGN   TGX  ST";
    char *pl;
    short i, sy;
    int j;
    FILE *fp;

    /*
     * Change year specification to the required 2 digits...
     */
    if (year < 2000) {
	sy = (short) (year-1900);
    } else {
	sy = (short) (year-2000);
    }

    /*
     * Allocate memory required.
     */
    infile = (char *) malloc(FILELEN*sizeof(char));
    if (!infile) {
	fprintf(stderr, " %s %s\n", errmsg, errmem);
	return(2);
    }
    if (create_stdata(std, stl.cnt)) {
	clear_stdata(std, stl.cnt);
	return(2);
    }
    dummy = (char *) malloc(OBSRECLEN*sizeof(char));
    if (!dummy) {
	clear_stdata(std, stl.cnt);
	fprintf(stderr, " %s %s\n", errmsg, errmem);
	return(2);
    }
    pl = (char *) malloc(OBSRECLEN*sizeof(char));
    if (!pl) {
	clear_stdata(std, stl.cnt);
	fprintf(stderr, " %s %s\n", errmsg, errmem);
	return(2);
    }
    sprintf(pl,"%s%s%s",pl1,pl2,pl3);


    for (i=0; i<stl.cnt; i++) {
	/*
	 * Create filenames to read using year, month and station number
	 * specification (mm0sssss.cyy).
	 */
	sprintf(infile,"%s%02d0%05d.c%02d",path,month,stl.id[i].number,sy);
	fprintf(stdout," Reading autostation file: %s\n", infile);

	/*
	 * Open the specified list of stations and read data into data
	 * structure.  Must read first line and the deceide how to read
	 * data (number of parameters vary). 
	 */
	fp = fopen(infile,"r");
	if (!fp) {
	    fprintf(stderr," %s %s %s\n", errmsg,infile,
		"is missing");
	    (*std)[i].missing = 1;
	    continue;
	}

	if (!fgets(dummy, OBSRECLEN, fp)) {
	    fprintf(stderr, " %s %s\n", errmsg, errio);
	    return(1);
	}
	if (!strstr(dummy,pl)) {
	    fprintf(stderr, " %s %s\n", errmsg, errpl);
	    fprintf(stderr,"\t%s\n\t%s\n",dummy,pl);
	    return(1);
	}

	(*std)[i].id = stl.id[i].number;
	j = 0;
	while (fgets(dummy, OBSRECLEN, fp)) {
	    sscanf(dummy,
		"%s%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f%f",
		(*std)[i].param[j].date,
		&((*std)[i]).param[j].TTM,&((*std)[i]).param[j].TTN,
		&((*std)[i]).param[j].TTX,
		&((*std)[i]).param[j].TJM10,&((*std)[i]).param[j].TJM20,
		&((*std)[i]).param[j].TJM50,
		&((*std)[i]).param[j].UUM,&((*std)[i]).param[j].UUX,
		&((*std)[i]).param[j].RR,
		&((*std)[i]).param[j].FM2,&((*std)[i]).param[j].FG2,
		&((*std)[i]).param[j].FX2,
		&((*std)[i]).param[j].Q0,&((*std)[i]).param[j].BT,
		&((*std)[i]).param[j].TGM,&((*std)[i]).param[j].TGN,
		&((*std)[i]).param[j].TGX,
		&((*std)[i]).param[j].ST,
		&((*std)[i]).param[j].TT); 

	    if ((*std)[i].param[j].TTM > 100000000.) {
		(*std)[i].param[j].TTM = -999.;
	    } 
	    if ((*std)[i].param[j].TTN > 100000000.) {
		(*std)[i].param[j].TTN = -999.;
	    } 
	    if ((*std)[i].param[j].TTX > 100000000.) {
		(*std)[i].param[j].TTX = -999.;
	    } 
	    if ((*std)[i].param[j].TJM10 > 100000000.) {
		(*std)[i].param[j].TJM10 = -999.;
	    } 
	    if ((*std)[i].param[j].TJM20 > 100000000.) {
		(*std)[i].param[j].TJM20 = -999.;
	    } 
	    if ((*std)[i].param[j].TJM50 > 100000000.) {
		(*std)[i].param[j].TJM50 = -999.;
	    } 
	    if ((*std)[i].param[j].UUM > 100000000.) {
		(*std)[i].param[j].UUM = -999.;
	    } 
	    if ((*std)[i].param[j].UUX > 100000000.) {
		(*std)[i].param[j].UUX = -999.;
	    } 
	    if ((*std)[i].param[j].RR > 100000000.) {
		(*std)[i].param[j].RR = -999.;
	    } 
	    if ((*std)[i].param[j].FM2 > 100000000.) {
		(*std)[i].param[j].FM2 = -999.;
	    } 
	    if ((*std)[i].param[j].FG2 > 100000000.) {
		(*std)[i].param[j].FG2 = -999.;
	    } 
	    if ((*std)[i].param[j].FX2 > 100000000.) {
		(*std)[i].param[j].FX2 = -999.;
	    } 
	    if ((*std)[i].param[j].Q0 > 100000000.) {
		(*std)[i].param[j].Q0 = -999.;
	    } 
	    if ((*std)[i].param[j].BT > 100000000.) {
		(*std)[i].param[j].BT = -999.;
	    } 
	    if ((*std)[i].param[j].TGM > 100000000.) {
		(*std)[i].param[j].TGM = -999.;
	    } 
	    if ((*std)[i].param[j].TGN > 100000000.) {
		(*std)[i].param[j].TGN = -999.;
	    } 
	    if ((*std)[i].param[j].TGX > 100000000.) {
		(*std)[i].param[j].TGX = -999.;
	    } 
	    if ((*std)[i].param[j].ST > 100000000.) {
		(*std)[i].param[j].ST = -999.;
	    } 
	    if ((*std)[i].param[j].TT > 100000000.) {
		(*std)[i].param[j].TT = -999.;
	    }
	    j++;
	}

	fclose(fp);
    }

    /*
     * Clean up before function is left.
     */
    free(infile);
    free(dummy);
    free(pl);
    return(FM_OK);
}

int create_stdata(stdata **pt, int size) {
    int i, j;

    *pt = (stdata *) malloc(size*sizeof(stdata));
    if (!(*pt)) return(2);

    for (i=0; i<size; i++) {
	(*pt)[i].missing = 0;
	(*pt)[i].param = (parlist *) malloc(NO_MONTHOBS*sizeof(parlist));
	if (!(*pt)[i].param) {
	    free(*pt);
	    return(2);
	}
	for (j=0; j<NO_MONTHOBS; j++) {
	    sprintf((*pt)[i].param[j].date,"xxxxxxxxxxxx");
	    (*pt)[i].param[j].TTM=-999.;
	    (*pt)[i].param[j].TTN=-999.;
	    (*pt)[i].param[j].TTX=-999.;
	    (*pt)[i].param[j].TJM10=-999.;
	    (*pt)[i].param[j].TJM50=-999.;
	    (*pt)[i].param[j].UUM=-999.;
	    (*pt)[i].param[j].UUX=-999.;
	    (*pt)[i].param[j].RR=-999.;
	    (*pt)[i].param[j].FM2=-999.;
	    (*pt)[i].param[j].FG2=-999.;
	    (*pt)[i].param[j].FX2=-999.;
	    (*pt)[i].param[j].Q0=-999.;
	    (*pt)[i].param[j].BT=-999.;
	    (*pt)[i].param[j].TGM=-999.;
	    (*pt)[i].param[j].TGN=-999.;
	    (*pt)[i].param[j].TGX=-999.;
	    (*pt)[i].param[j].ST=-999.;
	    (*pt)[i].param[j].TT=-999.;
	}
    }

    return(FM_OK);
}

int clear_stdata(stdata **pt, int size) {
    int i;

    for (i=0; i<size; i++) {
	free((*pt)[i].param);
    }
    free(*pt);

    return(FM_OK);
}

