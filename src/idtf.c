// -------------------------------------------------------------------------------------------------
//  File idtf.c
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
//  05/2016 Dirk Apitz, created
//  05/2016 Dirk Apitz, formats 2, 4, 5
//  06/2016 Dirk Apitz, scale, mirror
//  08/2016 Theo Dari / Dirk Apitz, Windows port
// -------------------------------------------------------------------------------------------------

// Standard libraries
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Platform includes
#if defined(_WIN32) || defined(WIN32)
#include "plt-windows.h"
#else
#include "plt-posix.h"
#endif

// Module header
#include "idtf.h"


// -------------------------------------------------------------------------------------------------
//  Defines
// -------------------------------------------------------------------------------------------------

#define ILDACOLOR(r, g, b)      (((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))


// -------------------------------------------------------------------------------------------------
//  Prototypes
// -------------------------------------------------------------------------------------------------

void logError(const char *fmt, ...);
void logInfo(const char *fmt, ...);


// -------------------------------------------------------------------------------------------------
//  Variables
// -------------------------------------------------------------------------------------------------

static unsigned long ildaDefaultPalette[256] =      // LFI / Aura Technologies
{
    ILDACOLOR(255, 0,   0),                         // Red
    ILDACOLOR(255, 16,  0),
    ILDACOLOR(255, 32,  0),
    ILDACOLOR(255, 48,  0),
    ILDACOLOR(255, 64,  0),
    ILDACOLOR(255, 80,  0),
    ILDACOLOR(255, 96,  0),
    ILDACOLOR(255, 112, 0),
    ILDACOLOR(255, 128, 0),
    ILDACOLOR(255, 144, 0),
    ILDACOLOR(255, 160, 0),
    ILDACOLOR(255, 176, 0),
    ILDACOLOR(255, 192, 0),
    ILDACOLOR(255, 208, 0),
    ILDACOLOR(255, 224, 0),
    ILDACOLOR(255, 240, 0),
    ILDACOLOR(255, 255, 0),                         // Yellow
    ILDACOLOR(224, 255, 0),
    ILDACOLOR(192, 255, 0),
    ILDACOLOR(160, 255, 0),
    ILDACOLOR(128, 255, 0),
    ILDACOLOR(96,  255, 0),
    ILDACOLOR(64,  255, 0),
    ILDACOLOR(32,  255, 0),
    ILDACOLOR(0,   255, 0),                         // Green
    ILDACOLOR(0,   255, 36),
    ILDACOLOR(0,   255, 73),
    ILDACOLOR(0,   255, 109),
    ILDACOLOR(0,   255, 146),
    ILDACOLOR(0,   255, 182),
    ILDACOLOR(0,   255, 219),
    ILDACOLOR(0,   255, 255),                       // Cyan
    ILDACOLOR(0,   227, 255),
    ILDACOLOR(0,   198, 255),
    ILDACOLOR(0,   170, 255),   
    ILDACOLOR(0,   142, 255),
    ILDACOLOR(0,   113, 255),
    ILDACOLOR(0,   85,  255),
    ILDACOLOR(0,   56,  255),
    ILDACOLOR(0,   28,  255),
    ILDACOLOR(0,   0,   255),                       // Blue
    ILDACOLOR(32,  0,   255),
    ILDACOLOR(64,  0,   255),
    ILDACOLOR(96,  0,   255),
    ILDACOLOR(128, 0,   255),
    ILDACOLOR(160, 0,   255),
    ILDACOLOR(192, 0,   255),
    ILDACOLOR(224, 0,   255),
    ILDACOLOR(255, 0,   255),                       // Magenta
    ILDACOLOR(255, 32,  255),
    ILDACOLOR(255, 64,  255),
    ILDACOLOR(255, 96,  255),
    ILDACOLOR(255, 128, 255),
    ILDACOLOR(255, 160, 255),
    ILDACOLOR(255, 192, 255),
    ILDACOLOR(255, 224, 255),
    ILDACOLOR(255, 255, 255),                       // White
    ILDACOLOR(255, 224, 224),
    ILDACOLOR(255, 192, 192),
    ILDACOLOR(255, 160, 160),
    ILDACOLOR(255, 128, 128),
    ILDACOLOR(255, 96,  96),
    ILDACOLOR(255, 64,  64),
    ILDACOLOR(255, 32,  32),

    // Unused index entries. Set grey for debuggung
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(64,  64,  64),
};


static unsigned long ildaStandardPalette[256] =     // ILDA color palette standard (obsolete)
{
    ILDACOLOR(0,   0,   0),
    ILDACOLOR(255, 255, 255),
    ILDACOLOR(255, 0,   0),
    ILDACOLOR(255, 255, 0),
    ILDACOLOR(0,   255, 0),
    ILDACOLOR(0,   255, 255),
    ILDACOLOR(0,   0,   255),
    ILDACOLOR(255, 0,   255),
    ILDACOLOR(255, 128, 128),
    ILDACOLOR(255, 140, 128),
    ILDACOLOR(255, 151, 128),
    ILDACOLOR(255, 163, 128),
    ILDACOLOR(255, 174, 128),
    ILDACOLOR(255, 186, 128),
    ILDACOLOR(255, 197, 128),
    ILDACOLOR(255, 209, 128),
    ILDACOLOR(255, 220, 128),
    ILDACOLOR(255, 232, 128),
    ILDACOLOR(255, 243, 128),
    ILDACOLOR(255, 255, 128),
    ILDACOLOR(243, 255, 128),
    ILDACOLOR(232, 255, 128),
    ILDACOLOR(220, 255, 128),
    ILDACOLOR(209, 255, 128),
    ILDACOLOR(197, 255, 128),
    ILDACOLOR(186, 255, 128),
    ILDACOLOR(174, 255, 128),
    ILDACOLOR(163, 255, 128),
    ILDACOLOR(151, 255, 128),
    ILDACOLOR(140, 255, 128),
    ILDACOLOR(128, 255, 128),
    ILDACOLOR(128, 255, 140),
    ILDACOLOR(128, 255, 151),
    ILDACOLOR(128, 255, 163),
    ILDACOLOR(128, 255, 174),
    ILDACOLOR(128, 255, 186),
    ILDACOLOR(128, 255, 197),
    ILDACOLOR(128, 255, 209),
    ILDACOLOR(128, 255, 220),
    ILDACOLOR(128, 255, 232),
    ILDACOLOR(128, 255, 243),
    ILDACOLOR(128, 255, 255),
    ILDACOLOR(128, 243, 255),
    ILDACOLOR(128, 232, 255),
    ILDACOLOR(128, 220, 255),
    ILDACOLOR(128, 209, 255),
    ILDACOLOR(128, 197, 255),
    ILDACOLOR(128, 186, 255),
    ILDACOLOR(128, 174, 255),
    ILDACOLOR(128, 163, 255),
    ILDACOLOR(128, 151, 255),
    ILDACOLOR(128, 140, 255),
    ILDACOLOR(128, 128, 255),
    ILDACOLOR(140, 128, 255),
    ILDACOLOR(151, 128, 255),
    ILDACOLOR(163, 128, 255),
    ILDACOLOR(174, 128, 255),
    ILDACOLOR(186, 128, 255),
    ILDACOLOR(197, 128, 255),
    ILDACOLOR(209, 128, 255),
    ILDACOLOR(220, 128, 255),
    ILDACOLOR(232, 128, 255),
    ILDACOLOR(243, 128, 255),
    ILDACOLOR(255, 128, 255),
    ILDACOLOR(255, 128, 243),
    ILDACOLOR(255, 128, 232),
    ILDACOLOR(255, 128, 220),
    ILDACOLOR(255, 128, 209),
    ILDACOLOR(255, 128, 197),
    ILDACOLOR(255, 128, 186),
    ILDACOLOR(255, 128, 174),
    ILDACOLOR(255, 128, 163),
    ILDACOLOR(255, 128, 151),
    ILDACOLOR(255, 128, 140),
    ILDACOLOR(255, 0,   0),
    ILDACOLOR(255, 23,  0),
    ILDACOLOR(255, 46,  0),
    ILDACOLOR(255, 70,  0),
    ILDACOLOR(255, 93,  0),
    ILDACOLOR(255, 116, 0),
    ILDACOLOR(255, 139, 0),
    ILDACOLOR(255, 162, 0),
    ILDACOLOR(255, 185, 0),
    ILDACOLOR(255, 209, 0),
    ILDACOLOR(255, 232, 0),
    ILDACOLOR(255, 255, 0),
    ILDACOLOR(232, 255, 0),
    ILDACOLOR(209, 255, 0),
    ILDACOLOR(185, 255, 0),
    ILDACOLOR(162, 255, 0),
    ILDACOLOR(139, 255, 0),
    ILDACOLOR(116, 255, 0),
    ILDACOLOR(93,  255, 0),
    ILDACOLOR(70,  255, 0),
    ILDACOLOR(46,  255, 0),
    ILDACOLOR(23,  255, 0),
    ILDACOLOR(0,   255, 0),
    ILDACOLOR(0,   255, 23),
    ILDACOLOR(0,   255, 46),
    ILDACOLOR(0,   255, 70),
    ILDACOLOR(0,   255, 93),
    ILDACOLOR(0,   255, 116),
    ILDACOLOR(0,   255, 139),
    ILDACOLOR(0,   255, 162),
    ILDACOLOR(0,   255, 185),
    ILDACOLOR(0,   255, 209),
    ILDACOLOR(0,   255, 232),
    ILDACOLOR(0,   255, 255),
    ILDACOLOR(0,   232, 255),
    ILDACOLOR(0,   209, 255),
    ILDACOLOR(0,   185, 255),
    ILDACOLOR(0,   162, 255),
    ILDACOLOR(0,   139, 255),
    ILDACOLOR(0,   116, 255),
    ILDACOLOR(0,   93,  255),
    ILDACOLOR(0,   70,  255),
    ILDACOLOR(0,   46,  255),
    ILDACOLOR(0,   23,  255),
    ILDACOLOR(0,   0,   255),
    ILDACOLOR(23,  0,   255),
    ILDACOLOR(46,  0,   255),
    ILDACOLOR(70,  0,   255),
    ILDACOLOR(93,  0,   255),
    ILDACOLOR(116, 0,   255),
    ILDACOLOR(139, 0,   255),
    ILDACOLOR(162, 0,   255),
    ILDACOLOR(185, 0,   255),
    ILDACOLOR(209, 0,   255),
    ILDACOLOR(232, 0,   255),
    ILDACOLOR(255, 0,   255),
    ILDACOLOR(255, 0,   232),
    ILDACOLOR(255, 0,   209),
    ILDACOLOR(255, 0,   185),
    ILDACOLOR(255, 0,   162),
    ILDACOLOR(255, 0,   139),
    ILDACOLOR(255, 0,   116),
    ILDACOLOR(255, 0,   93),
    ILDACOLOR(255, 0,   70),
    ILDACOLOR(255, 0,   46),
    ILDACOLOR(255, 0,   23),
    ILDACOLOR(128, 0,   0),
    ILDACOLOR(128, 12,  0),
    ILDACOLOR(128, 23,  0),
    ILDACOLOR(128, 35,  0),
    ILDACOLOR(128, 47,  0),
    ILDACOLOR(128, 58,  0),
    ILDACOLOR(128, 70,  0),
    ILDACOLOR(128, 81,  0),
    ILDACOLOR(128, 93,  0),
    ILDACOLOR(128, 105, 0),
    ILDACOLOR(128, 116, 0),
    ILDACOLOR(128, 128, 0),
    ILDACOLOR(116, 128, 0),
    ILDACOLOR(105, 128, 0),
    ILDACOLOR(93,  128, 0),
    ILDACOLOR(81,  128, 0),
    ILDACOLOR(70,  128, 0),
    ILDACOLOR(58,  128, 0),
    ILDACOLOR(47,  128, 0),
    ILDACOLOR(35,  128, 0),
    ILDACOLOR(23,  128, 0),
    ILDACOLOR(12,  128, 0),
    ILDACOLOR(0,   128, 0),
    ILDACOLOR(0,   128, 12),
    ILDACOLOR(0,   128, 23),
    ILDACOLOR(0,   128, 35),
    ILDACOLOR(0,   128, 47),
    ILDACOLOR(0,   128, 58),
    ILDACOLOR(0,   128, 70),
    ILDACOLOR(0,   128, 81),
    ILDACOLOR(0,   128, 93),
    ILDACOLOR(0,   128, 105),
    ILDACOLOR(0,   128, 116),
    ILDACOLOR(0,   128, 128),
    ILDACOLOR(0,   116, 128),
    ILDACOLOR(0,   105, 128),
    ILDACOLOR(0,   93,  128),
    ILDACOLOR(0,   81,  128),
    ILDACOLOR(0,   70,  128),
    ILDACOLOR(0,   58,  128),
    ILDACOLOR(0,   47,  128),
    ILDACOLOR(0,   35,  128),
    ILDACOLOR(0,   23,  128),
    ILDACOLOR(0,   12,  128),
    ILDACOLOR(0,   0,   128),
    ILDACOLOR(12,  0,   128),
    ILDACOLOR(23,  0,   128),
    ILDACOLOR(35,  0,   128),
    ILDACOLOR(47,  0,   128),
    ILDACOLOR(58,  0,   128),
    ILDACOLOR(70,  0,   128),
    ILDACOLOR(81,  0,   128),
    ILDACOLOR(93,  0,   128),
    ILDACOLOR(105, 0,   128),
    ILDACOLOR(116, 0,   128),
    ILDACOLOR(128, 0,   128),
    ILDACOLOR(128, 0,   116),
    ILDACOLOR(128, 0,   105),
    ILDACOLOR(128, 0,   93),
    ILDACOLOR(128, 0,   81),
    ILDACOLOR(128, 0,   70),
    ILDACOLOR(128, 0,   58),
    ILDACOLOR(128, 0,   47),
    ILDACOLOR(128, 0,   35),
    ILDACOLOR(128, 0,   23),
    ILDACOLOR(128, 0,   12),
    ILDACOLOR(255, 192, 192),
    ILDACOLOR(255, 64,  64),
    ILDACOLOR(192, 0,   0),
    ILDACOLOR(64,  0,   0),
    ILDACOLOR(255, 255, 192),
    ILDACOLOR(255, 255, 64),
    ILDACOLOR(192, 192, 0),
    ILDACOLOR(64,  64,  0),
    ILDACOLOR(192, 255, 192),
    ILDACOLOR(64,  255, 64),
    ILDACOLOR(0,   192, 0),
    ILDACOLOR(0,   64,  0),
    ILDACOLOR(192, 255, 255),
    ILDACOLOR(64,  255, 255),
    ILDACOLOR(0,   192, 192),
    ILDACOLOR(0,   64,  64),
    ILDACOLOR(192, 192, 255),
    ILDACOLOR(64,  64,  255),
    ILDACOLOR(0,   0,   192),
    ILDACOLOR(0,   0,   64),
    ILDACOLOR(255, 192, 255),
    ILDACOLOR(255, 64,  255),
    ILDACOLOR(192, 0,   192),
    ILDACOLOR(64,  0,   64),
    ILDACOLOR(255, 96,  96),
    ILDACOLOR(255, 255, 255),
    ILDACOLOR(245, 245, 245),
    ILDACOLOR(235, 235, 235),
    ILDACOLOR(224, 224, 224),
    ILDACOLOR(213, 213, 213),
    ILDACOLOR(203, 203, 203),
    ILDACOLOR(192, 192, 192),
    ILDACOLOR(181, 181, 181),
    ILDACOLOR(171, 171, 171),
    ILDACOLOR(160, 160, 160),
    ILDACOLOR(149, 149, 149),
    ILDACOLOR(139, 139, 139),
    ILDACOLOR(128, 128, 128),
    ILDACOLOR(117, 117, 117),
    ILDACOLOR(107, 107, 107),
    ILDACOLOR(96,  96,  96),
    ILDACOLOR(85,  85,  85),
    ILDACOLOR(75,  75,  75),
    ILDACOLOR(64,  64,  64),
    ILDACOLOR(53,  53,  53),
    ILDACOLOR(43,  43,  43),
    ILDACOLOR(32,  32,  32),
    ILDACOLOR(21,  21,  21),
    ILDACOLOR(11,  11,  11),
    ILDACOLOR(0,   0,   0)
};


static unsigned long customPalette[256];


// -------------------------------------------------------------------------------------------------
//  Code
// -------------------------------------------------------------------------------------------------

inline static int checkEOF(FILE *fp, const char *dbgText)
{
    if(feof(fp)) { logError("[IDTF] Unexpected end of file (%s)", dbgText); return -1; }

    return 0;
}


inline static uint16_t readShort(FILE *fp)
{
    uint16_t c1 = ((uint16_t)fgetc(fp)) & 0xFF;
    uint16_t c2 = ((uint16_t)fgetc(fp)) & 0xFF;

    return (uint16_t)((c1 << 8) | c2);
}


// -------------------------------------------------------------------------------------------------
//  API functions
// -------------------------------------------------------------------------------------------------

int idtfRead(char *filename, float xyScale, unsigned options, IDTF_CALLBACK_FUNC *cbFunc, void *cbContext)
{
    // Determine which palette to use
    unsigned palOption = options & IDTFOPT_PALETTE_MASK;
    unsigned long *currentPalette = ildaDefaultPalette;
    if((palOption == 0) || (palOption == IDTFOPT_PALETTE_IDTF_DEFAULT)) { }
    else if(palOption == IDTFOPT_PALETTE_ILDA_STANDARD) { currentPalette = ildaStandardPalette; }
    else { logError("[IDTF] Invalid palette option"); return -1; }

    float xScale = (options & IDTFOPT_MIRROR_X) ? -xyScale : xyScale;
    float yScale = (options & IDTFOPT_MIRROR_Y) ? -xyScale : xyScale;

    // Open the passed file
    FILE *fpIDTF = plt_fopen(filename, "rb");
    if(!fpIDTF) 
    {
        logError("[IDTF] %s: Cannot open file (err = %d)", filename, plt_getLastError());
        return -1;
    }


    // Structure of each section:
    //
    // uint8_t 'I', 'L', 'D', 'A';
    // ----
    // uint8_t reservedA[3]
    // uint8_t formatCode
    // ----
    // uint8_t dataSetName[8]
    // uint8_t companyName[8]
    // ----
    // uint16_t recordCnt               // Big-Endian!!
    // uint16_t dataSetNumber           // Big-Endian!!
    // ----
    // uint16_t dataSetCnt              // Big-Endian!!
    // uint8_t headNumber
    // uint8_t reservedB
    // ----
    // Data Records


    long filePos;
    uint8_t ilda[4];

    // Sanity check - Read signature of first section
    filePos = ftell(fpIDTF);
    fread(ilda, sizeof(ilda), 1, fpIDTF);

    // Check for EOF and the signature of first section
    if(feof(fpIDTF) || !((ilda[0] == 'I') && (ilda[1] == 'L') && (ilda[2] == 'D') && (ilda[3] == 'A')))
    {
        logError("[IDTF] %s: Not an IDTF file", filename);
        fclose(fpIDTF);
        return -1;
    }

    // Rewind for loop start
    fseek(fpIDTF, filePos, SEEK_SET);


    // -------------------------------------------------------------------------
    // OK - Read the file
    // -------------------------------------------------------------------------

    int result = 0;
    while(1)
    {
        // Read section signature. Silently abort in case of (incorrect) EOF.
        filePos = ftell(fpIDTF);
        fread(ilda, sizeof(ilda), 1, fpIDTF);
        if(feof(fpIDTF)) break;

        // Some systems use this signature before appending further (non-IDTF) data...
        if((ilda[0] == 0) && (ilda[1] == 0) && (ilda[2] == 0) && (ilda[3] == 0)) break;

        // Check for IDTF section signature
        if(!((ilda[0] == 'I') && (ilda[1] == 'L') && (ilda[2] == 'D') && (ilda[3] == 'A')))
        {
            logError("[IDTF] %s: Bad section signature at pos 0x%08X", filename, filePos);
            result = -1;
            break;
        }

        // Skip reserved bytes and read format code
        fgetc(fpIDTF);
        fgetc(fpIDTF);
        fgetc(fpIDTF);
        uint8_t formatCode = (uint8_t)fgetc(fpIDTF);
        if((result = checkEOF(fpIDTF, "Header")) != 0) break;

        // Read data set name (frame name or color palette name) and company name
        uint8_t dataSetName[8], companyName[8];
        fread(dataSetName, 8, 1, fpIDTF);
        fread(companyName, 8, 1, fpIDTF);
        if((result = checkEOF(fpIDTF, "Header")) != 0) break;

        //logInfo("dataSetName: %8.8s", dataSetName);
        //logInfo("companyName: %8.8s", companyName);

        // Read record count and data set number (frame number or color palette number)
        uint16_t recordCnt = (uint16_t)readShort(fpIDTF);
        uint16_t dataSetNumber = (uint16_t)readShort(fpIDTF);
        if((result = checkEOF(fpIDTF, "Header")) != 0) break;

        // Read data set count (not used) and head number (not used)
        uint16_t dataSetCnt = (uint16_t)readShort(fpIDTF);
        uint8_t headNumber = (uint8_t)fgetc(fpIDTF);
        fgetc(fpIDTF);

        //logInfo("Format: %d, Records: %d, Set number: %d, Set count: %d, Head: %d",
        //        formatCode, recordCnt, dataSetNumber, dataSetCnt, headNumber);

        // Terminate in case of an empty section (no records - regular end)
        if(recordCnt == 0) break;

        // Handle data section depending on format code
        if((formatCode == 0) || (formatCode == 1) || (formatCode == 4) || (formatCode == 5)) 
        {
            //logInfo("Frame, fmt=%u, filePos 0x%08X", formatCode, filePos);

            // Terminate on insane frames
            if(recordCnt <= 1)
            {
                logError("[IDTF] %s: Frames should contain at least 2 points", filename); 
                result = -1;
                break;
            }

            // Formats 0 and 4 are X, Y, Z; Formats 1 and 5 are X, Y.
            int hasZ = (formatCode == 0) || (formatCode == 4);

            // Formats 0 and 1 have color index; Formats 4 and 5 are true color RGB.
            int hasIndex = (formatCode == 0) || (formatCode == 1);

            // Tell the output to open a frame
            if(cbFunc->openFrame(cbContext)) { result = -1; break; }

            // Loop through all points
            for(int i = 0; i < recordCnt; i++)
            {
                int16_t x, y, z;
                uint8_t statusCode, r, g, b;

                // Remember file pos at the beginning of the point
                filePos = ftell(fpIDTF);

                // Read coordinates
                x = (short)((float)(short)readShort(fpIDTF) * xScale);
                y = (short)((float)(short)readShort(fpIDTF) * yScale);
                if(hasZ) z = readShort(fpIDTF); else z = 0;

                // Read status code
                statusCode = (uint8_t)fgetc(fpIDTF);

                // Read color
                if(hasIndex) 
                {
                    uint8_t colorIndex = (uint8_t)fgetc(fpIDTF);
                    long rgb = currentPalette[colorIndex];
                    r = (uint8_t)(rgb >> 16);
                    g = (uint8_t)(rgb >> 8);
                    b = (uint8_t)rgb;
                }
                else
                {
                    b = (uint8_t)fgetc(fpIDTF);
                    g = (uint8_t)fgetc(fpIDTF);
                    r = (uint8_t)fgetc(fpIDTF);
                }

                // Check for unexpected EOF
                if(feof(fpIDTF)) 
                { 
                    logError("[IDTF] Unexpected end of file: Record %u of %u", i, recordCnt); 
                    result = -1;
                    break;
                }

                // Output the point
                if(statusCode & 0x40) r = g = b = 0;
                if(cbFunc->putSampleXYRGB(cbContext, x, y, r, g, b)) { result = -1; break; }

                // Check the status code (last point) against the record counter
                int lastPointFlag = ((statusCode & 0x80) != 0);
                int lastRecordFlag = ((i + 1) == recordCnt);
                if(lastPointFlag && !lastRecordFlag)
                {
                    logError("[IDTF] Last point flag set, record count mismatch: Record %u of %u, file pos 0x%08X", i, recordCnt, filePos);
                    result = -1;
                    break;
                }
                else if(!lastPointFlag && lastRecordFlag)
                {
                    logError("[IDTF] Last point flag not set on last record: File pos 0x%08X", filePos);
                }
            }
            if(result != 0) break;

            // Tell the output to push the frame
            if(cbFunc->pushFrame(cbContext)) { result = -1; break; }
        }
        else if(formatCode == 2)
        {
            //logInfo("Palette, filePos 0x%08X", formatCode, filePos);

            // Terminate on insane palettes
            if(recordCnt > 256)
            {
                logError("[IDTF] %s: Palettes shall not contain more than 256 colors", filename); 
                result = -1;
                break;
            }

            // Initialize palette table
            memset(customPalette, 0, sizeof(customPalette));

            // Loop through all color indices
            for(int i = 0; i < recordCnt; i++)
            {
                // Read color
                uint8_t r = (uint8_t)fgetc(fpIDTF);
                uint8_t g = (uint8_t)fgetc(fpIDTF);
                uint8_t b = (uint8_t)fgetc(fpIDTF);

                // Check for unexpected EOF
                if(feof(fpIDTF)) 
                { 
                    logError("[IDTF] Unexpected end of file: Record %u of %u", i, recordCnt); 
                    result = -1;
                    break;
                }

                // Set palette entry
                customPalette[i] = ILDACOLOR(r, g, b);
            }
            if(result != 0) break;

            // Set custom palette for the next sections
            currentPalette = customPalette;
        }
        else
        {
            logError("[IDTF] %s: formatCode = %d", filename, formatCode);
            result = -1;
            break;
        }
    }

    fclose(fpIDTF);

    return result;
}

