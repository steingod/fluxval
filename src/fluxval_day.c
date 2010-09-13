/*
 * NAME:
 * qc_auto_daily.c
 * 
 * PURPOSE:
 * To monitor the quality of SAF SSI products against automatic stations in
 * Norway.
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
 * Support for observations from danish ground stations received from
 * Søren Andersen should be included in this system. As should support for
 * the data received from Ås. When this is implemented thorough check of
 * representation times should be performed
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
 *
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
 * Øystein Godøy, DNMI/FOU, 22.06.2003
 * Øystein Godøy, DNMI/FOU, 07.07.2003
 * Better handling of missing observations...
 * Øystein Godøy, METNO/FOU, 28.08.2010
 * Updated to use new libraries and increased functionality, including
 * asynchroneous validation.
 *
 * VERSION:
 * $Id$
 */

#include <fluxval.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {

    extern char *optarg;
    char *where="fluxval_day";
    char *outfile, *infile, *stfile, *fntest, *files[MAXFILES];
    char stime[11], etime[11], timeid[11], fn[50];
    int h, i, j, k, l, nf, anf, novalobs, noobs, obsmon;
    short sflg = 0, eflg = 0, iflg = 0, oflg = 0, aflg = 0, status;
    osihdf ipd;
    DIR *dirp;
    struct dirent *direntp;
    struct stat stbuf;
    struct tm time_str, *time_ptr;
    time_t time_unix, time_start, time_end, time_tst;
    fns fns_str[150];
    stlist stl;
    stdata **std;
    s_data sdata;
    fmgeopos gpos;
    float meanflux, meanobs;
    FILE *fp;

    fprintf(stdout,"\n===> Quality control of daily SSI estimates <===\n\n");

    /* 
     * Decode command line arguments containing path to input files (one for
     * each area produced) and name (and path) of the output file.
     */
    while ((i = getopt(argc, argv, "as:e:i:o:")) != EOF) {
	switch (i) {
	    case 's':
		strcpy(stime,optarg);
		sflg++;
		break;
	    case 'e':
		strcpy(etime,optarg);
		eflg++;
		break;
	    case 'i':
	    	stfile = (char *) malloc(FILENAMELEN);
		if (!stfile) exit(3);
		if (sprintf(stfile,"%s",optarg) < 0) exit(1);
		iflg++;
		break;
	    case 'o':
	    	outfile = (char *) malloc(FILENAMELEN);
		if (!outfile) exit(3);
		if (sprintf(outfile,"%s",optarg) < 0) exit(1);
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
    if (!fntest) exit(3);
    sprintf(fntest,".hdf5");

    /*
     * Decode time specification of period.
     */
    if (timecnv(stime, &time_str) != 0) {
	fprintf(stderr," ERROR(qc_auto_daily):");
	fprintf(stderr," Could not decode time specification\n");
	exit(FM_OK);
    }
    time_start = mktime(&time_str);
    if (timecnv(etime, &time_str) != 0) {
	fprintf(stderr," ERROR(qc_auto_daily):");
	fprintf(stderr," Could not decode time specification\n");
	exit(FM_OK);
    }
    time_end = mktime(&time_str);

    /*
     * Decode station list information.
     */
    if (decode_stlist(stfile, &stl) != 0) {
	fprintf(stderr," ERROR(qc_auto_daily):");
	fprintf(stderr," Could not decode station file\n");
	fprintf(stderr," Program execution terminates\n");
	exit(FM_OK);
    }

    /*
     * Loop through all passages available within the specified time period.
     */
    dirp = opendir(DAILYPATH);
    if (!dirp) {
	fprintf(stderr, " ERROR(qc_auto_daily):");
	fprintf(stderr, " Could not open directory\n %s\n", DAILYPATH);
	fprintf(stderr, " for file listing\n");
	exit(FM_OK);
    }
    fprintf(stdout, " Checking available files in\n %s\n", DAILYPATH);
    nf = 0;
    while ((direntp = readdir(dirp)) != NULL) {
	if (strstr(direntp->d_name,fntest) == NULL) continue;
	if (nf > (MAXFILES-1)) {
	    fprintf(stderr, " ERROR(qc_auto_daily):");
	    fprintf(stderr," increase MAXFILES variable in header file\n");
	    exit(FM_OK);
	}
	files[nf] = (char *) malloc((strlen(direntp->d_name)+1)*sizeof(char));
	if (!files[nf]) {
	    fprintf(stderr," ERROR(qc_auto_daily):");
	    fprintf(stderr," Could not allocate memory for filenames\n");
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
	fprintf(stderr," ERROR(qc_auto_daily):");
	fprintf(stderr," Could not allocate memory for filename\n");
	exit(FM_OK);
    }
    anf = 0;

    for (i=0; i<nf; i++) {
	if (strstr(files[i],"ssi") == NULL ||
	    strstr(files[i],"hdf5") == NULL) continue;
	/*
	 * Get representation time
	 */
	sprintf(infile,"%s%s", DAILYPATH,files[i]);
	status = read_hdf5_product(infile, &ipd, 1);
	if (status != 0) {
	    fprintf(stdout," Could not open file\n %s\n", infile);
	    continue;
	}

	/*
	 * Check if within the allowed period, if so store header. This is 
	 * easiest achieved if time is converted to UNIX time...
	 */
	stat(infile,&stbuf);
	fprintf(stdout," %s %d %d %d\n",
	    files[i],(int) stbuf.st_mtime,(int) time_start,(int) time_end);
	if (stbuf.st_mtime > time_start && stbuf.st_mtime < time_end) {
	    time_str.tm_year = ipd.h.year-1900;
	    time_str.tm_mon = ipd.h.month-1;
	    time_str.tm_mday = ipd.h.day;
	    time_str.tm_hour = ipd.h.hour;
	    time_str.tm_min = ipd.h.minute;
	    time_str.tm_sec = 00.;
	    time_unix = mktime(&time_str);
	    strcpy((fns_str[anf]).filename,files[i]);
	    fns_str[anf].time_unix = time_unix;
	    fprintf(stdout,">> %s\n",(fns_str[anf]).filename);
	    anf++;
	}
    }

    if (anf == 0) {
	fprintf(stderr,
		" Could not find any flux files for the specified period\n");
	fprintf(stderr," %s - %s\n", stime, etime);
	exit(FM_OK);
    }
    fprintf(stdout," Found a total of %d files satisfying time req..\n", anf);

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
     */
    time_ptr = gmtime(&fns_str[0].time_unix);
    std = (stdata **) malloc(sizeof(stdata *));
    if (!*std) {
	fprintf(stderr," ERROR(qc_auto_daily):");
	fprintf(stderr," Could not allocate memory\n");
	exit(FM_OK);
    }
    if (qc_auto_obs_read((time_ptr->tm_year+1900), 
	(time_ptr->tm_mon+1), stl, std) != 0) {
	fprintf(stderr,
	    " ERROR(qc_auto_daily): Could not read autostation data\n");
	fprintf(stderr," Program execution terminates...\n");
	exit(FM_OK);
    }
    obsmon = time_ptr->tm_mon;

    /*
     * Specifying the size of the data collection box. This should be
     * configurable in the future, but is hardcoded at present...
     */
    sdata.iw = 1;
    sdata.ih = 1;
    sdata.data = (float *) malloc((sdata.iw*sdata.ih)*sizeof(float));
    if (!sdata.data) {
	fprintf(stderr," ERROR(qc_auto_daily):");
	fprintf(stderr," Could not allocate memory\n");
	exit(FM_OK);
    }
     
    fp = fopen(outfile,"a");
    if (!fp) {
	fprintf(stderr," ERROR(qc_auto_daily):");
	fprintf(stderr," Could not open output file...\n");
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
	sprintf(infile,"%s%s", DAILYPATH,fns_str[i].filename);
	printf(" Reading\n %s\n", infile);
	status = read_hdf5_product(infile, &ipd, 0);
	if (status != 0) {
	    fprintf(stderr," ERROR(qc_auto_daily):");
	    fprintf(stderr," Could not read input file\n");
	    fprintf(stderr," Program execution terminates\n");
	    exit(FM_OK);
	}

	/*
	 * Read autostation flux data for the image if month has changed as
	 * file list is looped...
	 */
	/*k = time_str.tm_mon;*/
	time_ptr = gmtime(&fns_str[i].time_unix);
	if (obsmon != time_ptr->tm_mon) {
	    fprintf(stdout," Month has changed when looping file list\n");
	    fprintf(stdout," Reading new autostation data\n");
	    clear_stdata(std, stl.cnt);
	    std = (stdata **) malloc(sizeof(stdata *));
	    if (!*std) {
		fprintf(stderr," ERROR(qc_auto_daily):");
		fprintf(stderr," Could not allocate memory\n");
		exit(FM_OK);
	    }
	    if (qc_auto_obs_read((time_ptr->tm_year+1900), 
		(time_ptr->tm_mon+1), stl, std) != 0) {
		fprintf(stderr,
			" ERROR:(main): Could not read autostation data\n");
		fprintf(stderr," Program execution terminates...\n");
		exit(FM_OK);
	    }
	    obsmon = time_ptr->tm_mon;
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
		fprintf(stderr," ERROR(qc_auto_daily):");
		fprintf(stderr," Did not find valid flux data for station %s\n",
		    stl.id[j].name);
		fprintf(stderr," for flux file %s\n",fns_str[i].filename); 
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
	     * Checking that sat and obs is from the same hour.
	     * According to Sofus Lystad the observations represents 
	     * integration of the last hour, time is given in UTC.
	     * The date specification below might cause evening
	     * observations during month changes to be missed, but this is
	     * not a major problem...
	     */
	    sprintf(timeid,"%04d%02d%02d", 
		ipd.h.year, ipd.h.month, ipd.h.day);
	    if (stl.id[j].number == (*std)[j].id) {
		meanobs = 0.;
		noobs = 0;
		for (h=0; h<NO_MONTHOBS; h++) {
		    /*
		     * Finds the first in the files...
		     */
		    if (strstr((*std)[j].param[h].date, timeid) != NULL) {
			for (k=1; k<=24; k++) {
			    /*
			    if ((*std)[j].param[h+k].Q0 < 9999999.) {
			    */
			    if ((*std)[j].param[h+k].Q0 > -999.) {
				meanobs += (*std)[j].param[h+k].Q0;
				noobs++;
			    }
			    
			}
			if (noobs == 0) break;
			meanobs /= (float) noobs;
			/*
			 * First print acquisition time of estimates.
			 */
			fprintf(fp," %4d%02d%02d",
			    ipd.h.year,ipd.h.month,ipd.h.day);
			/*
			 * Dumping observations.
			 */
			fprintf(fp," %d %7.2f", 
			    (*std)[j].id,meanobs);
			/*
			 * Now dumping estimates...
			 */
			fprintf(fp," %7.2f", 
				meanflux);
			fprintf(fp,"\n");
			break;
		    }
		}
	    }
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
    fprintf(stdout," fluxval_day [-a] -s <start_time> -e <end_time>");
    fprintf(stdout," -i <stlist> -o <output>\n");
    fprintf(stdout,"     start_time: yyyymmddhh\n");
    fprintf(stdout,"     end_time: yyyymmddhh\n");
    fprintf(stdout,"     stlist: ASCII file containing station ids\n");
    fprintf(stdout,"     output: filename and path (ASCII file)\n");
    fprintf(stdout,"     -a  only extract and store satellite estimates\n");
    fprintf(stdout,"\n");
    exit(FM_OK);
    
}

