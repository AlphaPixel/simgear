/***************************************************************************
 * stopwatch.hxx        Timer class, for use in benchmarking
 *
 * Based on blitz/Timer.h
 *
 * $Id$
 *
 * Copyright (C) 1997,1998 Todd Veldhuizen <tveldhui@seurat.uwaterloo.ca>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Suggestions:          blitz-suggest@cybervision.com
 * Bugs:                 blitz-bugs@cybervision.com
 *
 * For more information, please see the Blitz++ Home Page:
 *    http://seurat.uwaterloo.ca/blitz/
 *
 ***************************************************************************
 * $Log$
 * Revision 1.1  1999/04/05 21:32:33  curt
 * Initial revision
 *
 * Revision 1.3  1998/11/20 01:01:03  curt
 * FreeBSD support.
 *
 * Revision 1.2  1998/11/02 18:28:31  curt
 * Additional win32 support.
 *
 * Revision 1.1  1998/09/01 19:06:30  curt
 * Initial revision.
 *
 * Revision 1.4  1998/03/14 00:04:47  tveldhui
 * 0.2-alpha-05
 *
 * Revision 1.3  1997/07/16 14:51:20  tveldhui
 * Update: Alpha release 0.2 (Arrays)
 *
 * Revision 1.2  1997/01/24 14:42:00  tveldhui
 * Periodic RCS update
 *
 */

// This class is not portable to non System V platforms.
// It will need to be rewritten for Windows, NT, Mac.
// NEEDS_WORK

#ifndef _STOPWATCH_HXX
#define _STOPWATCH_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if defined(__linux__) && ! defined(HAVE_GETRUSAGE)
#  define HAVE_GETRUSAGE
#endif

#if defined( WIN32 ) && defined( HAVE_GETRUSAGE )
#  undef HAVE_GETRUSAGE
#endif // WIN32

#if defined( HAVE_GETRUSAGE )
#  if defined( __FreeBSD__ )
#    include <sys/types.h>
#  endif 
#  include <sys/time.h>
#  include <sys/resource.h>
#  include <unistd.h>
#elif defined( WIN32 )
#  include <windows.h>
#else
#  include <time.h>
#endif

class StopWatch {

public:
    StopWatch() 
    { 
//         state_ = uninitialized;
    }

    void start()
    { 
//         state_ = running;
        t1_ = systemTime();
    }

    void stop()
    {
        t2_ = systemTime();
// 	BZPRECONDITION(state_ == running);
// 	state_ = stopped;
    }

    double elapsedSeconds()
    {
//         BZPRECONDITION(state_ == stopped);
        return t2_ - t1_;
    }

private:
    StopWatch(StopWatch&) { }
    void operator=(StopWatch&) { }

    double systemTime()
    {
#if defined( HAVE_GETRUSAGE )
        getrusage(RUSAGE_SELF, &resourceUsage_);
        double seconds = resourceUsage_.ru_utime.tv_sec 
            + resourceUsage_.ru_stime.tv_sec;
        double micros  = resourceUsage_.ru_utime.tv_usec 
            + resourceUsage_.ru_stime.tv_usec;
        return seconds + micros/1.0e6;
#elif defined( WIN32 )
	return double(GetTickCount()) * double(1e-3);
#else
        return clock() / (double) CLOCKS_PER_SEC;
#endif
    }

//     enum { uninitialized, running, stopped } state_;

#if defined( HAVE_GETRUSAGE )
    struct rusage resourceUsage_;
#endif

    double t1_, t2_;
};

#endif // _STOPWATCH_HXX

