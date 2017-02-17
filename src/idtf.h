// -------------------------------------------------------------------------------------------------
//  File idtf.h
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
//  06/2016 Dirk Apitz, scale, mirror
// -------------------------------------------------------------------------------------------------


#ifndef IDTF_H
#define IDTF_H


// Standard libraries
#include <stdint.h>


// -------------------------------------------------------------------------------------------------
//  Defines
// -------------------------------------------------------------------------------------------------

#define IDTFOPT_PALETTE_IDTF_DEFAULT    1           // Use the default palette as of IDTF rev. 11
#define IDTFOPT_PALETTE_ILDA_STANDARD   2           // Use the abandoned ILDA Standard Palette
#define IDTFOPT_PALETTE_MASK            0x000F

#define IDTFOPT_MIRROR_X                0x0100      // Mirror x axis
#define IDTFOPT_MIRROR_Y                0x0200      // Mirror y axis


// -------------------------------------------------------------------------------------------------
//  Typedefs
// -------------------------------------------------------------------------------------------------

typedef struct
{
    int (* openFrame)(void *context);
    int (* putSampleXYRGB)(void *context, int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
    int (* pushFrame)(void *context);

} IDTF_CALLBACK_FUNC;


// -------------------------------------------------------------------------------------------------
//  Prototypes
// -------------------------------------------------------------------------------------------------

int idtfRead(char *filename, float xyScale, unsigned options, IDTF_CALLBACK_FUNC *cbFunc, void *cbContext);


#endif

