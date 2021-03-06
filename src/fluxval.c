/*
 * NAME:
 * fluxval.c
 * 
 * PURPOSE:
 * To monitor the quality of OSISAF radiative flux products against
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
 * Experience core dump in clear_stdat when changing year.
 *
 * AUTHOR:
 * �ystein God�y, DNMI/FOU, 27/07/2000
 * MODIFIED:
 * �ystein God�y, DNMI/FOU, 10/10/2001
 * Adapted for use on LINUX.
 * �ystein God�y, DNMI/FOU, 25/03/2002
 * Changing output format and controlling time comparison between obs and
 * sat. Control is only implemented for ns products at present (hard coded
 * below...). It is assumed that observations are given in UTC time and
 * centered on the observation time...
 * �ystein God�y, DNMI/FOU, 27/03/2002
 * Added satellite name and observation geometry to output...
 * �ystein God�y, DNMI/FOU, 03/04/2002
 * Added cloud mask information to output, this requires cloud mask
 * information to be put in the SSI product area output...
 * �ystein God�y, DNMI/FOU, 10/04/2002
 * Renamed qc_auto to qc_auto_hour to prepare for daily integration
 * version and rewrote qc_auto_hour to accept product area as command line
 * input. I did also find an error in the stations input list. Position is
 * given as degrees, minutes and seconds in hundredths in the reference
 * book I have used (Klimaavdelingen) while I thought it was in decimal
 * degrees. This is now changed...
 * �ystein God�y, DNMI/FOU, 07.07.2003
 * Better handling of missing observations...
 * �ystein God�y, met.no/FOU, 13.05.2004
 * Changed specification of cloud type/amount information from PPS cloud
 * type product. Now a value between 1 and 2 is returned, 1 representing
 * cloud free and 2 overcast...
 * �ystein God�y, METNO/FOU, 13.09.2010: Adapted for use with libfmutil
 * wherever possible. Some more cleaning is necessary. Extraction of
 * functions is still to be done along with use of configuration file
 * instead of command line options and it should operate without /starc as
 * well. 
 * �ystein God�y, METNO/FOU, 14.09.2010: Included validation of both
 * passage and daily estimates in the same main program for easier
 * maintenance in the future.
 * �ystein God�y, METNO/FOU, 22.11.2010: Added option to circumvent /starc
 * for testing purposes.
 * �ystein God�y, METNO/FOU, 2011-02-11: Added -m option to specify
 * directory containing measurements.
 * �ystein God�y, METNO/FOU, 2011-04-04: Added support for IPY-data and
 * DLI products. Changed command line options.
 * �ystein God�y, METNO/FOU, 2014-02-11: Added reading of data extracted
 * from KDVH (Bioforsk stations).
 * �ystein God�y, METNO/FOU, 2014-08-21: Added support from WMO GTS data
 * (own ASCII format dumped from BUFR).
 * �ystein God�y, METNO/FOU, 2015-04-23: Added support for OSISAF archive.
 * �ystein God�y, METNO/FOU, 2016-01-21: Added support for 5km daily
 * files.
 */

#include <fluxval.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    extern char *optarg;
    char *where="fluxval";
    char dir2read[FMSTRING512];
    char *outfile, *infile, *indir, *stfile, *parea, *fntest, *datadir;
    char product[FMSTRING16];
    char stime[FMSTRING16], etime[FMSTRING16];
    char timeid[FMSTRING16], obstime[FMSTRING16];
    int h, i, j, k, l, m, n, novalobs, cmobs, geomobs, noobs;
    short sflg = 0, eflg = 0, pflg =0, iflg = 0, oflg = 0, aflg = 0, dflg = 0;
    short rflg = 0, mflg = 0, gflg = 0, cflg = 0, kflg = 0, bflg = 0, wflg = 0;
    short fflg = 0, lflg = 0;
    short status;
    short obsmonth;
    osihdf ipd;
    struct tm time_str;
    time_t time_start, time_end;
    fmsec1970 tstart, tend;
    fmtime tstartfm, tendfm;
    fmstarclist starclist;
    fmfilelist filelist;
    stlist stl;
    stdata **std;
    s_data sdata;
    fmgeopos gpos;
    float meanflux, meanvalues[3], *cmdata, meancm;
    float meanobs;
    float misval=-999.;
    FILE *fp;

    /* 
     * Decode command line arguments containing path to input files (one for
     * each area produced) and name (and path) of the output file.
     */
    while ((i = getopt(argc, argv, "ablcwfks:e:p:g:i:o:dr:m:")) != EOF) {
        switch (i) {
            case 's':
                if (strlen(optarg) != 10) {
                    fmerrmsg(where,
                        "stime (%s) is not of appropriate length", optarg);
                    exit(FM_IO_ERR);
                }
                strcpy(stime,optarg);
                sflg++;
                break;
            case 'e':
                if (strlen(optarg) != 10) {
                    fmerrmsg(where,
                        "etime (%s) is not of appropriate length", optarg);
                    exit(FM_IO_ERR);
                }
                strcpy(etime,optarg);
                eflg++;
                break;
            case 'g':
                parea = (char *) malloc(FILENAMELEN);
                if (!parea) exit(FM_MEMALL_ERR);
                if (sprintf(parea,"%s",optarg) < 0) exit(FM_IO_ERR);
                gflg++;
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
            case 'r':
                indir = (char *) malloc(FILENAMELEN);
                if (!indir) exit(FM_MEMALL_ERR);
                if (sprintf(indir,"%s",optarg) < 0) exit(FM_IO_ERR);
                rflg++;
                break;
            case 'p':
                if (sprintf(product,"%s",optarg) < 0) exit(FM_IO_ERR);
                pflg++;
                break;
            case 'm':
                datadir = (char *) malloc(FILENAMELEN);
                if (!datadir) exit(FM_MEMALL_ERR);
                if (sprintf(datadir,"%s",optarg) < 0) exit(FM_IO_ERR);
                mflg++;
                break;
            case 'b':
                bflg++;
                break;
            case 'c':
                cflg++;
                break;
            case 'w':
                wflg++;
                break;
            case 'a':
                aflg++;
                break;
            case 'd':
                dflg++;
                break;
            case 'l':
                lflg++;
                break;
            case 'k':
                kflg++;
                break;
            case 'f':
                fflg++;
                break;
            default:
                usage();
                break;
        }
    }

    /*
     * Check if all necessary information was given at command line.
     */
    if (!sflg || !eflg || !iflg || !oflg || !pflg) usage();
    if ((bflg && cflg)||(bflg && wflg)||(cflg && wflg)) usage();
    if (!mflg) {
        datadir = (char *) malloc(FILENAMELEN);
        if (!datadir) exit(FM_MEMALL_ERR);
        if (sprintf(datadir,"%s",DATAPATH) < 0) exit(FM_IO_ERR);
    }

    /*
     * Create character string to test filenames against to avoid
     * unnecessary processing...
     */
    fntest = (char *) malloc(FILENAMELEN);
    if (!fntest) exit(FM_MEMALL_ERR);
    if (dflg) {
        sprintf(fntest,"daily");
    } else if (lflg) {
        sprintf(fntest,"24h_hl");
    } else {
        sprintf(fntest,"%s.hdf5",parea);
    }

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
     * Loop through products stored
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
    if (rflg && kflg) { /* starc */
        if (fmstarcdirs(tstartfm,tendfm,&starclist)) {
            fmerrmsg(where,"Could not create starcdirs to process.");
            exit(FM_IO_ERR);
        }
    } else if (rflg && fflg) { /* OSISAF archive */
        if (fmsafarcdirs(tstartfm,tendfm,&starclist)) {
            fmerrmsg(where,"Could not create safarcdirs to process.");
            exit(FM_IO_ERR);
        }
    } else if (rflg) {
        starclist.nfiles = 1;
        if (fmalloc_byte_2d(&(starclist.dirname),1,FMSTRING512)) {
            fmerrmsg(where,"Could not allocate starclist for single directory");
            exit(FM_MEMALL_ERR);
        }
        sprintf(starclist.dirname[0],"%s",indir);
    } else {
        if (fmstarcdirs(tstartfm,tendfm,&starclist)) {
            fmerrmsg(where,"Could not create starcdirs to process.");
            exit(FM_IO_ERR);
        }
    }
    if (starclist.nfiles == 0) {
        fmerrmsg(where,"No estimate files found in %s...",indir);
        printf("%d - %d \n", (int) tstart, (int) tend);
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
    if (dflg || lflg) {
        sdata.iw = 1;
        sdata.ih = 1;
    } else {
        sdata.iw = 13;
        sdata.ih = 13;
    }
    sdata.data = (float *) malloc((sdata.iw*sdata.ih)*sizeof(float));
    if (!sdata.data) {
        fmerrmsg(where,"Could not allocate memory");
        exit(FM_MEMALL_ERR);
    }
    obsmonth = 0;
    for (i=0;i<starclist.nfiles;i++) {
        if (rflg && kflg) {
            sprintf(dir2read,"%s/%s/%s",indir,starclist.dirname[i],product);
        } else if (rflg && fflg) {
            sprintf(dir2read,"%s/%s",indir,starclist.dirname[i]);
        } else if (rflg) {
            sprintf(dir2read,"%s",starclist.dirname[0]);
        } else {
            sprintf(dir2read,"%s/%s/%s",STARCPATH,starclist.dirname[i],product);
        }
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
                printf("Source: %s\n", ipd.h.source);
                printf("Product: %s\n", ipd.h.product);
                printf("Area: %s\n", ipd.h.area);
                printf("\t%4d-%02d-%02d %02d:%02d\n",
                        ipd.h.year, ipd.h.month, ipd.h.day, ipd.h.hour, ipd.h.minute);
                for (k=0; k<ipd.h.z; k++) {
                    printf("\tBand %d - %s\n", k, ipd.d[k].description);
                }
                printf("\tImage width: %d\n",ipd.h.iw);
                printf("\tImage height: %d\n",ipd.h.ih);

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
                        if (cflg) {
                            if (fluxval_readobs_ascii(datadir, 
                                        ipd.h.year,ipd.h.month, 
                                        stl, std) != 0) {
                                fmerrmsg(where,
                                        "Could not read autostation data\n");
                                exit(FM_OK);
                            }
                        } else if (bflg) {
                            if (fluxval_readobs_ulric(datadir, 
                                        ipd.h.year,ipd.h.month, 
                                        stl, std) != 0) {
                                fmerrmsg(where,
                                        "Could not read autostation data\n");
                                exit(FM_OK);
                            }
                        } else if (wflg) {
                            if (fluxval_readobs_gts(datadir, 
                                        ipd.h.year,ipd.h.month, 
                                        stl, std) != 0) {
                                fmerrmsg(where,
                                        "Could not read autostation data\n");
                                exit(FM_OK);
                            }
                        } else {
                            if (fluxval_readobs(datadir, 
                                        ipd.h.year,ipd.h.month, 
                                        stl, std) != 0) {
                                fmerrmsg(where,
                                        "Could not read autostation data\n");
                                exit(FM_OK);
                            }
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
                        meanvalues[m] = 0.;
                    }
                    meancm = 0.;
                    if (!dflg && !lflg && (strstr(product,"ssi")!=NULL)) {
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
                    }

                    /*
                     * Process the cloud mask information.
                     * This block should probably be extracted into a
                     * subroutine/function...
                     */
                    if (!dflg && !lflg) {
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
                    }

                    /*
                     * If only satellite data are to be extracted around
                     * the stations listed, use the following...
                     */
                    if (aflg) {
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
                    if ((*std)[k].missing) {
                        fmerrmsg(where,
                        "Observations are not available for station %d",k);
                        continue;
                    }

                    /*
                     * Checking that sat and obs is from the same hour.
                     * According to Sofus Lystad the Bioforsk observations
                     * represents integration of the last hour, time is
                     * given in UTC. The date specification below might
                     * cause evening observations during month changes to
                     * be missed, but this is not a major problem...
                     *
                     * IPY-observations (Arctic stations) are
                     * represented at the central time. Data are collected
                     * at 1 minute intervals and transformed into hourly
                     * estimates, centered at observation time.
                     *
                     * Ekofisk are represented by 10 min intervals, where
                     * each time represents the data from the previous 10
                     * minutes. Data are reformatted to hourly data.
                     */
                    if (dflg || lflg) {
                        sprintf(timeid,"%04d%02d%02d", 
                            ipd.h.year, ipd.h.month, ipd.h.day);
                    } else {
                        if (ipd.h.minute > 10) {
                            if (ipd.h.hour == 23) {
                                sprintf(timeid,"%04d%02d%02d0000", 
                                    ipd.h.year, ipd.h.month, (ipd.h.day+1));
                            } else {
                                if (cflg) {
                                    sprintf(timeid,"%04d%02d%02d%02d30", 
                                        ipd.h.year, ipd.h.month, ipd.h.day, 
                                        ipd.h.hour);
                                } else {
                                    sprintf(timeid,"%04d%02d%02d%02d00", 
                                        ipd.h.year, ipd.h.month, ipd.h.day, 
                                        (ipd.h.hour+1));
                                }
                            }
                        } else {
                            sprintf(timeid,"%04d%02d%02d%02d00", 
                                ipd.h.year, ipd.h.month, ipd.h.day, ipd.h.hour);
                        }
                    }
                    if (stl.id[k].number == (*std)[k].id) {
                        meanobs = 0;
                        noobs = 0;
                        for (h=0; h<NO_MONTHOBS; h++) {
                            if (cflg) {
                                /* Compensating for roundoff errors in
                                 * time spec. */
                                sprintf(obstime,"%s",(*std)[k].param[h].date);
                                obstime[10] = '3';
                                obstime[11] = '0';
                                obstime[12] = '0';
                            } else {
                                sprintf(obstime,"%s",(*std)[k].param[h].date);
                            }
                            if (strstr(obstime,timeid)) {
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
                                if (dflg || lflg) {
                                    fprintf(fp, " %7.2f %3d", 
                                        meanflux, (sdata.iw*sdata.ih));
                                } else {
                                    fprintf(fp,
                                        " %7.2f %3d %3d %s %.2f %.2f %.2f %.2f", 
                                        meanflux, novalobs, 
                                        (sdata.iw*sdata.ih),
                                        ipd.h.source, 
                                        meanvalues[0], 
                                        meanvalues[1], meanvalues[2],
                                        meancm);
                                }
                                /*
                                 * Dump all information concerning
                                 * observations. 
                                 */
                                if (dflg || lflg) {
                                    for (n=1;n<=24;n++) {
                                        if (strstr(product,"ssi")){
                                            if ((*std)[k].param[h+n].Q0 > misval) {
                                                meanobs += (*std)[k].param[h+n].Q0;
                                                noobs++;
                                            }
                                        } else {
                                            if ((*std)[k].param[h+n].LW > misval) {
                                                meanobs += (*std)[k].param[h+n].LW;
                                                noobs++;
                                            }
                                        }
                                    }
                                    if (noobs == 0) {
                                        fprintf(fp," %05d %7.2f",
                                                (*std)[k].id,misval);
                                        fprintf(fp,"\n");
                                        break;
                                    }
                                    meanobs /= (float) noobs;
                                    fprintf(fp," %05d %7.2f",
                                        (*std)[k].id,meanobs);
                                    fprintf(fp,"\n");
                                    break;
                                } else {
                                    if (cflg) {
                                        if (strstr(product,"ssi")) {
                                            fprintf(fp,
                                                " %12s %05d %7.2f", 
                                                (*std)[k].param[h].date, 
                                                (*std)[k].id,
                                                (*std)[k].param[h].Q0);
                                        } else {
                                            fprintf(fp,
                                                " %12s %05d %7.2f", 
                                                (*std)[k].param[h].date, 
                                                (*std)[k].id,
                                                (*std)[k].param[h].LW);
                                        }
                                        fprintf(fp,"\n");
                                    } else {
                                        fprintf(fp,
                                            " %12s %05d %7.2f %7.2f %7.2f", 
                                            (*std)[k].param[h].date, 
                                            (*std)[k].id,
                                            (*std)[k].param[h].TTM,
                                            (*std)[k].param[h].Q0,
                                            (*std)[k].param[h].ST);
                                        fprintf(fp,"\n");
                                    }
                                }
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

    exit(FM_OK);
}

void usage(void) {

    fprintf(stdout,"\n");
    fprintf(stdout," fluxval [-adlcfkbw -g <area>] -p <product> ");
    fprintf(stdout," -s <start_time> -e <end_time>");
    fprintf(stdout," -r <satestdir> -m <obsdir>");
    fprintf(stdout," -i <stlist> -o <output>\n");
    fprintf(stdout,"     -p product: ssi or dli\n");
    fprintf(stdout,"     -s start_time: yyyymmddhh\n");
    fprintf(stdout,"     -e end_time: yyyymmddhh\n");
    fprintf(stdout,"     -g geographical area: ns | nr | at | gr\n");
    fprintf(stdout,"     -i stlist: ASCII file containing station ids\n");
    fprintf(stdout,"     -o output: filename and path (ASCII file)\n");
    fprintf(stdout,"     -r satestdir: directory to collet satellite estimates from\n");
    fprintf(stdout,"     -m obsdir: directory to collet measurements from\n");
    fprintf(stdout,"     -a: only store satellite estimates\n");
    fprintf(stdout,"     -d: process daily products (in old resolution), ignores option -g\n");
    fprintf(stdout,"     -l: process daily products (in new resolution), ignores option -g\n");
    fprintf(stdout,"     -b: Bioforskdata extracted from KDVH\n");
    fprintf(stdout,"     -c: compact observation format (IPY stations etc.)\n");
    fprintf(stdout,"     -w: observations extracted from WMO GTS\n");
    fprintf(stdout,"     -k: segmented data (starc-like)\n");
    fprintf(stdout,"     -f: segmented data (OSISAF archive like)\n");
    fprintf(stdout,"\n");

    exit(FM_OK);
}

