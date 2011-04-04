/*
 * NAME:
 * fluxval_readobs.h
 * 
 * PURPOSE:
 * Header file for reading of automatic stations.
 *
 * NOTES:
 * See qc_auto.c
 *
 * BUGS:
 *
 * RETURN VALUES:
 * 0 - normal and correct ending
 * 1 - i/o problem
 * 2 - memory problem
 *
 * DEPENDENCIES:
 *
 * AUTHOR:
 * Øystein Godøy, DNMI/FOU, 27/07/2000
 *
 * MODIFIED:
 * Øystein Godøy, DNMI/FOU, 07.07.2003
 * Øystein Godøy, METNO/FOU, 2011-02-11: Changed interface to function.
 * Øystein Godøy, METNO/FOU, 2011-03-03: Changed some prototypes.
 * Øystein Godøy, METNO/FOU, 2011-04-04: Added some parameters to be able
 * to use the same structure to hold IPY-data as Bioforsk data.
 *
 * ID:
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fmutil.h>

#define FILELEN 100
#define ST_NAMELEN 20
#define ST_RECLEN 1024
#define NO_MONTHOBS 750
#define OBSRECLEN 350

#define DATAPATH "/opdata/automat/data/"

typedef struct {
    int number;
    char *name;
    float lat;
    float lon;
} stid;
typedef struct {
    short cnt;
    stid *id;
} stlist;

typedef struct {
    char date[16];	/* Date info. */
    float TTM;		/* Mean air temp. */
    float TTN;		/* Min. air temp. */
    float TTX;		/* Max. air temp. */
    float TJM10;	/* Mean soil temp. 10cm */
    float TJM20;	/* Mean soil temp. 20cm */
    float TJM50;	/* Mean soil temp. 50cm */
    float UUM;		/* Mean relative humidity */
    float UUX;		/* Max. relative humidity */
    float RR;		/* Precipitation */
    float FM2;		/* Mean wind at 2m */
    float FG2;		/* Wind gust at 2m */
    float FX2;		/* Max. wind at 2m ? */
    float Q0;		/* Global radiation */
    float BT;		/* Leaves humidity time last hour ? */
    float TGM;		/* Mean grass temp. */
    float TGN;		/* Min. grass temp. */
    float TGX;		/* Max. grass temp. */
    float ST;		/* Solar time (minutes of Sun last hour) */
    float TT;		/* Air. temp. */
    float TG;		/* Grass temp. */
    float UU;		/* Relative humidity */
    float FF2;		/* Wind speed at 2m */
    float FF;		/* Wind speed (6m or 10m) */
    float DD;		/* Wind direction */
    float FM;		/* Mean wind speed */
    float DM;		/* Mean wind direction */
    float FG;		/* Wind gust */
    float DG;		/* Gust direction */
    float FX;		/* Max. wind speed */
    float DX;		/* Max. wind direction ?? */
    float ARR;		/* Extra precipitation instrument (A - for 2 here) */
    float RA;		/* Accumulated precipitation ? */
    float LW;           /* Longwave irradiance */
} parlist;

typedef struct {
    int id;
    parlist *param;
    short missing;
} stdata;

/*
 * Function prototypes.
 */
int decode_stlist(char *filename, stlist *stl);
int create_stlist(int size, stlist *pts);
int copy_stlist(stlist *lhs, stlist *rhs);
int clear_stlist(stlist *pts);
int create_stdata(stdata **pt, int size);
int clear_stdata(stdata **pt, int size);
int fluxval_readobs(char *path, int year, short month, stlist stl, stdata **std);
int fluxval_readobs_ascii(char *path, int year, short month, stlist stl, stdata **std); 
