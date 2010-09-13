/*
 * NAME:
 * return_product_area.h
 * 
 * PURPOSE:
 * Header file for return_product_area.c
 *
 * NOTES:
 * See return_product_area.c and qc_auto.c
 *
 * BUGS:
 * NA
 *
 * RETURN VALUES:
 *
 * DEPENDENCIES:
 *
 * AUTHOR:
 * Øystein Godøy, DNMI/FOU, 06/11/2000
 */

#include <stdio.h>
#include <stdlib.h>
#include <fmutil.h>
#include <safhdf.h>

#ifndef _RETURN_PRODUCT_AREA
#define _RETURN_PRODUCT_AREA

typedef struct {
    int iw;
    int ih;
    float *data;
} s_data;

#endif

