// -------------------------------------------------------------------------------------------------
//  File plt-posix.h
//
//  Copyright (c) 2016, 2017 DexLogic, Dirk Apitz
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//
// -------------------------------------------------------------------------------------------------
//  Change History:
//
//  06/2017 Dirk Apitz, created
// -------------------------------------------------------------------------------------------------


#ifndef PLT_POSIX_H
#define PLT_POSIX_H


// Standard libraries
#include <unistd.h>
#include <errno.h>
#include <time.h>

// Platform headers
#include <arpa/inet.h>


// -------------------------------------------------------------------------------------------------
//  Inline functions
// -------------------------------------------------------------------------------------------------

inline static int plt_getLastError()
{
    return errno;
}


inline static int plt_initialize()
{
    extern struct timespec tsRef;
    extern uint32_t currTimeUS;

    extern void logError(const char *fmt, ...);
    extern void logInfo(const char *fmt, ...);

    // Initialize time reference
    if(clock_gettime(CLOCK_MONOTONIC, &tsRef) < 0)
    {
        logError("clock_gettime(CLOCK_MONOTONIC) errno = %d", errno);
        return -1;
    }

    // Set the current time stamp randomly
    currTimeUS = (uint32_t)((tsRef.tv_sec * 1000000ul) + (tsRef.tv_nsec / 1000));

    return 0;
}


inline static uint32_t plt_getSystemTimeUS()
{
    extern struct timespec tsRef;
    extern uint32_t currTimeUS;

    // Get current time
    struct timespec tsNow, tsDiff;
    clock_gettime(CLOCK_MONOTONIC, &tsNow);

    // Determine difference to reference time
	if(tsNow.tv_nsec < tsRef.tv_nsec) 
    {
		tsDiff.tv_sec = (tsNow.tv_sec - tsRef.tv_sec) - 1;
		tsDiff.tv_nsec = (1000000000 + tsNow.tv_nsec) - tsRef.tv_nsec;
	} 
    else 
    {
		tsDiff.tv_sec = tsNow.tv_sec - tsRef.tv_sec;
		tsDiff.tv_nsec = tsNow.tv_nsec - tsRef.tv_nsec;
	}

    // Update internal counters
    currTimeUS += (uint32_t)((tsDiff.tv_sec * 1000000) + (tsDiff.tv_nsec / 1000));
    tsRef = tsNow;

    return currTimeUS;
}


inline static int plt_usleep(unsigned usec)
{
    return usleep(usec);
}


inline static int plt_sockOpen(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}


inline static int plt_sockClose(int sockFD)
{
    return close(sockFD);
}


inline static FILE *plt_fopen(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}


#endif

