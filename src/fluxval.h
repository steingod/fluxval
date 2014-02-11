/*
 * NAME:
 * fluxval.h
 * 
 * PURPOSE:
 * Header file for the software performing quality control of SSI data
 * against surface observations at agricultural stations in Norway (should
 * be extended in the future).
 *
 * NOTES:
 * See fluxval.c
 *
 * AUTHOR:
 * Øystein Godøy, DNMI/fou, 27/07/2000
 *
 * MODIFIED:
 * Øystein Godøy, METNO/FOU, 01.04.2010: Modified for use at laika.
 *
 * VERSION:
 * $Id$
 */

#ifndef _QC_AUTO_H
#define _QC_AUTO_H

/*
 * Header files that are required.
 */

/*
 * System files.
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/*
 * DNMI specific files.
 */
#include <fmutil.h>
#include <safprod.h>
#include <safhdf.h>
#include <fluxval_readobs.h>
#include <return_product_area.h>

/*
 * Variable definitions
 */
#define PATHLEN 100
#define FILENAMELEN 1024
#define PRODUCTPATH "/disk1/testdata/osisaf_data/output/flux/ssi/product/"
#define DAILYPATH "/disk1/testdata/osisaf_data/output/flux/ssi/daily/"
#define STARCPATH "/starc/DNMI_SAFOSI/"
#define MAXFILES 250
#define DEG2RAD PI/180.		/* Factor to multiply with to get radians */
#define RAD2DEG 180./PI		/* Factor to multiply with to get degrees */

typedef struct {
    int jd;
    int hh;
    int mi;
    short timecode;
    float reflon;
    fmgeopos pos;
} toain;

typedef struct {
    float sh;
    float extglo;
} toaout;

typedef struct {
    int yy_s;
    short mm_s;
    short dd_s;
    short hh_s;
    short mi_s;
    long ss_s;
    int yy_e;
    short mm_e;
    short dd_e;
    short hh_e;
    short mi_e;
    long ss_e;
    float Ax;
    float Ay;
    float Bx;
    float By;
    int iw;
    int ih;
} toa_m_in;

typedef struct {
    int yy;
    short mm;
    short dd;
    short hh;
    short mi;
    float Ax;
    float Ay;
    float Bx;
    float By;
    int iw;
    int ih;
} toa_a_in;

typedef struct {
    time_t time_unix;
    char filename[50];
} fns;

/*
 * Function prototypes.
 */
void usage(void);
/*
short toa_flux(toain indata, toaout *outdata);
short toa_mean(toa_m_in info, float **data);
*/
short timecnv(char tim[], struct tm *time);
int return_product_area(fmgeopos gpos, 
    PRODhead header, float *data, s_data *a); 
/*
 * End function prototypes.
 */
#endif /* _QC_AUTO_H */
