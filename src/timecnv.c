/*
 * NAME:
 * timecnv.c
 *
 * PURPOSE:
 * To convert int of type yyyymmddhhii to time struct.
 *
 * NOTES:
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
 * Øystein Godøy, DNMI/FOU, 
 *
 * VERSION:
 * $Id$
 */

#include <fluxval.h>

#define TIMLEN 13

short timecnv(char dummy1[], struct tm *time) {

    int i;
    char *dummy2;

    dummy2 = (char *) malloc(5*sizeof(char));
    if (!dummy2) return(2);

    /*
     * Initialize
     */
    for (i=0; i<5; i++) {
	dummy2[i] = '\0';
    }
    time->tm_sec = -9.;
    time->tm_min = -9.;
    time->tm_hour = -9.;
    time->tm_mday = -9.;
    time->tm_mon = -9.;
    time->tm_year = -9.;
    time->tm_wday = -9.;
    time->tm_yday = -9.;
    time->tm_isdst = -9.;

    /*
     * Get year
     */
    for (i=0;i<4;i++) {
	dummy2[i] = dummy1[i];
    }
    time->tm_year = atoi(dummy2);
    time->tm_year -= 1900;
    dummy2[2] = dummy2[3] = '\0';

    /*
     * Get month
     */
    for (i=0;i<2;i++) {
	dummy2[i] = dummy1[4+i];
    }
    time->tm_mon = atoi(dummy2);
    if (time->tm_mon < 1 || time->tm_mon > 12) {
	time->tm_mon = -9.; 
    } else {
	time->tm_mon--;
    }

    /*
     * Get day
     */
    for (i=0;i<2;i++) {
	dummy2[i] = dummy1[6+i];
    }
    time->tm_mday = atoi(dummy2);
    if (time->tm_mday < 1 || time->tm_mday > 31) time->tm_mday = -9.; 

    /*
     * Get hour
     */
    for (i=0;i<2;i++) {
	dummy2[i] = dummy1[8+i];
    }
    time->tm_hour = atoi(dummy2);
    if (time->tm_hour < 0 || time->tm_hour > 23) time->tm_hour = -9.; 

    free(dummy2);

    if (time->tm_min < 0 && time->tm_hour < 0 && time->tm_mday < 0 &&
	time->tm_mon < 0 && time->tm_year < 0) return(1);

    return(0);
}
