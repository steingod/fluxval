/*
 * NAME:
 * fluxval_readobs.c
 *
 * PURPOSE:
 * To read ASCII files containing observations from automatic stations with
 * global radiation measurements.
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
 * Øystein Godøy, DNMI/FOU, 01/11/2000
 *
 * MODIFIED:
 * Øystein Godøy, DNMI/FOU, 07.07.2003
 * Øystein Godøy, METNO/FOU, 01.04.2010: Adapted for new format while
 * awaiting new validation setup using libfmcol.
 * Øystein Godøy, METNO/FOU, 2011-02-11: Changed interface, changed return
 * codes to comply with libfmutil.
 * Øystein Godøy, METNO/FOU, 2014-08-21: Added reading of observations
 * from decoded WMO GTS BUFR files.
 */

#include <fluxval_readobs.h>
#include <string.h>

/*
 * Bioforsk data in original format, prior to ingestion in KDVH. Only used
 * for historical data now. Øystein Godøy, METNO/FOU, 2014-08-21 
 */
int fluxval_readobs(char *path, int year, short month, stlist stl, stdata **std) {

    char *where="fluxval_readobs";
    char *infile, *dummy;
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
        fmerrmsg(where,"Could not allocate infile");
        return(FM_MEMALL_ERR);
    }
    if (create_stdata(std, stl.cnt)) {
        clear_stdata(std, stl.cnt);
        return(FM_IO_ERR);
    }
    dummy = (char *) malloc(OBSRECLEN*sizeof(char));
    if (!dummy) {
        clear_stdata(std, stl.cnt);
        fmerrmsg(where,"Could not allocate dummy");
        return(FM_MEMALL_ERR);
    }
    pl = (char *) malloc(OBSRECLEN*sizeof(char));
    if (!pl) {
        clear_stdata(std, stl.cnt);
        fmerrmsg(where,"Could not allocate pl");
        return(FM_MEMALL_ERR);
    }
    sprintf(pl,"%s%s%s",pl1,pl2,pl3);


    for (i=0; i<stl.cnt; i++) {
        /*
         * Create filenames to read using year, month and station number
         * specification (mm0sssss.cyy).
         */
        sprintf(infile,"%s/%02d0%05d.c%02d",path,month,stl.id[i].number,sy);
        fprintf(stdout," Reading autostation file: %s\n", infile);

        /*
         * Open the specified list of stations and read data into data
         * structure.  Must read first line and the deceide how to read
         * data (number of parameters vary). 
         */
        fp = fopen(infile,"r");
        if (!fp) {
            fmerrmsg(where,"Could not open %s", infile);
            (*std)[i].missing = 1;
            continue;
        }

        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!strstr(dummy,pl)) {
            fmerrmsg(where,"Incorrect parameter list\ngot: %s\nexpected: %s",
                    dummy, pl);
            return(FM_IO_ERR);
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

/*
 * IPY data
 * These files are generated by the R-package ncradflux function
 * averadflux. The original data are minute data which are transformed
 * into hourly data using averadflux. 
 * Check https://github.com/steingod/R-ncradflux for details.
 */
int fluxval_readobs_ascii(char *path, int year, short month, stlist stl, stdata **std) {

    char *where="fluxval_readobs";
    char *infile, *dummy;
    char dummytime[FMSTRING32], dummytime2[FMSTRING32];
    char *pl="\"time\" \"mssi\" \"nssi\" \"mdli\" \"ndli\"";
    short i;
    fmtime obstime;
    int j;
    FILE *fp;

    /*
     * Allocate memory required.
     */
    infile = (char *) malloc(FILELEN*sizeof(char));
    if (!infile) {
        fmerrmsg(where,"Could not allocate infile");
        return(FM_MEMALL_ERR);
    }
    if (create_stdata(std, stl.cnt)) {
        clear_stdata(std, stl.cnt);
        return(FM_IO_ERR);
    }
    dummy = (char *) malloc(OBSRECLEN*sizeof(char));
    if (!dummy) {
        clear_stdata(std, stl.cnt);
        fmerrmsg(where,"Could not allocate dummy");
        return(FM_MEMALL_ERR);
    }

    for (i=0; i<stl.cnt; i++) {
        /*
         * Create filenames to read using year, month and station number
         * specification (mm0sssss.cyy).
         */
        sprintf(infile,"%s/radflux_%s_%4d%02d.txt",
                path,stl.id[i].name,year,month);
        fprintf(stdout," Reading autostation file: %s\n", infile);

        /*
         * Open the specified list of stations and read data into data
         * structure.  Must read first line and the deceide how to read
         * data (number of parameters vary). 
         */
        fp = fopen(infile,"r");
        if (!fp) {
            fmerrmsg(where,"Could not open %s", infile);
            (*std)[i].missing = 1;
            continue;
        }

        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!strstr(dummy,pl)) {
            fmerrmsg(where,"Incorrect parameter list\ngot: %s\nexpected: %s",
                    dummy, pl);
            return(FM_IO_ERR);
        }

        (*std)[i].id = stl.id[i].number;
        j = 0;
        while (fgets(dummy, OBSRECLEN, fp)) {
            sscanf(dummy,
                    "%s%s%f%*f%f%*f",
                    dummytime, dummytime2,
                    &((*std)[i]).param[j].Q0,
                    &((*std)[i]).param[j].LW); 
            strcat(dummytime," ");
            strcat(dummytime, dummytime2);
            printf("[%s]-[%s]\n", dummytime,dummytime2);
            fmstring2fmtime(dummytime,"YYYY-MM-DD hh:mm:ss",&obstime);
            sprintf((*std)[i].param[j].date,"%4d%02d%02d%02d%02d%02d",
                    obstime.fm_year,obstime.fm_mon,obstime.fm_mday,
                    obstime.fm_hour,obstime.fm_min,obstime.fm_sec);
            j++;
        }
    }

    /*
     * Clean up before function is left.
     */
    free(infile);
    free(dummy);
    return(FM_OK);
}

/*
 * Read data extracted from Ulric, the URL interface to KDVH
 * Currently only air temperature, global radiation and sunshine duration
 * is expected for this on an hourly basis.
 */
int fluxval_readobs_ulric(char *path, int year, short month, stlist stl, stdata **std) {

    char *where="fluxval_readobs";
    char *infile, *dummy;
    char dummytime[FMSTRING32], dummytime2[FMSTRING32];
    char *pl="# Time TA QO OT_1";
    short i;
    fmtime obstime;
    int j;
    FILE *fp;

    /*
     * Allocate memory required.
     */
    infile = (char *) malloc(FILELEN*sizeof(char));
    if (!infile) {
        fmerrmsg(where,"Could not allocate infile");
        return(FM_MEMALL_ERR);
    }
    if (create_stdata(std, stl.cnt)) {
        clear_stdata(std, stl.cnt);
        return(FM_IO_ERR);
    }
    dummy = (char *) malloc(OBSRECLEN*sizeof(char));
    if (!dummy) {
        clear_stdata(std, stl.cnt);
        fmerrmsg(where,"Could not allocate dummy");
        return(FM_MEMALL_ERR);
    }

    for (i=0; i<stl.cnt; i++) {
        /*
         * Create filenames to read using year, month and station number
         * specification (mm0sssss.cyy).
         */
        sprintf(infile,"%s/radflux_%d_%4d%02d.txt",
                path,stl.id[i].number,year,month);
        fprintf(stdout," Reading autostation file: %s\n", infile);

        /*
         * Open the specified list of stations and read data into data
         * structure.  Must read first line and the deceide how to read
         * data (number of parameters vary). 
         */
        fp = fopen(infile,"r");
        if (!fp) {
            fmerrmsg(where,"Could not open %s", infile);
            (*std)[i].missing = 1;
            continue;
        }

        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!strstr(dummy,pl)) {
            fmerrmsg(where,"Incorrect parameter list\n\tgot: %s\n\texpected: %s",
                    dummy, pl);
            return(FM_IO_ERR);
        }

        (*std)[i].id = stl.id[i].number;
        j = 0;
        while (fgets(dummy, OBSRECLEN, fp)) {
            sscanf(dummy,
                    "%s%f%f%f",
                    dummytime,
                    &((*std)[i]).param[j].TTM,
                    &((*std)[i]).param[j].Q0,
                    &((*std)[i]).param[j].ST); 
            /*
            strcat(dummytime," ");
            strcat(dummytime, dummytime2);
            */
            fmstring2fmtime(dummytime,"YYYYMMDDThhmm",&obstime);
            sprintf((*std)[i].param[j].date,"%4d%02d%02d%02d%02d%02d",
                    obstime.fm_year,obstime.fm_mon,obstime.fm_mday,
                    obstime.fm_hour,obstime.fm_min,obstime.fm_sec);
            j++;
        }
    }

    /*
     * Clean up before function is left.
     */
    free(infile);
    free(dummy);
    return(FM_OK);
}

/* 
 * Read data extracted from the WMO GTS data stream in BUFR. A wrapper
 * around bufrdump.pl has been used to generate monthly ASCII files. All
 * files look the same regardless of which parameters that are available.
 * No header is applied in files.
 */
int fluxval_readobs_gts(char *path, int year, short month, stlist stl, stdata **std) {

    char *where="fluxval_readobs";
    char *infile, *dummy;
    char dummytime[FMSTRING32], dummytime2[FMSTRING32];
    char *pl=""; /* Not used currently */
    short i;
    fmtime obstime;
    int j;
    FILE *fp;

    /*
     * Allocate memory required.
     */
    infile = (char *) malloc(FILELEN*sizeof(char));
    if (!infile) {
        fmerrmsg(where,"Could not allocate infile");
        return(FM_MEMALL_ERR);
    }
    if (create_stdata(std, stl.cnt)) {
        clear_stdata(std, stl.cnt);
        return(FM_IO_ERR);
    }
    dummy = (char *) malloc(OBSRECLEN*sizeof(char));
    if (!dummy) {
        clear_stdata(std, stl.cnt);
        fmerrmsg(where,"Could not allocate dummy");
        return(FM_MEMALL_ERR);
    }

    for (i=0; i<stl.cnt; i++) {
        /*
         * Create filenames to read using year, month and station number
         * specification (mm0sssss.cyy).
         */
        sprintf(infile,"%s/radflux_%d_%4d%02d.txt",
                path,stl.id[i].number,year,month);
        fprintf(stdout," Reading autostation file: %s\n", infile);

        /*
         * Open the specified list of stations and read data into data
         * structure.  Must read first line and the deceide how to read
         * data (number of parameters vary). 
         */
        fp = fopen(infile,"r");
        if (!fp) {
            fmerrmsg(where,"Could not open %s", infile);
            (*std)[i].missing = 1;
            continue;
        }

        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!fgets(dummy, OBSRECLEN, fp)) {
            fmerrmsg(where,"Could not read data.");
            return(FM_IO_ERR);
        }
        if (!strstr(dummy,pl)) {
            fmerrmsg(where,"Incorrect parameter list\n\tgot: %s\n\texpected: %s",
                    dummy, pl);
            return(FM_IO_ERR);
        }

        (*std)[i].id = stl.id[i].number;
        j = 0;
        while (fgets(dummy, OBSRECLEN, fp)) {
            sscanf(dummy,
                    "%s%s%f%f%f",
                    dummytime, dummytime2,
                    &((*std)[i]).param[j].Q0,
                    &((*std)[i]).param[j].LW,
                    &((*std)[i]).param[j].ST); 
            strcat(dummytime," ");
            strcat(dummytime, dummytime2);
            fmstring2fmtime(dummytime,"YYYY-MM-DD hh:mm:ss",&obstime);
            sprintf((*std)[i].param[j].date,"%4d%02d%02d%02d%02d%02d",
                    obstime.fm_year,obstime.fm_mon,obstime.fm_mday,
                    obstime.fm_hour,obstime.fm_min,obstime.fm_sec);
            j++;
        }
    }

    /*
     * Clean up before function is left.
     */
    free(infile);
    free(dummy);
    return(FM_OK);
}

int create_stdata(stdata **pt, int size) {
    int i, j;

    *pt = (stdata *) malloc(size*sizeof(stdata));
    if (!(*pt)) return(FM_MEMALL_ERR);

    for (i=0; i<size; i++) {
        (*pt)[i].missing = 0;
        (*pt)[i].param = (parlist *) malloc(NO_MONTHOBS*sizeof(parlist));
        if (!(*pt)[i].param) {
            free(*pt);
            return(FM_MEMALL_ERR);
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
            (*pt)[i].param[j].LW=-999.;
        }
    }

    return(FM_OK);
}

int clear_stdata(stdata **pt, int size) {
    int i;

    if (size == 0) return(FM_OK);
    for (i=0; i<size; i++) {
        free((*pt)[i].param);
    }
    free(*pt);

    return(FM_OK);
}

