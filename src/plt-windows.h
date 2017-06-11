// -------------------------------------------------------------------------------------------------
//  File plt-windows.h
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


#ifndef PLT_WINDOWS_H
#define PLT_WINDOWS_H


// Standard libraries
#include <stdint.h>

// Platform headers
#include <windows.h>


// -------------------------------------------------------------------------------------------------
//  Typedefs
// -------------------------------------------------------------------------------------------------

typedef unsigned long in_addr_t;


// -------------------------------------------------------------------------------------------------
//  Inline functions
// -------------------------------------------------------------------------------------------------

inline static int plt_initialize()
{
    return 0;
}


inline static uint32_t plt_getSystemTimeUS(void)
{
    ULARGE_INTEGER ul;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    // Fill ULARGE_INTEGER low and high parts.
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;

    // Convert to microseconds (FILETIME is in 100 nanosecont intervals).
    return (uint32_t)(ul.QuadPart / 10ULL);
}


inline static int plt_usleep(unsigned usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    // Convert to 100 nanosecond interval, negative value indicates relative time
    ft.QuadPart = -(10 * (__int64)usec);
    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);

    return 0;
}


inline static FILE *plt_fopen(const char *filename, const char *mode)
{
    FILE *fp;
    errno_t err = fopen_s(&fp, filename, mode);
    if(err != 0) return (FILE *)0;

    return fp;
}

inline static int plt_sockStartup()
{
    // Initialize Winsock
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}


inline static int plt_sockCleanup()
{
    return WSACleanup();
}


inline static int plt_sockGetLastError()
{
    return WSAGetLastError();
}


inline static int plt_sockOpen(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}


inline static int plt_sockClose(int fdSocket)
{
    return closesocket(fdSocket);
}


#endif

