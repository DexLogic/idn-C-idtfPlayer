// -------------------------------------------------------------------------------------------------
//  File main.c
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
//  06/2016 Dirk Apitz, color shift, fragmented frames, scale, mirror
//  08/2016 Theo Dari / Dirk Apitz, Windows port
// -------------------------------------------------------------------------------------------------

// Standard libraries
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>


// Platform includes
#if defined(_WIN32) || defined(WIN32)
#include "plt-windows.h"
#else
#include "plt-posix.h"
#endif



// Platform
#if defined(_WIN32) || defined(WIN32)

#include <windows.h>
#include <tchar.h>

typedef unsigned long in_addr_t;

#else

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#endif


// Project headers
#include "idn-hello.h"
#include "idn-stream.h"
#include "idtf.h"


// -------------------------------------------------------------------------------------------------
//  Defines
// -------------------------------------------------------------------------------------------------

#define DEFAULT_FRAMERATE               30
#define DEFAULT_SCANSPEED               30000

#define MAX_IDN_MESSAGE_LEN             0xFF00      // IDN-Message maximum length (due to lower layer transport)
//#define MAX_IDN_MESSAGE_LEN             0x0800      // Message len for fragmentation tests

#define XYRGB_SAMPLE_SIZE               7


// -------------------------------------------------------------------------------------------------
//  Typedefs
// -------------------------------------------------------------------------------------------------

typedef struct
{
    int fdSocket;                           // Socket file descriptor
    struct sockaddr_in serverSockAddr;      // Target server address
    unsigned usFrameTime;                   // Time for one frame in microseconds (1000000/frameRate)
    int jitterFreeFlag;                     // Scan frames only once to exactly match frame rate
    unsigned scanSpeed;                     // Scan speed in samples per second
    unsigned colorShift;                    // Color shift in points/samples

    unsigned bufferLen;                     // Length of work buffer
    uint8_t *bufferPtr;                     // Pointer to work buffer

    uint32_t startTime;                     // System time at stream start (log reference)
    uint32_t frameCnt;                      // Number of sent frames
    uint32_t frameTimestamp;                // Timestamp of the last frame
    uint32_t cfgTimestamp;                  // Timestamp of the last channel configuration

    // Buffer related
    uint8_t *payload;                       // Pointer to the end of the buffer

    // IDN-Hello related
    uint16_t sequence;                      // IDN-Hello sequence number (UDP packet tracking)

    // IDN-Stream related
    IDNHDR_SAMPLE_CHUNK *sampleChunkHdr;    // Current sample chunk header
    uint32_t sampleCnt;                     // Current number of samples

} IDNCONTEXT;


// -------------------------------------------------------------------------------------------------
//  Tools
// -------------------------------------------------------------------------------------------------

void logError(const char *fmt, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, fmt);

    //printf("\x1B[1;31m");
    vprintf(fmt, arg_ptr);
    //printf("\x1B[0m");
    printf("\n");
    fflush(stdout);
}


void logInfo(const char *fmt, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, fmt);

    vprintf(fmt, arg_ptr);
    printf("\n");
    fflush(stdout);
}


static int ensureBufferCapacity(IDNCONTEXT *ctx, unsigned minLen)
{
    // Check for buffer enlargement
    if(ctx->bufferLen < minLen)
    {
        if(ctx->bufferLen == 0) ctx->bufferLen = minLen;
        else while(ctx->bufferLen < minLen) ctx->bufferLen *= 2;

        ctx->bufferPtr = (uint8_t *)realloc(ctx->bufferPtr, ctx->bufferLen);
    }

    // Check buffer pointer
    if(ctx->bufferPtr == (uint8_t *)0) 
    { 
        logError("[IDN] Insufficient buffer memory"); 
        ctx->payload = (uint8_t *)0; 
        return -1; 
    }

    return 0;
}


static char int2Hex(unsigned i)
{
    i &= 0xf;
    return (char)((i > 9) ? ((i - 10) + 'A') : (i + '0'));
}


void binDump(void *buffer, unsigned length)
{
    if(!length || !buffer) return;

    char send[80];
    char *dst1 = 0, *dst2 = 0;
    char *src = (char *)buffer;
    unsigned k = 0;

    printf("dump buffer %08X; %d Bytes\n", (uint32_t)(uintptr_t)buffer, length);

    while(k < length)
    {
        if(!dst1)
        {
            memset(send, ' ', 80);
            send[79] = 0;

            send[0] = int2Hex((k >> 12) & 0x0f);
            send[1] = int2Hex((k >> 8) & 0x0f);
            send[2] = int2Hex((k >> 4) & 0x0f);
            send[3] = int2Hex(k & 0x0f);
            dst1 = &send[5];
            dst2 = &send[57];
        }

        unsigned char c = *src++;
        *dst1++ = int2Hex(c >> 4);
        *dst1++ = int2Hex(c);
        *dst1++ = ' ';
        if((k % 16) == 7)
        {*dst1++ = ' '; *dst1++ = ' ';}
        if(c < 0x20) c = '.';
        if(c > 0x7F) c = '.';
        *dst2++ = c;

        if((k % 16) == 15)
        {
            *dst2++ = 0;
            printf("%s\n", send);
            dst1 = 0;
            dst2 = 0;
        }
        k++;
    }

    if(k % 16) printf("%s\n", send);

    fflush(stdout);
}


static int idnSend(void *context, IDNHDR_PACKET *packetHdr, unsigned packetLen)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

/*
    printf("\n%u\n", (plt_getSystemTimeUS() - ctx->startTime) / 1000);
    binDump(packetHdr, packetLen);
*/

    if(sendto(ctx->fdSocket, (const char *)packetHdr, packetLen, 0, (struct sockaddr *)&ctx->serverSockAddr, sizeof(ctx->serverSockAddr)) < 0)
    {
        logError("sendto() error %d", plt_getLastError());
        return -1;
    }

    return 0;
}


// -------------------------------------------------------------------------------------------------
//  IDN
// -------------------------------------------------------------------------------------------------

int idnOpenFrameXYRGB(void *context)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Make sure there is enough buffer
    if(ensureBufferCapacity(ctx, 0x4000)) return -1;

    // IDN-Hello packet header. Note: Sequence number populated on push
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    packetHdr->command = IDNCMD_MESSAGE;
    packetHdr->flags = 0;

    // ---------------------------------------------------------------------------------------------

    // IDN-Stream channel message header. Note: Remaining fields populated on push
    IDNHDR_CHANNEL_MESSAGE *channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)&packetHdr[1];
    uint16_t contentID = IDNFLG_CONTENTID_CHANNELMSG;
    
    // Insert channel config header every 200 ms
    unsigned now = plt_getSystemTimeUS();
    IDNHDR_SAMPLE_CHUNK *sampleChunkHdr = (IDNHDR_SAMPLE_CHUNK *)&channelMsgHdr[1];
    if((ctx->frameCnt == 0) || ((now - ctx->cfgTimestamp) > 200000))
    {
        // IDN-Stream channel configuration header
        IDNHDR_CHANNEL_CONFIG *channelConfigHdr = (IDNHDR_CHANNEL_CONFIG *)sampleChunkHdr;
        channelConfigHdr->wordCount = 4;
        channelConfigHdr->flags = IDNFLG_CHNCFG_ROUTING;
        channelConfigHdr->serviceID = 0;
        channelConfigHdr->serviceMode = IDNVAL_SMOD_LPGRF_DISCRETE;

        // Standard IDTF-to-IDN descriptors
        uint16_t *descriptors = (uint16_t *)&channelConfigHdr[1];
        descriptors[0] = htons(0x4200);     // X
        descriptors[1] = htons(0x4010);     // 16 bit precision
        descriptors[2] = htons(0x4210);     // Y
        descriptors[3] = htons(0x4010);     // 16 bit precision
        descriptors[4] = htons(0x527E);     // Red, 638 nm
        descriptors[5] = htons(0x5214);     // Green, 532 nm
        descriptors[6] = htons(0x51CC);     // Blue, 460 nm
        descriptors[7] = htons(0x0000);     // Void for alignment

        // Move sample chunk start and set flag in contentID field
        sampleChunkHdr = (IDNHDR_SAMPLE_CHUNK *)&descriptors[8];
        contentID |= IDNFLG_CONTENTID_CONFIG_LSTFRG;
    }
    channelMsgHdr->contentID = htons(contentID);

    // ---------------------------------------------------------------------------------------------

    // Chunk data pointer setup
    ctx->sampleChunkHdr = sampleChunkHdr;
    ctx->payload = (uint8_t *)&sampleChunkHdr[1];
    ctx->sampleCnt = 0;

    return 0;
}


int idnPutSampleXYRGB(void *context, int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Sanity check
    if(ctx->payload == (uint8_t *)0) return -1;

    // Make sure there is enough buffer
    unsigned lenUsed = ctx->payload - ctx->bufferPtr;
    unsigned lenNeeded = lenUsed + ((1 + ctx->colorShift) * XYRGB_SAMPLE_SIZE);
    if(ensureBufferCapacity(ctx, lenNeeded)) return -1;


    // Note: With IDN, the first two points and the last two points of a frame have special 
    // meanings, The first point is the start point and shall be invisible (not part of the frame, 
    // not taken into account with duration calculations) and is used to move the draw cursor 
    // only. This is because a shape of n points has n-1 connecting segments (with associated time
    // and color). The shape is closed when last point and first point are equal and the shape is
    // continuous when first segment and last segment are continuous. Hidden lines or bends shall 
    // be inserted on the fly in case of differing start point and end point or discontinuity.


    // Get pointer to next sample
    uint8_t *p = ctx->payload;

    // Store galvo sample bytes
    *p++ = (uint8_t)(x >> 8);
    *p++ = (uint8_t)x;
    *p++ = (uint8_t)(y >> 8);
    *p++ = (uint8_t)y;

    // Check for color shift init
    if(ctx->sampleCnt == 0) 
    {
        // Color shift samples
        for(unsigned i = 0; i < ctx->colorShift; i++) 
        {
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p += XYRGB_SAMPLE_SIZE;
        }
    }
    else
    {
        // Other samples - just move the pointer
        p += XYRGB_SAMPLE_SIZE * ctx->colorShift;
    }

    // Store color sample bytes
    *p++ = r;
    *p++ = g;
    *p++ = b;

    // Update pointer to next sample, update sample count
    ctx->payload += XYRGB_SAMPLE_SIZE;
    ctx->sampleCnt++;

    return 0;
}


int idnPushFrameXYRGB(void *context)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Sanity check
    if(ctx->payload == (uint8_t *)0) return -1;
    if(ctx->sampleCnt < 2) { logError("[IDN] Invalid sample count %u", ctx->sampleCnt); return -1; }

    // ---------------------------------------------------------------------------------------------

    // Duplicate last position for color shift samples
    for(unsigned i = 0; i < ctx->colorShift; i++) 
    {
        // Get pointer to last position and next sample (already has color due to shift)
        uint16_t *src = (uint16_t *)(ctx->payload - XYRGB_SAMPLE_SIZE);
        uint16_t *dst = (uint16_t *)ctx->payload;

        // Duplicate position
        *dst++ = *src++;
        *dst++ = *src++;

        // Update pointer to next sample, update sample count
        ctx->payload += XYRGB_SAMPLE_SIZE;
        ctx->sampleCnt++;
    }

    // Sample chunk header: Calculate frame duration based on scan speed.
    // In case jitter-free option is set: Scan frames 2.. ony once.
    uint32_t frameDuration = (((uint64_t)(ctx->sampleCnt - 1)) * 1000000ull) / (uint64_t)ctx->scanSpeed;
    uint8_t frameFlags = 0;
    if(ctx->jitterFreeFlag && ctx->frameCnt != 0) frameFlags |= IDNFLG_GRAPHIC_FRAME_ONCE;
    ctx->sampleChunkHdr->flagsDuration = htonl((frameFlags << 24) | frameDuration);

    // Wait between frames to match frame rate
    if(ctx->frameCnt != 0)
    {
        unsigned usWait = ctx->usFrameTime - (plt_getSystemTimeUS() - ctx->frameTimestamp);
        if((int)usWait > 0) plt_usleep(usWait);
    }
    ctx->frameCnt++;

    // ---------------------------------------------------------------------------------------------

    // Calculate header pointers, get message contentID (because of byte order)
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    IDNHDR_CHANNEL_MESSAGE *channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)&packetHdr[1];
    uint16_t contentID = ntohs(channelMsgHdr->contentID);

    // IDN channel message header: Set timestamp; Update internal timestamps.
    unsigned now = plt_getSystemTimeUS();
    channelMsgHdr->timestamp = htonl(now);
    ctx->frameTimestamp = now;
    if(contentID & IDNFLG_CONTENTID_CONFIG_LSTFRG) ctx->cfgTimestamp = now;

    // Message header: Calculate message length. Must not exceed 0xFF00 octets !!
    unsigned msgLength = ctx->payload - (uint8_t *)channelMsgHdr;
    if(msgLength > MAX_IDN_MESSAGE_LEN)
    {
        // Fragmented frame (split across multiple messages), set message length and chunk type
        channelMsgHdr->totalSize = htons(MAX_IDN_MESSAGE_LEN);
        channelMsgHdr->contentID = htons(contentID | IDNVAL_CNKTYPE_LPGRF_FRAME_FIRST);
        uint8_t *splitPtr = (uint8_t *)channelMsgHdr + MAX_IDN_MESSAGE_LEN;

        // Set IDN-Hello sequence number (used on UDP for lost packet tracking)
        packetHdr->sequence = htons(ctx->sequence++);

        // Send the packet
        if(idnSend(ctx, packetHdr, splitPtr - (uint8_t *)packetHdr)) return -1;

        // Delete config flag (in case set - not config headers in fragments), set sequel fragment chunk type
        contentID &= ~IDNFLG_CONTENTID_CONFIG_LSTFRG;
        contentID |= IDNVAL_CNKTYPE_LPGRF_FRAME_SEQUEL;

        // Send remaining fragments
        while(1)
        {
            // Allocate message header (overwrite previous packet data), fragment number shared with timestamp
            channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)(splitPtr - sizeof(IDNHDR_CHANNEL_MESSAGE));
            channelMsgHdr->timestamp = htonl(++now);

            // Allocate and populate packet header
            packetHdr = (IDNHDR_PACKET *)((uint8_t *)channelMsgHdr - sizeof(IDNHDR_PACKET));
            packetHdr->command = IDNCMD_MESSAGE;
            packetHdr->flags = 0;
            packetHdr->sequence = htons(ctx->sequence++);

            // Calculate remaining message length
            msgLength = ctx->payload - (uint8_t *)channelMsgHdr;
            if(msgLength > MAX_IDN_MESSAGE_LEN)
            {
                // Middle sequel fragment
                channelMsgHdr->totalSize = htons(MAX_IDN_MESSAGE_LEN);
                channelMsgHdr->contentID = htons(contentID);
                splitPtr = (uint8_t *)channelMsgHdr + MAX_IDN_MESSAGE_LEN;

                // Send the packet
                if(idnSend(ctx, packetHdr, splitPtr - (uint8_t *)packetHdr)) return -1;
            }
            else
            {
                // Last sequel fragment, set last fragment flag
                channelMsgHdr->totalSize = htons((unsigned short)msgLength);
                channelMsgHdr->contentID = htons(contentID | IDNFLG_CONTENTID_CONFIG_LSTFRG);

                // Send the packet
                if(idnSend(ctx, packetHdr, ctx->payload - (uint8_t *)packetHdr)) return -1;

                // Done sending the packet
                break;
            }
        }
    }
    else
    {
        // Regular frame (single message), set message length and chunk type
        channelMsgHdr->totalSize = htons((unsigned short)msgLength);
        channelMsgHdr->contentID = htons(contentID | IDNVAL_CNKTYPE_LPGRF_FRAME);

        // Set IDN-Hello sequence number (used on UDP for lost packet tracking)
        packetHdr->sequence = htons(ctx->sequence++);

        // Send the packet
        if(idnSend(ctx, packetHdr, ctx->payload - (uint8_t *)packetHdr)) return -1;
    }

    // Invalidate payload - cause error in case of invalid call order
    ctx->payload = (uint8_t *)0;

    return 0;
}


int idnSendVoid(void *context)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Make sure there is enough buffer
    if(ensureBufferCapacity(ctx, 0x1000)) return -1;

    // IDN-Hello packet header
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    packetHdr->command = IDNCMD_MESSAGE;
    packetHdr->flags = 0;
    packetHdr->sequence = htons(ctx->sequence++);

    // IDN-Stream channel message header
    IDNHDR_CHANNEL_MESSAGE *channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)&packetHdr[1];
    uint16_t contentID = IDNFLG_CONTENTID_CHANNELMSG | IDNVAL_CNKTYPE_VOID;
    channelMsgHdr->contentID = htons(contentID);

    // Pointer to the end of the buffer for message length and packet length calculation
    ctx->payload = (uint8_t *)&channelMsgHdr[1];

    // Populate message header fields
    channelMsgHdr->totalSize = htons((unsigned short)(ctx->payload - (uint8_t *)channelMsgHdr));
    channelMsgHdr->timestamp = htonl(plt_getSystemTimeUS());

    // Send the packet
    if(idnSend(context, packetHdr, ctx->payload - (uint8_t *)packetHdr)) return -1;

    return 0;
}


int idnSendClose(void *context)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Make sure there is enough buffer
    if(ensureBufferCapacity(ctx, 0x1000)) return -1;

    // IDN-Hello packet header
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    packetHdr->command = IDNCMD_MESSAGE;
    packetHdr->flags = 0;
    packetHdr->sequence = htons(ctx->sequence++);

    // IDN-Stream channel message header
    IDNHDR_CHANNEL_MESSAGE *channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)&packetHdr[1];
    uint16_t contentID = IDNFLG_CONTENTID_CHANNELMSG | IDNFLG_CONTENTID_CONFIG_LSTFRG | IDNVAL_CNKTYPE_VOID;
    channelMsgHdr->contentID = htons(contentID);

    // IDN-Stream channel config header (close channel)
    IDNHDR_CHANNEL_CONFIG *channelConfigHdr = (IDNHDR_CHANNEL_CONFIG *)&channelMsgHdr[1];
    channelConfigHdr->wordCount = 0;
    channelConfigHdr->flags = IDNFLG_CHNCFG_CLOSE;
    channelConfigHdr->serviceID = 0;
    channelConfigHdr->serviceMode = 0;

    // Pointer to the end of the buffer for message length and packet length calculation
    ctx->payload = (uint8_t *)&channelConfigHdr[1];

    // Populate message header fields
    channelMsgHdr->totalSize = htons((unsigned short)(ctx->payload - (uint8_t *)channelMsgHdr));
    channelMsgHdr->timestamp = htonl(plt_getSystemTimeUS());

    // Send the packet
    if(idnSend(context, packetHdr, ctx->payload - (uint8_t *)packetHdr)) return -1;

    return 0;
}


// -------------------------------------------------------------------------------------------------
//  Entry point
// -------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    int usageFlag = 0;
    in_addr_t helloServerAddr = 0;
    char *idtfFilename = 0;
    unsigned holdTime = 5;
    unsigned frameRate = DEFAULT_FRAMERATE;
    int jitterFreeFlag = 0;
    unsigned scanSpeed = DEFAULT_SCANSPEED;
    unsigned colorShift = 0;
    float xyScale = 1.0;
    unsigned options = 0;


    for(int i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-hs"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            helloServerAddr = inet_addr(argv[i]);
        }
        else if(!strcmp(argv[i], "-idtf"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            idtfFilename = argv[i];
        }
        else if(!strcmp(argv[i], "-hold"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            int param = atoi(argv[i]);
            if(param > 0) holdTime = param;
        }
        else if(!strcmp(argv[i], "-fr"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            frameRate = atoi(argv[i]);
        }
        else if(!strcmp(argv[i], "-jf"))
        {
            jitterFreeFlag = 1;
        }
        else if(!strcmp(argv[i], "-pps"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            scanSpeed = atoi(argv[i]);
        }
        else if(!strcmp(argv[i], "-sft"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            colorShift = atoi(argv[i]);
        }
        else if(!strcmp(argv[i], "-scale"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            xyScale = (float)atof(argv[i]);
        }
        else if(!strcmp(argv[i], "-mx"))
        {
            options |= IDTFOPT_MIRROR_X;
        }
        else if(!strcmp(argv[i], "-my"))
        {
            options |= IDTFOPT_MIRROR_Y;
        }
        else if(!strcmp(argv[i], "-def-pal"))
        {
            options = (options & ~IDTFOPT_PALETTE_MASK) | IDTFOPT_PALETTE_IDTF_DEFAULT;
        }
        else if(!strcmp(argv[i], "-std-pal"))
        {
            options = (options & ~IDTFOPT_PALETTE_MASK) | IDTFOPT_PALETTE_ILDA_STANDARD;
        }
        else
        {
            usageFlag = 1;
            break;
        }
    }

    if(usageFlag || !helloServerAddr || !idtfFilename || (frameRate < 5))
    {
        printf("\n");
        printf("USAGE: idtfPlayer { Options } \n\n");
        printf("Options:\n");
        printf("  -hs      ipAddress  IP address of the IDN-Hello server.\n");
        printf("  -idtf    filename   Name of the IDTF (ILDA Image Data Transfer Format) file.\n");
        printf("  -hold    time       Time in seconds to display single-frame files\n");
        printf("  -fr      frameRate  Number of frames per second. (default: 30)\n");
        printf("  -jf                 Jitter-Free (scan frames only once to match frame rate)\n");
        printf("  -pps     scanSpeed  Number of points/samples per second. (default: 30000)\n");
        printf("  -sft     colorShift Number of points, the color is shifted (default: 0)\n");
        printf("  -scale   factor     Factor by which to scale the IDTF file (default: 1.0)\n");
        printf("  -mx                 Mirror x axis\n");
        printf("  -my                 Mirror y axis\n");
        printf("  -def-pal            Use the default palette as of IDTF rev. 11 (default).\n");
        printf("  -std-pal            Use the abandoned ILDA Standard Palette.\n");
        printf("\n");

        return 0;
    }


    // -------------------------------------------------------------------------

    // Initialize driver function context
    IDNCONTEXT ctx = { 0 };
    ctx.serverSockAddr.sin_family      = AF_INET;
    ctx.serverSockAddr.sin_port        = htons(IDNVAL_HELLO_UDP_PORT);
    ctx.serverSockAddr.sin_addr.s_addr = helloServerAddr;
    ctx.usFrameTime = 1000000 / frameRate;
    ctx.jitterFreeFlag = jitterFreeFlag;
    ctx.scanSpeed = scanSpeed;
    ctx.colorShift = colorShift;
    ctx.startTime = plt_getSystemTimeUS();

    do
    {
        printf("Connecting to IDN-Hello server at %s\n", inet_ntoa(*(struct in_addr *)&helloServerAddr));
        printf("Press Ctrl-C to stop\n");

        // Initialize platform specifics
        if(plt_initialize() != 0) break;

        // Open UDP socket
        ctx.fdSocket = plt_sockOpen(AF_INET, SOCK_DGRAM, 0);
        if(ctx.fdSocket < 0)
        {
            logError("socket() error %d", plt_getLastError());
            break;
        }

        // Initialize IDTF reader callback function table
        IDTF_CALLBACK_FUNC cbFunc = { 0 };
        cbFunc.openFrame = idnOpenFrameXYRGB;
        cbFunc.putSampleXYRGB = idnPutSampleXYRGB;
        cbFunc.pushFrame = idnPushFrameXYRGB;
        
        // Run IDTF reader
        if(idtfRead(idtfFilename, xyScale, options, &cbFunc, &ctx)) break;

        // Check for single frame IDTF file.(wait for the passed hold time)
        if(ctx.frameCnt == 1) 
        {
            // Wait
            for(unsigned i = 0; i < holdTime * 10; i++) 
            {
                plt_usleep(100000);
                idnSendVoid(&ctx);
            }
        }

        // Close the IDN channel
        idnSendClose(&ctx);
    }
    while(0);

    // Free buffer memory
    if(ctx.bufferPtr) free(ctx.bufferPtr);

    // Close socket
    if(ctx.fdSocket >= 0) plt_sockClose(ctx.fdSocket);

    return 0;
}

