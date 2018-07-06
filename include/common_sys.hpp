/*
 * common_sys.hpp
 *
 *  Created on: 2017年3月13日
 *      Author: chenhuapan
 */

#ifndef INCLUDE_COMMON_SYS_HPP_
#define INCLUDE_COMMON_SYS_HPP_

#include <sys/resource.h>
#include <sys/select.h>
#include "common_include.h"

#define SLEEP(X) do{ usleep(X*1000); }while(0)

// safe delete
#define SAFE_DEL(X) do{ if(NULL!=X){ delete X; X=NULL; } }while(0)
#define SAFE_DEL_ARRAY(X) do{ if(NULL!=X){ delete [] X; X=NULL; } }while(0)

inline
int SetRLimitForNofile( int nfds )
{
	   if( nfds <= 0 )
		{
			return -1;
		}

	    struct rlimit rlim;

	    rlim.rlim_cur = rlim.rlim_max = nfds + 100;
	    if ( setrlimit(RLIMIT_NOFILE, &rlim) < 0 )
	    {
	        printf( "setlimit wrong! nfds=%d", nfds );
	        perror( "setlimit" );
	        return -1;
	    }

	    getrlimit(RLIMIT_NOFILE, &rlim);
	    printf( "Max Open file: %d\n", (int)(rlim.rlim_cur) );

	    return rlim.rlim_cur;
}

inline
int mSleep(unsigned long lMs)
{
	struct timeval stTv;

	stTv.tv_sec = lMs / 1000;
	stTv.tv_usec = (lMs % 1000) * 1000;
	select(0, NULL, NULL, NULL, &stTv);
	return 0;
}



#endif /* INCLUDE_COMMON_SYS_HPP_ */
