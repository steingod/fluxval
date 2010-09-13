/*
 * NAME:
 * fluxval_hour.c
 * 
 * PURPOSE:
 * To monitor the quality of SAF radiative flux products against
 * observations.
 *
 * NOTES:
 * In order to compare the estimated fluxes with observed, the syntax of the
 * software require a time period to be specified at command line. The program
 * will read the first observation file required (contains data for one month)
 * and then subsequent files as these are required...
 *
 * DEPENDENCIES:
 * o I/O functions for HDF5 file handling (including libhdf5)
 * o SAF product definitions
 *
 * BUGS:
 * Modification of time comparison is required along with some other changes. 
 * At present satellite and ground data are compared for the hour
 * following the  satellite observation only - this should be modified as
 * soon as possible. Check what the ground observation represents...
 *
 * Better qc of input parameters, especially start and end times should be
 * implemented...
 *
 * Dynamical allocation of space for filenames is not implemented. Some code is
 * prepared for realloc use, but it crashes. At present this is excluded and
 * should be persued further in the future...
 *
 * AUTHOR:
 * Øystein Godøy, DNMI/FOU, 27/07/2000
 * MODIFIED:
 * Øystein Godøy, DNMI/FOU, 10/10/2001
 * Adapted for use on LINUX.
 * Øystein Godøy, DNMI/FOU, 25/03/2002
 * Changing output format and controlling time comparison between obs and
 * sat. Control is only implemented for ns products at present (hardcoded
 * below...). It is assumed that observations are given in UTC time and
 * centered on the observation time...
 * Øystein Godøy, DNMI/FOU, 27/03/2002
 * Added satellite name and observation geometry to output...
 * Øystein Godøy, DNMI/FOU, 03/04/2002
 * Added cloud mask information to output, this requires cloud mask
 * information to be put in the SSI product area output...
 * Øystein Godøy, DNMI/FOU, 10/04/2002
 * Renamed qc_auto to qc_auto_hour to prepare for daily integration
 * version and rewrote qc_auto_hour to accept product area as commandline
 * input. I did also find an error in the stations input list. Position is
 * given as degrees, minutes and seconds in hundreths in the reference
 * book I have used (Klimaavdelingen) while I thought it was in decimal
 * degrees. This is now changed...
 * Øystein Godøy, DNMI/FOU, 07.07.2003
 * Better handling of missing observations...
 * Øystein Godøy, met.no/FOU, 13.05.2004
 * Changed specification of cloud type/amount information from PPS cloud
 * type product. Now a value between 1 and 2 is returned, 1 representing
 * cloud free and 2 overcast...
 *
 * VERSION:
 * $Id$
 */

#include <fluxval.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    extern char *optarg;
    char *where="fluxval_hour";
    char dir2read[FMSTRING512];
    char *outfile, *infile, *stfile, *parea, *fntest, *files[MAXFILES];
    char stime[11], etime[11], timeid[11], fn[50];
    int h, i, j, k, l, m, nf, anf, novalobs, cmobs, geomobs;
    short sflg = 0, eflg = 0, pflg =0, iflg = 0, oflg = 0, aflg = 0, status;
    short obsmonth;
    osihdf ipd;
    DIR *dirp;
    struct dirent *direntp;
    struct tm time_str, *time_ptr;
    time_t time_unix, time_start, time_end, time_tst;
    fmsec1970 tstart, tend;
    fmtime tstartfm, tendfm;
    fmstarclist starclist;
    fmfilelist filelist;
    fns fns_str[150];
    stlist stl;
    stdata **std;
    s_data sdata;
    fmgeopos gpos;
    float meanflux, meanvalues[3], *cmdata, meancm;
    float misval=-999.;
    FILE *fp;

    /* 
     * Decode command line arguments containing path to input files (one for
     * each area produced) and name (and path) of the output file.
     */
    while ((i = getopt(argc, argv, "as:e:p:i:o:")) != EOF) {
        switch (i) {
            case 's':
                strcpy(stime,optarg);
                sflg++;
                break;
            case 'e':
                strcpy(etime,optarg);
                eflg++;
                break;
            case 'p':
                parea = (char *) malloc(FILENAMELEN);
                if (!parea) exit(FM_MEMALL_ERR);
                if (sprintf(parea,"%s",optarg) < 0) exit(FM_IO_ERR);
                pflg++;
                break;
            case 'i':
                stfile = (char *) malloc(FILENAMELEN);
                if (!stfile) exit(FM_MEMALL_ERR);
                if (sprintf(stfile,"%s",optarg) < 0) exit(FM_IO_ERR);
                iflg++;
                break;
            case 'o':
                outfile = (char *) malloc(FILENAMELEN);
                if (!outfile) exit(FM_MEMALL_ERR);
                if (sprintf(outfile,"%s",optarg) < 0) exit(FM_IO_ERR);
                oflg++;
                break;
            case 'a':
                aflg++;
                break;
            default:
                usage();
                break;
        }
    }

    /*
     * Check if all necessary information was given at commandline.
     */
    if (!sflg || !eflg || !iflg || !oflg) usage();

    /*
     * Create character string to test filneames against to avoid
     * unnecessary processing...
     */
    fntest = (char *) malloc(FILENAMELEN);
    if (!fntest) exit(FM_MEMALL_ERR);
    sprintf(fntest,"%s.hdf5",parea);

    /*
     * Decode time specification of period.
     */
    if (timecnv(stime, &time_str) != 0) {
        fmerrmsg(where,"Could not decode time specification");
        exit(FM_OK);
    }
    time_start = mktime(&time_str);
    if (timecnv(etime, &time_str) != 0) {
        fmerrmsg(where,"Could not decode time specification");
        exit(FM_OK);
    }
    time_end = mktime(&time_str);

    /*
     * Decode station list information.
     */
    if (decode_stlist(stfile, &stl) != 0) {
        fmerrmsg(where," Could not decode station file.");
        exit(FM_OK);
    }

    /*
     * Loop through products stored at starc
     */
    tstart = ymdh2fmsec1970(stime,0);
    tend = ymdh2fmsec1970(etime,0);
    if (tofmtime(tstart,&tstartfm)) {
        fmerrmsg(where,"Could not decode start time to fmtime");
        exit(FM_IO_ERR);
    }
    if (tofmtime(tend,&tendfm)) {
        fmerrmsg(where,"Could not decode end time to fmtime");
        exit(FM_IO_ERR);
    }
    if (fmstarcdirs(tstartfm,tendfm,&starclist)) {
        fmerrmsg(where,"Could not create starcdirs to process.");
        exit(FM_IO_ERR);
    }

    /*
     * Open file to store results in
     */
    fp = fopen(outfile,"a");
    if (!fp) {
        fmerrmsg(where,"Could not open output file...");
        exit(FM_OK);
    }

    /*
     * Loop through data directories containing satellite estimates
     * Currently only either SSI or DLI can be read, but this could be
     * used in a more generic way in the future.
     *
     * But first some variables have to be prepared.
     */
    /*
     * Filename for flux products
     */
    infile = (char *) malloc(FILENAMELEN*sizeof(char));
    if (!infile) {
        fmerrmsg(where,"Could not allocate memory for filename");
        exit(FM_OK);
    }
    /*
     * Specifying the size of the data collection box. This should be
     * configurable in the future, but is hardcoded at present...
     */
    sdata.iw = 13;
    sdata.ih = 13;
    sdata.data = (float *) malloc((sdata.iw*sdata.ih)*sizeof(float));
    if (!sdata.data) {
        fmerrmsg(where,"Could not allocate memory");
        exit(FM_MEMALL_ERR);
    }
    obsmonth = 0;
    for (i=0;i<starclist.nfiles;i++) {
        sprintf(dir2read,"%s/%s/ssi",STARCPATH,starclist.dirname[i]);
        if (fmreaddir(dir2read, &filelist)) {
            fmerrmsg(where,"Could not read content of %s", 
                    dir2read);
            continue;
        }
        fmfilelist_sort(&filelist);
        fmlogmsg(where, 
                " Directory\n\t%s\n\tcontains\n\t%d files",filelist.path, filelist.nfiles);
        for (j=0;j<filelist.nfiles;j++) {
            if (strstr(filelist.filename[j],fntest)) {
                fmlogmsg(where,"Processing %s", filelist.filename[j]);
                /*
                 * Read the satellite derived data
                 */
                sprintf(infile,"%s/%s", dir2read,filelist.filename[j]);
                fmlogmsg(where, "Reading OSISAF product %s", infile);
                status = read_hdf5_product(infile, &ipd, 0);
                if (status != 0) {
                    fmerrmsg(where, "Could not read input file %s", infile);
                    exit(FM_OK);
                }
                /* while testing... */
                if (ipd.h.hour < 9 || ipd.h.hour > 13) {
                    free_osihdf(&ipd);
                    continue;
                }
                printf("Source: %s\n", ipd.h.source);
                printf("Product: %s\n", ipd.h.product);
                printf("Area: %s\n", ipd.h.area);
                printf("\t%4d-%02d-%02d %02d:%02d\n",
                        ipd.h.year, ipd.h.month, ipd.h.day, ipd.h.hour, ipd.h.minute);
                for (k=0; k<ipd.h.z; k++) {
                    printf("\tBand %d - %s\n", k, ipd.d[k].description);
                }

                /*
                 * Transform CM data to float array before further processing.
                 */
                if (ipd.h.z == 7 && strcmp(ipd.d[6].description,"CM") == 0) {
                    cmdata = (float *) malloc((ipd.h.iw*ipd.h.ih)*sizeof(float));
                    if (!cmdata) {
                        fmerrmsg(where,
                                "Could not allocate cmdata for %s", 
                                filelist.filename[j]);
                        exit(FM_MEMALL_ERR);
                    }
                    for (k=0;k<(ipd.h.iw*ipd.h.ih);k++) {
                        cmdata[k] = (float) ((unsigned short *) ipd.d[6].data)[k];
                    }
                }
                /*
                 * Get observations (if not already read)
                 */
                if (!aflg) {
                    if (obsmonth != ipd.h.month) {
                        if (obsmonth > 0) {
                            clear_stdata(std, stl.cnt);
                        }
                        std = (stdata **) malloc(sizeof(stdata *));
                        if (!std) {
                            fmerrmsg(where," Could not allocate memory");
                            exit(FM_OK);
                        }
                        fmlogmsg(where,
                                "Reading surface observations of radiative fluxes.");
                        if (qc_auto_obs_read(ipd.h.year,ipd.h.month, 
                                    stl, std) != 0) {
                            fmerrmsg(where,
                                    "Could not read autostation data\n");
                            exit(FM_OK);
                        }
                        obsmonth = ipd.h.month;
                    }
                }	
                /*
                 * Store collocated flux estimates and measurements in
                 * file. Below all available stations are looped for each
                 * satellite derived flux file.
                 *
                 * Much of the averaging below can be isolated in
                 * subroutines to make a nicer software outline...
                 */
                for (k=0; k<stl.cnt; k++) {

                    /*
                     * Each station listed is processed and data
                     * surrounding the stations is extracted and processed
                     * before storage in collocation file.
                     */
                    gpos.lat = stl.id[k].lat;
                    gpos.lon = stl.id[k].lon;
                    /*
                     * First the OSISAF flux data surrounding a station
                     * are extracted on a representative subarea.
                     */
                    fmlogmsg(where,
                            "Collecting OSISAF flux estimates around station %s",
                            stl.id[k].name);
                    if (return_product_area(gpos, ipd.h, ipd.d[0].data, &sdata) != FM_OK) {
                        fmerrmsg(where,
                                "Did not find valid flux data for station %s for flux file %s",
                                stl.id[k].name, filelist.filename[j]); 
                        continue;
                    }
                    /*
                     * Average flux estimates first
                     */
                    if (sdata.iw == 1 && sdata.ih == 1) {
                        meanflux = *(sdata.data);
                    } else {
                        /*
                         * Generate mean value from all satellite data
                         * and store this in collocated file for
                         * easier analysis. This could be changed in
                         * the future...
                         */
                        meanflux = 0.;
                        novalobs = 0;
                        for (l=0; l<(sdata.iw*sdata.ih); l++) {
                            if (sdata.data[l] >= 0) {
                                meanflux += sdata.data[l];
                                novalobs++;
                            }
                        }
                        meanflux /= (float) novalobs;
                    }

                    /*
                     * Needs info on observation geometry as well...
                     * This block should probably be extracted into a
                     * subroutine/function...
                     */
                    for (m=0;m<3;m++) {
                        if (return_product_area(gpos, ipd.h, 
                                    ipd.d[m+3].data, &sdata) != 0) {
                            fmerrmsg(where,
                                    " Did not find valid geom data for station %s %s",
                                    stl.id[k].name,
                                    "although flux data were found...");
                            continue;
                        }
                        /*
                         * Average obs geom estimates use val obs found for
                         * fluxes.
                         */
                        meanvalues[m] = 0.;
                        geomobs = 0;
                        if (sdata.iw == 1 && sdata.ih == 1) {
                            meanvalues[m] = *(sdata.data);
                        } else {
                            for (l=0; l<(sdata.iw*sdata.ih); l++) {
                                if (sdata.data[l] >= 0) {
                                    meanvalues[m] += sdata.data[l];
                                    geomobs++;
                                }
                            }
                            meanvalues[m] /= (float) geomobs;
                        }
                    }

                    /*
                     * Process the cloud mask information.
                     * This block should probably be extracted into a
                     * subroutine/function...
                     */
                    if ((ipd.h.z == 7) && 
                            (strcmp(ipd.d[6].description,"CM") == 0)) {
                        if (return_product_area(gpos, ipd.h, 
                                    cmdata, &sdata) != 0) {
                            fmerrmsg(where,
                                    " Did not find valid CM data for station %s %s\n",
                                    stl.id[j].name,
                                    "although flux data were found...");
                            continue;
                        }
                        /*
                         * Average CM used val obs found for fluxes.
                         */
                        meancm = 0.;
                        cmobs = 0;
                        if (sdata.iw == 1 && sdata.ih == 1) {
                            if (*(sdata.data) >= 0.99 && *(sdata.data) <= 4.01) {
                                meancm = 1;
                            } else if (*(sdata.data) >= 4.99 && *(sdata.data) <= 19.01) {
                                meancm = 2;
                            }
                        } else {
                            for (l=0; l<(sdata.iw*sdata.ih); l++) {
                                if (sdata.data[l] >= 0.99 && sdata.data[l] <= 4.01) {
                                    meancm += 1;
                                    cmobs++;
                                } else if (sdata.data[l] >= 4.99 && sdata.data[l] <= 19.01) {
                                    meancm += 2;
                                    cmobs++;
                                }

                            }
                            meancm /= (float) cmobs;
                        }
                    }

                    /*
                     * If only satellite data are to be extracted around
                     * the stations listed, use the following...
                     */
                    fmlogmsg(where,"Now data should have been written");
                    if (aflg) {
                        fmlogmsg(where,"And now they are written");
                        /*
                         * First print representative acquisition
                         * time for satellite based estimates.
                         */
                        fprintf(fp," %4d%02d%02d%02d%02d",
                                ipd.h.year,ipd.h.month,ipd.h.day,
                                ipd.h.hour,ipd.h.minute);
                        /*
                         * Dump satellite based estimates and
                         * auxiliary data. The satellite data are
                         * averaged over 13x13 pixels to
                         * compensate for positioning error of
                         * satellites and the different view
                         * perspective from ground and space.
                         */
                        fprintf(fp,
                                " %7.2f %3d %3d %s %.2f %.2f %.2f %.2f", 
                                meanflux, novalobs, 
                                (sdata.iw*sdata.ih),
                                ipd.h.source, 
                                meanvalues[0], 
                                meanvalues[1], 
                                meanvalues[2],
                                meancm);
                        /*
                         * Dump all information concerning
                         * observations.  If asynchoneous logging
                         * is done, dump placeholders for future
                         * in situ observations to be included.
                         */
                        fprintf(fp," %12s %5d %7.2f %7.2f %7.2f", 
                                "000000000000", 0,
                                misval,misval,misval);
                        /*
                         * Insert newline to mark record.
                         */
                        fprintf(fp,"\n");
                        continue;
                    }

                    /*
                     * If surface observations are available and to be
                     * stored in the same file, process these now...
                     */
                    if ((*std)[k].missing) continue;

                    /*
                     * Checking that sat and obs is from the same hour.
                     * According to Sofus Lystad the observations
                     * represents integration of the last hour, time is
                     * given in UTC. The date specification below might
                     * cause evening observations during month changes to
                     * be missed, but this is not a major problem...
                     */
                    if (ipd.h.minute > 10) {
                        if (ipd.h.hour == 23) {
                            sprintf(timeid,"%04d%02d%02d0000", 
                                    ipd.h.year, ipd.h.month, (ipd.h.day+1));
                        } else {
                            sprintf(timeid,"%04d%02d%02d%02d00", 
                                    ipd.h.year, ipd.h.month, ipd.h.day, (ipd.h.hour+1));
                        }
                    } else {
                        sprintf(timeid,"%04d%02d%02d%02d00", 
                                ipd.h.year, ipd.h.month, ipd.h.day, ipd.h.hour);
                    }
                    if (stl.id[k].number == (*std)[k].id) {
                        for (h=0; h<NO_MONTHOBS; h++) {
                            if (strncmp(timeid,(*std)[k].param[h].date,12) == 0) {
                                /*
                                 * First print representative acquisition
                                 * time for satellite based estimates.
                                 */
                                fprintf(fp," %4d%02d%02d%02d%02d",
                                        ipd.h.year,ipd.h.month,ipd.h.day,
                                        ipd.h.hour,ipd.h.minute);
                                /*
                                 * Dump satellite based estimates and
                                 * auxiliary data. The satellite data are
                                 * averaged over 13x13 pixels to
                                 * compensate for positioning error of
                                 * satellites and the different view
                                 * perspective from ground and space.
                                 */
                                fprintf(fp,
                                        " %7.2f %3d %3d %s %.2f %.2f %.2f %.2f", 
                                        meanflux, novalobs, 
                                        (sdata.iw*sdata.ih),
                                        ipd.h.source, 
                                        meanvalues[0], 
                                        meanvalues[1], meanvalues[2],
                                        meancm);
                                /*
                                 * Dump all information concerning
                                 * observations. 
                                 */
                                fprintf(fp," %12s %5d %7.2f %7.2f %7.2f", 
                                        (*std)[k].param[h].date, (*std)[k].id,
                                        (*std)[k].param[h].TTM,
                                        (*std)[k].param[h].Q0,
                                        (*std)[k].param[h].ST);
                                /*
                                 * Insert newline to mark record.
                                 */
                                fprintf(fp,"\n");
                            }
                        }
                    }
                }
                if ((ipd.h.z == 7) && (strcmp(ipd.d[6].description,"CM")
                            == 0)) {
                    free(cmdata);
                }
                free_osihdf(&ipd);
            }
        }
        fmfilelist_free(&filelist);
    }
    exit(0); /* while testing...*/

    /*
     * Loop through all passages available within the specified time period.
     */
    dirp = opendir(PRODUCTPATH);
    if (!dirp) {
        fmerrmsg(where, "Could not open directory\n %s for listing", PRODUCTPATH);
        exit(FM_OK);
    }
    fprintf(stdout, " Checking available files in\n %s\n", PRODUCTPATH);
    nf = 0;
    while ((direntp = readdir(dirp)) != NULL) {
        if (strstr(direntp->d_name,fntest) == NULL) continue;
        if (nf > (MAXFILES-1)) {
            fmerrmsg(where,"Increase MAXFILES variable in header file");
            exit(FM_OK);
        }
        files[nf] = (char *) malloc((strlen(direntp->d_name)+1)*sizeof(char));
        if (!files[nf]) {
            fmerrmsg(where,"Could not allocate memory for filenames");
            exit(FM_OK);
        }
        strcpy(files[nf],direntp->d_name);
        nf++;
    }
    closedir(dirp);
    free(fntest);

    fprintf(stdout," Found a total of %d files containing flux data.\n", nf);
    infile = (char *) malloc(FILENAMELEN*sizeof(char));
    if (!infile) {
        fmerrmsg(where,"Could not allocate memory for filename");
        exit(FM_OK);
    }
    anf = 0;

    for (i=0; i<nf; i++) {
        if (strstr(files[i],"ssi") == NULL ||
                strstr(files[i],"hdf5") == NULL) continue;
        /*
         * Get representation time
         */
        sprintf(infile,"%s%s", PRODUCTPATH,files[i]);
        status = read_hdf5_product(infile, &ipd, 1);
        if (status != 0) {
            fprintf(stdout," Could not use file\n %s\n", infile);
            continue;
        }

        /*
         * Check if within the allowed period, if so store header. This is 
         * easiest achieved if time is converted to UNIX time...
         */
        time_str.tm_year = ipd.h.year-1900;
        time_str.tm_mon = ipd.h.month-1;
        time_str.tm_mday = ipd.h.day;
        time_str.tm_hour = ipd.h.hour;
        time_str.tm_min = ipd.h.minute;
        time_str.tm_sec = 00.;
        time_unix = mktime(&time_str);
        if (time_unix > time_start && time_unix < time_end) {
            strcpy((fns_str[anf]).filename,files[i]);
            fns_str[anf].time_unix = time_unix;
            anf++;
        }
    }

    if (anf == 0) {
        fmerrmsg(where,
                "Could not find any flux files for the specified period %s - %s",
                stime, etime);
        exit(FM_OK);
    }

    /*
     * Sort satellite flux files by time...
     */
    time_tst = fns_str[0].time_unix;
    for (i=1; i<anf; i++) {
        time_tst = fns_str[i].time_unix;
        strcpy(fn,fns_str[i].filename);
        printf(" %s\n", fn);
        j=i-1;
        while (j>=0 && fns_str[j].time_unix > time_tst) {
            fns_str[j+1].time_unix=fns_str[j].time_unix;
            strcpy(fns_str[j+1].filename,fns_str[j].filename);
            j--;
        }
        fns_str[j+1].time_unix=time_tst;
        strcpy(fns_str[j+1].filename,fn);
    }
    fprintf(stdout,"\n");
    fprintf(stdout," Filenames is now sorted by time (UNIX)...\n");

    /*
     * Read the autostation flux data corresponding to the first time in
     * filelist. Each autostation file contains a whole month of data, reread
     * these data if month changes when running through file list.
     * Do not read if only asynchroneous storage of satellite data is
     * requested.
     */
    time_ptr = gmtime(&fns_str[0].time_unix);
    if (!aflg) {
        std = (stdata **) malloc(sizeof(stdata *));
        if (!*std) {
            fmerrmsg(where,"Could not allocate memory");
            exit(FM_MEMALL_ERR);
        }
        if (qc_auto_obs_read((time_ptr->tm_year+1900), 
                    (time_ptr->tm_mon+1), stl, std) != 0) {
            fmerrmsg(where,
                    "Could not read autostation data\n");
            exit(FM_OK);
        }
    }

    /*
     * Specifying the size of the data collection box. This should be
     * configurable in the future, but is hardcoded at present...
     */
    sdata.iw = 13;
    sdata.ih = 13;
    sdata.data = (float *) malloc((sdata.iw*sdata.ih)*sizeof(float));
    if (!sdata.data) {
        fmerrmsg(where,"Could not allocate memory");
        exit(FM_MEMALL_ERR);
    }

    fp = fopen(outfile,"a");
    if (!fp) {
        fmerrmsg(where,"Could not open output file...");
        exit(FM_OK);
    }

    /*
     * Now the satellite generated flux estimates are looped through and
     * time identification is compared with time of observations and data
     * stored i requirements are fulfilled...
     */
    printf(" Number of flux files %d\n", anf);
    for (i=0; i<anf; i++) {
        fprintf(stdout,"\n\n Reading flux file: %s\n", fns_str[i].filename);

        /*
         * Read the satellite derived data
         */
        sprintf(infile,"%s%s", PRODUCTPATH,fns_str[i].filename);
        printf(" Reading\n %s\n", infile);
        status = read_hdf5_product(infile, &ipd, 0);
        if (status != 0) {
            fmerrmsg(where,
                    "Could not read input file. Program execution terminates");
            exit(FM_OK);
        }
        /*
         * Transfer CM data to float array before further processing.
         */
        if (ipd.h.z == 7 && strcmp(ipd.d[6].description,"CM") == 0) {
            cmdata = (float *) malloc((ipd.h.iw*ipd.h.ih)*sizeof(float));
            if (!cmdata) {
                fmerrmsg(where,
                        "Could not allocate cmdata for %s", fns_str[i].filename);
                exit(FM_MEMALL_ERR);
            }
            for (j=0;j<(ipd.h.iw*ipd.h.ih);j++) {
                cmdata[j] = (float) ((unsigned short *) ipd.d[6].data)[j];
            }
        }

        /*
         * Read autostation flux data for the image if month has changed as
         * file list is looped...
         */
        k = time_str.tm_mon;
        time_ptr = gmtime(&fns_str[i].time_unix);
        if (k != time_ptr->tm_mon) {
            fprintf(stdout," Month has changed when looping file list\n");
            fprintf(stdout," Reading new autostation data\n");
            if (!aflg) {
                clear_stdata(std, stl.cnt);
                std = (stdata **) malloc(sizeof(stdata *));
                if (!*std) {
                    fmerrmsg(where," Could not allocate memory");
                    exit(FM_OK);
                }
                if (qc_auto_obs_read((time_ptr->tm_year+1900), 
                            (time_ptr->tm_mon+1), stl, std) != 0) {
                    fmerrmsg(where,
                            "Could not read autostation data. Program execution terminates...");
                    exit(FM_OK);
                }
            }
        }

        /*
         * Store collocated flux estimates and measurements in file. Below
         * all available stations are looped for each satellite derived
         * flux file.
         */
        for (j=0; j<stl.cnt; j++) {
            if ((*std)[j].missing) continue;

            gpos.lat = stl.id[j].lat;
            gpos.lon = stl.id[j].lon;
            /*
             * First the actual flux data is checked.
             */
            fprintf(stdout,"\n Collecting data for %s\n", stl.id[j].name);
            if (return_product_area(gpos, ipd.h, ipd.d[0].data, &sdata) != 0) {
                fmerrmsg(where,
                        "Did not find valid flux data for station %s\nfor flux file %s",
                        stl.id[j].name, fns_str[i].filename); 
                continue;
            }
            /*
             * Average flux estimates first
             */
            if (sdata.iw == 1 && sdata.ih == 1) {
                meanflux = *(sdata.data);
            } else {
                /*
                 * Generate mean value from all satellite data
                 * and store this in collocated file for
                 * easier analysis. This could be changed in
                 * the future...
                 */
                meanflux = 0.;
                novalobs = 0;
                for (l=0; l<(sdata.iw*sdata.ih); l++) {
                    if (sdata.data[l] >= 0) {
                        meanflux += sdata.data[l];
                        novalobs++;
                    }
                }
                meanflux /= (float) novalobs;
            }

            /*
             * Needs info on observation geometry as well...
             */
            for (m=0;m<3;m++) {
                /*printf(" %s\n", ipd.d[m+3].description);*/
                if (return_product_area(gpos, ipd.h, 
                            ipd.d[m+3].data, &sdata) != 0) {
                    fmerrmsg(where,
                            " Did not find valid geom data for station %s %s\n",
                            stl.id[j].name,
                            "although flux data were found...");
                    continue;
                }
                /*
                 * Average obs geom estimates use val obs found for
                 * fluxes.
                 */
                meanvalues[m] = 0.;
                geomobs = 0;
                if (sdata.iw == 1 && sdata.ih == 1) {
                    meanvalues[m] = *(sdata.data);
                } else {
                    for (l=0; l<(sdata.iw*sdata.ih); l++) {
                        if (sdata.data[l] >= 0) {
                            meanvalues[m] += sdata.data[l];
                            geomobs++;
                        }
                    }
                    meanvalues[m] /= (float) geomobs;
                }
            }

            /*
             * Process the cloud mask information.
             */
            if (ipd.h.z == 7 && strcmp(ipd.d[6].description,"CM") == 0) {
                if (return_product_area(gpos, ipd.h, 
                            cmdata, &sdata) != 0) {
                    fmerrmsg(where,
                            " Did not find valid CM data for station %s %s\n",
                            stl.id[j].name,
                            "although flux data were found...");
                    continue;
                }
                /*
                 * Average CM used val obs found for fluxes.
                 */
                meancm = 0.;
                cmobs = 0;
                if (sdata.iw == 1 && sdata.ih == 1) {
                    if (*(sdata.data) >= 0.99 && *(sdata.data) <= 4.01) {
                        meancm = 1;
                    } else if (*(sdata.data) >= 4.99 && *(sdata.data) <= 19.01) {
                        meancm = 2;
                    }
                } else {
                    for (l=0; l<(sdata.iw*sdata.ih); l++) {
                        if (sdata.data[l] >= 0.99 && sdata.data[l] <= 4.01) {
                            meancm += 1;
                            cmobs++;
                        } else if (sdata.data[l] >= 4.99 && sdata.data[l] <= 19.01) {
                            meancm += 2;
                            cmobs++;
                        }

                    }
                    meancm /= (float) cmobs;
                }
            }

            /*
             * Checking that sat and obs is from the same hour.
             * According to Sofus Lystad the observations represents 
             * integration of the last hour, time is given in UTC.
             * The date specification below might cause evening
             * observations during month changes to be missed, but this is
             * not a major problem...
             */
            if (ipd.h.minute > 10) {
                if (ipd.h.hour == 23) {
                    sprintf(timeid,"%04d%02d%02d0000", 
                            ipd.h.year, ipd.h.month, (ipd.h.day+1));
                } else {
                    sprintf(timeid,"%04d%02d%02d%02d00", 
                            ipd.h.year, ipd.h.month, ipd.h.day, (ipd.h.hour+1));
                }
            } else {
                sprintf(timeid,"%04d%02d%02d%02d00", 
                        ipd.h.year, ipd.h.month, ipd.h.day, ipd.h.hour);
            }
            /* Need to fix aflg here!!!!*/
            if (stl.id[j].number == (*std)[j].id) {
                for (h=0; h<NO_MONTHOBS; h++) {
                    if (strcmp(timeid,(*std)[j].param[h].date) == 0) {
                        /*
                         * First print representative acquisition time for
                         * satellite based estimates.
                         */
                        fprintf(fp," %4d%02d%02d%02d%02d",
                                ipd.h.year,ipd.h.month,ipd.h.day,
                                ipd.h.hour,ipd.h.minute);
                        /*
                         * Dump satellite based estimates and auxiliary
                         * data. The satellite data are averaged over
                         * 13x13 pixels to compensate for positioning
                         * error of satellites and the different view
                         * perspective from ground and space.
                         */
                        fprintf(fp," %7.2f %3d %3d %s %.2f %.2f %.2f %.2f", 
                                meanflux, novalobs, (sdata.iw*sdata.ih),
                                ipd.h.source, 
                                meanvalues[0], meanvalues[1], meanvalues[2],
                                meancm);
                        /*
                         * Dump all information concerning observations.
                         * If asynchoneous logging is done, dump
                         * placeholders for future in situ observations to
                         * be included.
                         */
                        if (aflg) {
                            fprintf(fp," %12s %5d %7.2f %7.2f %7.2f", 
                                    "000000000000", 0,
                                    misval,misval,misval);
                        } else {
                            fprintf(fp," %12s %5d %7.2f %7.2f %7.2f", 
                                    (*std)[j].param[h].date, (*std)[j].id,
                                    (*std)[j].param[h].TTM,
                                    (*std)[j].param[h].Q0,(*std)[j].param[h].ST);
                        }
                        /*
                         * Insert newline to mark record.
                         */
                        fprintf(fp,"\n");
                    }
                }
            }
        }
        if (ipd.h.z == 7 && strcmp(ipd.d[6].description,"CM") == 0) {
            free(cmdata);
        }
        free_osihdf(&ipd);
    }
    fclose(fp);

    /*
     * Release allocated memory...
     */
    for (i=0; i<nf; i++) {
        free(files[i]);
    }

    exit(FM_OK);
}

void usage(void) {

    fprintf(stdout,"\n");
    fprintf(stdout," fluxval_hour [-a] -s <start_time> -e <end_time>");
    fprintf(stdout," -p <area> -i <stlist> -o <output>\n");
    fprintf(stdout,"     start_time: yyyymmddhh\n");
    fprintf(stdout,"     end_time: yyyymmddhh\n");
    fprintf(stdout,"     area: ns | nr | at | gr\n");
    fprintf(stdout,"     stlist: ASCII file containing station ids\n");
    fprintf(stdout,"     output: filename and path (ASCII file)\n");
    fprintf(stdout,"     -a: only store satellite estimates\n");
    fprintf(stdout,"\n");
    exit(FM_OK);

}

