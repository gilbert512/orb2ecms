// Modified by Gilbert. 2019.06.27
// for sending uncertainty & remove #1300, #8000 etc
// Changed compare with now & origin time
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "sock_func.h"

#include "orb2ecms.h"

#define DEFAULT_PFFILE "./pf/orb2ecms"

#include <errno.h>

#ifndef TRUE
  #define TRUE  1
#endif
#ifndef FALSE
  #define FALSE 0
#endif
#ifndef HN_STA_COUNT
  #define HN_STA_COUNT 3
#endif
#ifndef HN_ON
  #define HN_ON   1
#endif
#ifndef HN_SAME
  #define HN_SAME 2
#endif
#ifndef HN_OFF
  #define HN_OFF  0
#endif

// jman, 20170615
#define TMINFO_NUM 50

typedef struct StaInfo
{
    char m_sSta[10];
    int m_iState;
}StaInfo;

// jman, 20170615
typedef struct 
{
    double tm;
    double mag;
}TmInfo;

Pf   *get_pf=0;


static void usage()
{
    cbanner ("$Date: 2008/12/04 12:00:49 $",
            "[-p orb2ecms] [-r reject] [-S state] [-v] orb dbname",
            "","","");
    exit (1);
}

int pg_alarm = 0;
int setDetectInfo(const char *ac_pcSta, const int ac_iState);
int send2ECMS(char *a_pcSvrAddr, int a_iPort, char *a_pcSendBuff);
int send2KQMS(char *a_pcSvrAddr, int a_iPort, char *a_pcSendBuff);
static void alarm_proc();
int ebss, ecmss;

int main (int argc, char **argv)
{
    int         verbose = 0 ;
    double      time ;
    char       *orbname=0, *dbname=0, *s, *pf=0, server_ip[100] ;
    int         c, errflg = 0;
    int         orb, i ;
    int         pktid ;
    int         totpkts=0, totbytes=0 ;
    double      start_time, end_time ;
    char        srcname[ORBSRCNAME_SIZE] ;
    double      after = 0.0, quit = 0.0  ;
    int         check=0;
    char       *match = 0, *reject = 0 ;
    int         nmatch ;
    char       *packet=0 ;
    int         nbytes = 0 ;
    int         retcode, type, li;
    int         bufsize=0 ;
    int         abort = 0 ;
    char       *statefile = 0 ;
    char        net[10], sta[10], chan[10], state[10], filter[20];
    double      dettime, trgtime, lddate;
    float       snr;
    int         numsite;
    // jman, 20170531
    //char        wk_msg[60],wk_srcname[20], *tmpstr;
    //char        wk_msg[120],wk_srcname[20], *tmpstr , wk_msg_new[120];
	char        wk_msg[255],wk_srcname[20], *tmpstr , wk_msg_new[255];
    int         wk_state=0, ecms_port=0;
    STACHAN    *stachan;
    double      onftrigtime=0.0, offftrigtime=0.0, check_time=0.0;
    Pf         *pktpf=0;
    int         check_origin=0;
    double      dPrevOrgTime = 0.0;
    double      dTempTime1 = 0;
    double      dTempTime2 = 0;
    int         nFirstSendMagInfo = 0;
    int         nRtv = 0;
    double	before_origin_time=(double)0.0;
    double	old_origin_time=(double)0.0;

	// gilbert, 20190625
    float		smajax=(float)0.0, sminax=(float)0.0, strike=(float)0.0, sdepth=(float)0.0, uncertainty=(float)0.0;

    // jman, 20170615
    TmInfo tmInfo[TMINFO_NUM];
    for(i=0; i<TMINFO_NUM; i++) 
    {
        tmInfo[i].tm = -1;
        tmInfo[i].mag = -999;
    }

    Program_Name = argv[0];
    elog_init ( argc, argv );

    alarm(0);

    while ((c = getopt (argc, argv, "m:p:r:S:Vv")) != -1)
        switch (c) {

            case 'm':
                match = optarg ;
                break ;

            case 'p':
                pf = (optarg) ;
                break ;

            case 'r':
                reject = optarg ;
                break ;

            case 'S':
                statefile = optarg ;
                break;

            case 'V':
                usage ();
                break;

            case 'v':
                verbose++ ;
                break ;

            case '?':
                errflg++;
        }

    if (errflg || (argc-optind < 1 || argc-optind > 4))
        usage ();

    orbname = argv[optind++];
    dbname  = argv[optind++];

    if ( argc > optind ) {
        after = str2epoch ( argv[optind++] ) ;
        if ( argc > optind ) {
            quit = str2epoch ( argv[optind++] ) ;
            if ( quit < after )
                quit += after ;
        }
    }

    if ( (orb = orbopen ( orbname, "r&" )) < 0 )
        die ( 0, "Can't open ring buffer '%s'\n", orbname ) ;

    if ( match ) {
        if ( (nmatch = orbselect ( orb, match )) < 0 )
            complain ( 1, "orbselect '%s' failed\n", match ) ;
        else
            printf ( "%d sources selected after select\n", nmatch) ;
    }

    if ( reject ) {
        if ( (nmatch = orbreject ( orb, reject )) < 0 )
            complain ( 1, "orbreject '%s' failed\n", match ) ;
        else
            printf ( "%d sources selected after reject\n", nmatch) ;
    }

    if ( after > 0 ) {
        if ((pktid = orbafter ( orb, after )) < 0) {
            complain ( 1, "orbafter to %s failed\n", s=strtime(after) )  ;
            free(s) ;
            pktid = orbtell ( orb ) ;
            printf ("pktid is still #%d\n", pktid ) ;
        } else
            printf ("new starting pktid is #%d\n", pktid ) ;
    }

    if ( statefile != 0 ) {
        char *s ;
        if ( exhume ( statefile, &abort, RT_MAX_DIE_SECS, 0 ) != 0 ) {
            elog_notify ( 0, "read old state file\n" ) ;
        }
        if ( orbresurrect ( orb, &pktid, &time ) == 0 )  {
            elog_notify ( 0, "resurrection successful: repositioned to pktid #%d @ %s\n",
                    pktid, s=strtime(time) ) ;
            free(s) ;
        } else {
            complain ( 0, "resurrection unsuccessful\n" ) ;
        }
    }

    retcode = orbget ( orb, ORBCURRENT, &pktid, srcname, &time, &packet, &nbytes, &bufsize ) ;


    elog_notify(0,"starting at %s\n", strtime(time) ) ;

    // jman, 20170529
    //free(s) ;

    if(pf == NULL )  pf=DEFAULT_PFFILE;
    if (pfread(pf, &get_pf) != 0)
    {
        die(1, "Pf read error.\n");
    }

    ebs_tbl = pfget_tbl(get_pf, "EBS");
    ebss = maxtbl(ebs_tbl);
    if( verbose )
        elog_notify(0,"ebs server : %i\n", ebss ) ;

    for (i = 0; i < ebss; i++)
    {
        tmpstr = (char *) gettbl(ebs_tbl, i);
        elog_notify(0,"ebs server[%d]: %s\n", i, tmpstr) ;
    }

    ecms_tbl = pfget_tbl(get_pf, "ECMS");
    ecmss = maxtbl(ecms_tbl);

    if( verbose )
        elog_notify(0,"ecms  server : %i\n", ecmss ) ;

    if( ebss <=0 && ecmss <=0 )
    {
        die(1, " can't ebs server and cems server in pf file\n");
    }

/*
	stachan = (STACHAN *) malloc( sizeof( STACHAN ) );

    if( (numsite=read_site_db(dbname)) <= 0)
    {
        fprintf(stderr,"can't read site_chan from database\n");
        exit(1);
    }
    else
    {
        elog_notify(0,"reading the station of %i from database  \n", numsite);
        for(i=0 ; i < numsite; i++ )
        {
            stachan = gettbl( Stachans, i );
            printf("setting %s_%s_%s \n", stachan->net, stachan->sta, stachan->chan);
        }
    }

*/

    printf("Setting complete.... \n");

    while ( ! abort )
    {
        signal( SIGALRM, alarm_proc );

        alarm(30);

        pg_alarm = 1;

        retcode = orbreap ( orb, &pktid, srcname, &time, &packet, &nbytes, &bufsize ) ;

/*
		if ( retcode < 0 )
        {
            alarm(0);
            if (pg_alarm == 2)
            {
                //send2EWMS
                double curr_time = now();
                sprintf(wk_msg, "1300%20.20s", epoch2str(curr_time + 9*3600, "%Y%m%d%H%M%S.%s"));
                for(i=0; i< ebss; i++)
                {
                    tmpstr = (char *) gettbl(ebs_tbl, i);
                    sscanf(tmpstr, "%s %d", server_ip, &ecms_port);
                    nRtv = send2ECMS(server_ip, ecms_port, wk_msg);
                    if (nRtv < 0)
                    {
                        elog_notify(0,"%s[%d] send2ECMS(%s,%d,...):%d(%s)\n",
                                    __func__, __LINE__,
                                    server_ip, ecms_port, nRtv, strerror(errno) ) ;
                    }
                }
                continue;
            }
            else
            {
                complain ( 1, "\norbreap fails\n" ) ;
                break ;
            }
        }

        alarm(0);

        if ( strncmp (srcname, "/db/detection", 13) == 0 )
        {
            static Packet   *unstuffed=0 ;
            type = unstuffPkt (srcname, time, packet, nbytes, &unstuffed);
	    
            dbgetv( unstuffed->db, 0, "sta", sta, 0);
            dbgetv( unstuffed->db, 0, "chan", chan, 0);
            dbgetv( unstuffed->db, 0, "time", &dettime, 0);
            dbgetv( unstuffed->db, 0, "state", state, 0);
            dbgetv( unstuffed->db, 0, "filter", filter, 0);
            dbgetv( unstuffed->db, 0, "snr", &snr, 0);
            dbgetv( unstuffed->db, 0, "lddate", &lddate, 0);

            stachan = lookup_stachan( &sta[0], &chan[0] );
            if( stachan->net == NULL ) continue;

            if( strncmp(state,"l", 1) ==0 )
            {
                int iState = setDetectInfo(sta, HN_ON);
                if ( iState != HN_ON)
                {
                    // HN_SAME or error
                    elog_notify(0, "%s[%d]setDetectInfo(%s, %d) = %d(%s)   dettime:%.lf\n",
                            __func__, __LINE__, sta, HN_ON, iState, strerror(errno), dettime);
                    continue;
                }

                wk_state = 0;
                sprintf(wk_srcname,"%s_%s_%s", stachan->net, sta, chan);
                sprintf(wk_msg,"1000%-30.30s%-20.6f%1.1d@",wk_srcname, dettime, wk_state);
                if( verbose )
                    elog_notify(0,"Detection  ON of l :%s\n", wk_msg);
                for(i=0; i< ecmss; i++)
                {
                    tmpstr = (char *) gettbl( ecms_tbl, i);
                    sscanf(tmpstr,"%s %d", server_ip, &ecms_port);
                    nRtv = send2KQMS(server_ip, ecms_port, wk_msg);
                    if (nRtv < 0)
                    {
                        elog_notify(0,"%s[%d] send2KQMS(%s,%d,...):%d(%s)\n",
                                    __func__, __LINE__,
                                    server_ip, ecms_port, nRtv, strerror(errno) ) ;
                    }
                    else
                    {
                        elog_notify(0,"send2KQMS Detect  ON of l :%s to %s %i\n", wk_msg, server_ip, ecms_port);
                    }
                }
            }

        }

*/
        if ( strncmp (srcname, "/pf/orb2dbt", 11) ==0 )
        {
            char *row;
            int evid=(int)0, orid=(int)0, jdata=(int)0, nass, ndef, ndp, grn, srn;
            float lat=(float)0.0, lon=(float)0.0, depth=(float)0.0;
            double curr_time, origin_time;
            char etype1[8], etype2[8], dtype[2], algo[16], auth[16], lddate[18], magtype[5], auth1[10], auth2[10] , dm_code[10];
            float mb, ms, ml, depdp, uncert;
            int mbid, msid, mlid, commid, magid, nsta;
            char wk_otime[255], wk_ctime[255];
            
            // jman, 20170615
            int is_exist;
            int exist_cnt;


            orbpkt2pf(packet,nbytes,&pktpf);


            // jman, 20170615
            row = pfget_string(pktpf,"magnitude_update");
            if( verbose )
                elog_notify(0,"magnitude_update: %s\n",row);
           
            row = pfget_string(pktpf,"disposition");
	        if( verbose )
		        elog_notify(0,"disposition: %s\n", row);
 

            row = pfget_string(pktpf,"origin");

            sscanf(row, "%f %f %f %lf %i %i %i %i %i %i %i %i %s %s %f %s %f %i %f %i %f %i %s %s %s %i %s",
                    &lat, &lon, &depth, &origin_time, &orid, &evid, &jdata, &nass, &ndef, &ndp, &grn, &srn, \
                    etype1, etype2, &depdp, dtype, &mb, &mbid, &ms, &msid, &ml, &mlid, algo, auth1, auth2, &commid, lddate);

            if( verbose )
                elog_notify(0,"%f %f %f %lf %i %i %i %i %i %i %i %i %s %s %f %s %f %i %f %i %f %i %s %s %s %i %s \n",
                        lat, lon, depth, origin_time, orid, evid, jdata, nass, ndef, ndp, grn, srn, \
                        etype1, etype2, depdp, dtype, mb, mbid, ms, msid, ml, mlid, algo, auth1, auth2, commid, lddate);

            //////////////////////
            // Time Check 
            //////////////////////

            curr_time = now();

            if (curr_time > ((double)600.0 + origin_time))
            //if (curr_time > ((double)31536000.0 + origin_time))
            {
                elog_notify(0, "\n Very Old Time(OriginTime: %s , CurrentTime: %s)\n \n",
                            epoch2str(origin_time, "%Y%m%d%H%M%S.%s"), epoch2str(curr_time, "%Y%m%d%H%M%S.%s") );
                check_origin = 0;
                continue;
            }

            // jman, 20170615
            // start new logic about send to EWMS -----------------------------

            // elimite old data
            for (i=0; i<TMINFO_NUM; i++) 
            {
                if ( fabs(tmInfo[i].tm - curr_time) > (double) 720.0 )
                //if ( fabs(tmInfo[i].tm - curr_time) > (double) 31536000.0 )
                {
                    tmInfo[i].tm = -1;
                    tmInfo[i].mag = -999;
                }
            }
            elog_notify(0,"TmInfo elimite old data");

            // case magnitude > 0.0
            if ( ml > (double)0.0) 
            {
                // search & insert
                is_exist = 0;
                for (i=0; i<TMINFO_NUM; i++) 
                {
                    //elog_notify(0,"Compare data.. %f %f\n", origin_time, tmInfo[i].tm);
                    if ( fabs(origin_time - tmInfo[i].tm) <= (double) 5.0 )
                    {
                        is_exist = 1;
                        break;
                    }
                }

                // not found
                if ( is_exist == 0)
                {
                    for (i=0; i<TMINFO_NUM; i++) 
                    {
                        if ( tmInfo[i].tm == -1 )
                        {
                            tmInfo[i].tm = origin_time;
                            tmInfo[i].mag = ml;

                            // send to EWMS
                            //sprintf(wk_msg, "1100%8d%8d%-10.3f%-10.3f%-10.2f%10.2f", evid, orid, lat, lon, depth, ml);
                            sprintf(wk_msg, "1100%8d%8d%10.4f%10.4f%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", evid, orid, lat, lon, depth, ml, smajax, sminax, strike, sdepth, uncertainty);
                            sprintf(wk_otime, epoch2str(origin_time + 9.*3600., "%Y%m%d%H%M%S.%s"));
                            sprintf(wk_ctime, epoch2str(curr_time + 9.*3600., "%Y%m%d%H%M%S.%s"));
                            //sprintf(wk_msg,"%s%-20.20s%-20.20s",wk_msg, wk_otime, wk_ctime);
                            sprintf(wk_msg,"%s%20.20s%20.20s",wk_msg, wk_otime, wk_ctime);
							

                            /**/
                            for(i=0; i< ebss; i++)
                            {
                                tmpstr = (char *) gettbl( ebs_tbl, i);
                                sscanf(tmpstr,"%s %d %s", server_ip, &ecms_port , dm_code);
								sprintf(wk_msg_new,"%s%10s" , wk_msg , dm_code);
								
								// just print log....
                                elog_notify(0,"send Origin :%s \n", wk_msg_new);
								
                                nRtv = send2ECMS(server_ip, ecms_port, wk_msg_new);
                                if (nRtv < 0)
                                {
                                    elog_notify(0,"%s[%d] send2ECMS(%s,%d,...):%d(%s)\n",
                                                __func__, __LINE__,
                                                server_ip, ecms_port, nRtv, strerror(errno) ) ;
                                }
                                else
                                {
                                    elog_notify(0,"send Origin :%s to %s %i\n", wk_msg_new, server_ip, ecms_port);
                                }
                            }
                            /**/

                            break;
                        }
                    }
                }
                else
                {
                    elog_notify(0,"Found in TmInfo. time: %f\n", origin_time);
                }

                // just data count
                exist_cnt = 0;
                for (i=0; i<TMINFO_NUM; i++) 
                {
                    if ( tmInfo[i].tm != -1)
                    {
                        exist_cnt++;
                    }
                }
                elog_notify(0,"Tminfo Data Count :%d \n", exist_cnt);

            }
            else
            {
                elog_notify(0,"ml is minus... ml: %f\n", ml);
            }
            // end new logic about send to EWMS -------------------------------




            if( fabs(origin_time - old_origin_time) > (double) 120.0 )
            {
                 check_origin =1;
                 old_origin_time = origin_time;
                 elog_notify(0, " check_origin : %i \n", check_origin);
           
            }
            else
            {
		    if( check_origin != 2 && ml < (double)0.0) continue;
                 elog_notify(0, " check_origin : %i  ml : %lf \n", check_origin, ml);
            }

            if( check_origin == 1 )
            {
                ml = -1;
                sprintf(wk_msg, "1000%8d%8d%-10.3f%-10.3f%-10.2f%10.2f", evid, orid, lat, lon, depth, ml);
                sprintf(wk_otime, epoch2str(origin_time + 9.*3600., "%Y%m%d%H%M%S.%s"));
                sprintf(wk_ctime, epoch2str(curr_time + 9.*3600., "%Y%m%d%H%M%S.%s"));
                sprintf(wk_msg,"%s%-20.20s%-20.20s",wk_msg, wk_otime, wk_ctime);

		        if (verbose)
                {
                    elog_notify(0,"First Information  :%s\n", wk_msg);
                }

                // jman, 20170531
                //for(i=0; i< ebss; i++)
                //{
                //    tmpstr = (char *) gettbl( ebs_tbl, i);
                //    sscanf(tmpstr,"%s %d", server_ip, &ecms_port);
                //    nRtv = send2ECMS(server_ip, ecms_port, wk_msg);
                //    if (nRtv < 0)
                //    {
                //        elog_notify(0,"%s[%d] send2ECMS(%s,%d,...):%d(%s)\n",
                //                    __func__, __LINE__,
                //                    server_ip, ecms_port, nRtv, strerror(errno) ) ;
                //    }
                //    else
                //    {
                //        elog_notify(0,"send Origin :%s to %s %i\n", wk_msg, server_ip, ecms_port);
                //    }
                //}
                

                sprintf(wk_msg, "8000%-10.3lf%-10.3lf%-10.2lf%-20.3lf%c@", lat, lon, depth, origin_time, ' ');

                for(i=0; i< ecmss; i++)
                {
                    tmpstr = (char *) gettbl( ecms_tbl, i);
                    sscanf(tmpstr,"%s %d", server_ip, &ecms_port);
                    nRtv = send2KQMS(server_ip, ecms_port, wk_msg);
                    if (nRtv < 0)
                    {
                        elog_notify(0,"%s[%d] send2KQMS(%s,%d,...):%d(%s)\n",
                                    __func__, __LINE__,
                                    server_ip, ecms_port, nRtv, strerror(errno) ) ;
                    }
                    else
                    {
			// jman, 20170531
                        //elog_notify(0,"send2KQMS Epicentor to %s %i\n", wk_msg, server_ip, ecms_port);
                        elog_notify(0,"send2KQMS Epicentor to %s %s %i\n", wk_msg, server_ip, ecms_port);
                    }
                }
	        check_origin =2;
            }
            else if( check_origin == 2 )
            {
                //sprintf(wk_msg, "1100%4d%4d%-8.3f", evid, orid, ml);  
                //sprintf(wk_msg, "1100%8d%8d%-10.3f%-10.3f%-10.2f%10.2f", evid, orid, lat, lon, depth, ml);
                sprintf(wk_msg, "1100%8d%8d%10.4f%10.4f%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", evid, orid, lat, lon, depth, ml, smajax, sminax, strike, sdepth, uncertainty);
                sprintf(wk_otime, epoch2str(origin_time + 9.*3600., "%Y%m%d%H%M%S.%s"));
                sprintf(wk_ctime, epoch2str(curr_time + 9.*3600., "%Y%m%d%H%M%S.%s"));
                //sprintf(wk_msg,"%s%-20.20s%-20.20s",wk_msg, wk_otime, wk_ctime);
                sprintf(wk_msg,"%s%20.20s%20.20s",wk_msg, wk_otime, wk_ctime);

                if (verbose)
                {
                    elog_notify(0,"Second Information  :%s\n", wk_msg);
                }

		/*
                for(i=0; i< ebss; i++)
                {
                    tmpstr = (char *) gettbl( ebs_tbl, i);
                    sscanf(tmpstr,"%s %d", server_ip, &ecms_port);
                    nRtv = send2ECMS(server_ip, ecms_port, wk_msg);
                    if (nRtv < 0)
                    {
                        elog_notify(0,"%s[%d] send2ECMS(%s,%d,...):%d(%s)\n",
                                    __func__, __LINE__,
                                    server_ip, ecms_port, nRtv, strerror(errno) ) ;
                    }
                    else
                    {
                        elog_notify(0,"send Origin :%s to %s %i\n", wk_msg, server_ip, ecms_port);
                    }
                }
		*/
            
                check_origin = 0;
                check_time = curr_time;
                {
                     char sTemp[512] = "";
                     char sEvID[20] = "";
                     char sMl[20] = "";
                     char sTime[30] = "";
                     sprintf(sTemp, "%-10d", evid);
                     sprintf(sEvID, "%-10.10s", sTemp);
                     sprintf(sTemp, "%-10.2lf", ml);
                     sprintf(sMl,   "%-10.10s", sTemp);
                     sprintf(sTemp, "%-20.3lf", origin_time);
                     sprintf(sTime, "%-20.20s", sTemp);
                     sprintf(wk_msg, "8100%-10.10s%-10.10s%-10.10s%-20.20s%c@", sEvID, "", sMl, sTime, ' ');
                }
/*

		if( verbose )
                {
                    elog_notify(0,"\n #### Origin Second :%s\n", wk_msg);
                }
*/

                for(i=0; i< ecmss; i++)
                {
                    tmpstr = (char *) gettbl( ecms_tbl, i);                    
		    sscanf(tmpstr,"%s %d %s", server_ip, &ecms_port , dm_code);								
		    sprintf(wk_msg_new,"%s%10s" , wk_msg , dm_code);
					
                    nRtv = send2KQMS(server_ip, ecms_port, wk_msg_new);
                    if (nRtv < 0)
                    {
                        elog_notify(0,"%s[%d] send2KQMS(%s,%d,...):%d(%s)\n",
                                    __func__, __LINE__,
                                    server_ip, ecms_port, nRtv, strerror(errno) ) ;
                    }
                    else
                    {
                        elog_notify(0,"send2KQMS Origin :%s to %s %i\n", wk_msg_new, server_ip, ecms_port);
                    }
                }
                before_origin_time = origin_time;
            }
            else
            {
                elog_notify(0,"Error check_origin's value: %d\n", check_origin);
                check_origin = 0;
            }
        }

    }
    free(packet) ;
    end_time = now() + (float)9.*3600 ;
    elog_notify(0,"finished at %s\n", epoch2str(end_time, "%Y%m%d%H%M%S.%s"));
    return 0 ;
}

int send2ECMS(char *a_pcSvrAddr, int a_iPort, char *a_pcSendBuff)
{
    int iSock = 0, nRtv = 0;
    int nTotSend = 0, nRemainLen = 0;
    struct sockaddr_in server_addr;

    if ((a_pcSendBuff == NULL) || (a_iPort < 0) || (a_pcSendBuff == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(a_pcSvrAddr);
    server_addr.sin_port        = htons(a_iPort);

    if ((iSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        close(iSock);
        return -1;
    }

    if (connect(iSock,(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(iSock);
        return -2;
    }

    nRemainLen = strlen(a_pcSendBuff);
    while (nRemainLen)
    {
        if ((nRtv = send(iSock, a_pcSendBuff + nTotSend, nRemainLen, 0)) <= 0)
        {
            close(iSock);
            return -1;
        }
        nTotSend += nRtv;
        nRemainLen -= nRtv;
    }
    close(iSock);
    return 0;
}

int send2KQMS(char *a_pcSvrAddr, int a_iPort, char *a_pcSendBuff)
{
    int iUdpSock = 0;
    int iRtv = 0;
    int iErrno = 0;
    SOCKADDR_IN stAddress;

    memset(&stAddress, 0, sizeof(stAddress));
    stAddress.sin_family = AF_INET;
    stAddress.sin_addr.s_addr = inet_addr(a_pcSvrAddr);
    stAddress.sin_port = htons(a_iPort);

    iUdpSock = UDPSock();
    if (iUdpSock < 0)
    {
        return -1;
    }

    iRtv = sendtoNdata(iUdpSock, a_pcSendBuff, strlen(a_pcSendBuff), &stAddress);
    if (iRtv != strlen(a_pcSendBuff))
    {
        iErrno = errno;
        close(iUdpSock);
        errno = iErrno;
        return -1;
    }

    close(iUdpSock);
    return 0;
}

static void alarm_proc()
{
    pg_alarm = 2;
}

int setDetectInfo(const char *ac_pcSta, const int ac_iState)
{
    static StaInfo *s_pstStaInfo;
    static int s_iStaCount = 0;

    int iMemSize;
    int iIndex;
    int iState = HN_SAME;
    int iOldState = HN_OFF;

    if ((ac_pcSta == NULL) || ((ac_iState != HN_OFF) && (ac_iState != HN_ON)))
    {
        errno = EINVAL;
        return -1;
    }

    if (s_iStaCount == 0)
    {
        iMemSize = HN_STA_COUNT * sizeof(StaInfo);
        s_pstStaInfo = (StaInfo *)malloc(iMemSize);
        if (s_pstStaInfo == NULL)
        {
            return -1;
        }
        s_iStaCount = HN_STA_COUNT;
        memset(s_pstStaInfo, 0, iMemSize);
    }

    for (iIndex = 0; iIndex < s_iStaCount; iIndex++)
    {
        if (strlen(s_pstStaInfo[iIndex].m_sSta) == 0)
        {
#if 0
            elog_notify(0, "%s[%d]add station()\n", __func__, __LINE__);
#endif

            // m_sSta? ª˜¾ÄÂãþÈ ùåÁî ??
            strcpy(s_pstStaInfo[iIndex].m_sSta, ac_pcSta);
            s_pstStaInfo[iIndex].m_iState = HN_OFF;
            iOldState = HN_OFF;
            break;
        }
        if (strcmp(s_pstStaInfo[iIndex].m_sSta, ac_pcSta) == 0)
        {
            iOldState = s_pstStaInfo[iIndex].m_iState;
            break;
        }
    }

    if (iIndex == s_iStaCount)
    {
        elog_notify(0, "%s[%d]realloc()\n", __func__, __LINE__);

        // s_iStaCount ¬ ?ùöÞÓ ùåÁî s_iStaCount ¬ þ® ½ùì Ã•Ã—ùêÃ±ª  ¬ ³Ä¹ð.
        iMemSize = sizeof(StaInfo) * (s_iStaCount + HN_STA_COUNT);
        s_pstStaInfo = (StaInfo *)realloc(s_pstStaInfo, iMemSize);
        if (s_pstStaInfo == NULL)
        {
            return -1;
        }
        memset(&s_pstStaInfo[s_iStaCount], 0, sizeof(StaInfo) * HN_STA_COUNT);
        s_iStaCount += HN_STA_COUNT;

        strcpy(s_pstStaInfo[iIndex].m_sSta, ac_pcSta);
        s_pstStaInfo[iIndex].m_iState = HN_OFF;
        iOldState = HN_OFF;
    }

    if (iOldState == ac_iState)
    {
        iState = HN_SAME;
    }
    else
    {
        iState = ac_iState;
        s_pstStaInfo[iIndex].m_iState = ac_iState;
    }

    return iState;
}

