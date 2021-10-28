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

    #include <windows.h>
    #include <tchar.h>

    #include "plt-windows.h"

#else

    #include <stdlib.h>
    #include <string.h>
    #include <arpa/inet.h>

    #include "plt-posix.h"

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
    unsigned char clientGroup;              // Client group to send on
    unsigned char serviceID;                // ServiceID to use
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
    uint32_t payloadLen;                    // Currently used length of the buffer

    // IDN-Hello related
    uint16_t sequence;                      // IDN-Hello sequence number (UDP packet tracking)

    // IDN-Stream related
    uint32_t sampleChunkHdrOffset;          // Offset of current sample chunk header 
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
        ctx->payloadLen = 0;
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
    printf("\n%u\n", (plt_getMonoTimeUS() - ctx->startTime) / 1000);
    binDump(packetHdr, packetLen);
*/

    if(sendto(ctx->fdSocket, (const char *)packetHdr, packetLen, 0, (struct sockaddr *)&ctx->serverSockAddr, sizeof(ctx->serverSockAddr)) < 0)
    {
        logError("sendto() failed (error: %d)", plt_sockGetLastError());
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

    // Sanity check (buffer already in use?)
    if(ctx->payloadLen != 0) return -1;

    // Make sure there is enough buffer
    if(ensureBufferCapacity(ctx, 0x4000)) return -1;

    // IDN-Hello packet header. Note: Sequence number populated on push
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    packetHdr->command = IDNCMD_RT_CNLMSG;
    packetHdr->flags = ctx->clientGroup;

    // ---------------------------------------------------------------------------------------------

    // IDN-Stream channel message header. Note: Remaining fields populated on push
    IDNHDR_CHANNEL_MESSAGE *channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)&packetHdr[1];
    uint16_t contentID = IDNFLG_CONTENTID_CHANNELMSG;
    
    // Insert channel config header every 200 ms
    unsigned now = plt_getMonoTimeUS();
    IDNHDR_SAMPLE_CHUNK *sampleChunkHdr = (IDNHDR_SAMPLE_CHUNK *)&channelMsgHdr[1];
    if((ctx->frameCnt == 0) || ((now - ctx->cfgTimestamp) > 200000))
    {
        // IDN-Stream channel configuration header
        IDNHDR_CHANNEL_CONFIG *channelConfigHdr = (IDNHDR_CHANNEL_CONFIG *)sampleChunkHdr;
        channelConfigHdr->wordCount = 4;
        channelConfigHdr->flags = IDNFLG_CHNCFG_ROUTING;
        channelConfigHdr->serviceID = ctx->serviceID;
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

    // Setup for sample chunk data positions
    ctx->sampleChunkHdrOffset = (uint8_t *)sampleChunkHdr - ctx->bufferPtr;
    ctx->payloadLen = (uint8_t *)&sampleChunkHdr[1] - ctx->bufferPtr;
    ctx->sampleCnt = 0;

    return 0;
}


int idnPutSampleXYRGB(void *context, int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Sanity check (sample chunk bracket open?)
    if(ctx->payloadLen == 0) return -1;

    // Make sure there is enough buffer.
    unsigned lenNeeded = ctx->payloadLen + ((1 + ctx->colorShift) * XYRGB_SAMPLE_SIZE);
    if(ensureBufferCapacity(ctx, lenNeeded)) return -1;


    // Note: With IDN, the first two points and the last two points of a frame have special 
    // meanings, The first point is the start point and shall be invisible (not part of the frame, 
    // not taken into account with duration calculations) and is used to move the draw cursor 
    // only. This is because a shape of n points has n-1 connecting segments (with associated time
    // and color). The shape is closed when last point and first point are equal and the shape is
    // continuous when first segment and last segment are continuous. Hidden lines or bends shall 
    // be inserted on the fly in case of differing start point and end point or discontinuity.


    // Get pointer to next sample
    uint8_t *p = &ctx->bufferPtr[ctx->payloadLen];

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

    // Update payload length to include the sample, update sample count
    ctx->payloadLen += XYRGB_SAMPLE_SIZE;
    ctx->sampleCnt++;

    return 0;
}


int idnPushFrameXYRGB(void *context)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Sanity check (sample chunk bracket open?)
    if(ctx->payloadLen == 0) return -1;
    if(ctx->sampleCnt < 2) { logError("[IDN] Invalid sample count %u", ctx->sampleCnt); return -1; }

    // ---------------------------------------------------------------------------------------------

    // Duplicate last position for color shift samples
    for(unsigned i = 0; i < ctx->colorShift; i++) 
    {
        // Get pointer to last position and next sample (already has color due to shift)
        uint16_t *src = (uint16_t *)&ctx->bufferPtr[ctx->payloadLen - XYRGB_SAMPLE_SIZE];
        uint16_t *dst = (uint16_t *)&ctx->bufferPtr[ctx->payloadLen];

        // Duplicate position samples (X/Y)
        *dst++ = *src++;
        *dst++ = *src++;

        // Update pointer to next sample, update sample count
        ctx->payloadLen += XYRGB_SAMPLE_SIZE;
        ctx->sampleCnt++;
    }

    // Sample chunk header: Calculate frame duration based on scan speed.
    // In case jitter-free option is set: Scan frames (starting from second) ony once.
    IDNHDR_SAMPLE_CHUNK *sampleChunkHdr = (IDNHDR_SAMPLE_CHUNK *)&ctx->bufferPtr[ctx->sampleChunkHdrOffset];
    uint32_t frameDuration = (((uint64_t)(ctx->sampleCnt - 1)) * 1000000ull) / (uint64_t)ctx->scanSpeed;
    uint8_t frameFlags = 0;
    if(ctx->jitterFreeFlag && ctx->frameCnt != 0) frameFlags |= IDNFLG_GRAPHIC_FRAME_ONCE;
    sampleChunkHdr->flagsDuration = htonl((frameFlags << 24) | frameDuration);

    // Wait between frames to match frame rate
    if(ctx->frameCnt != 0)
    {
        unsigned usWait = ctx->usFrameTime - (plt_getMonoTimeUS() - ctx->frameTimestamp);
        if((int)usWait > 0) plt_usleep(usWait);
    }
    ctx->frameCnt++;

    // ---------------------------------------------------------------------------------------------

    // Calculate header pointers, get message contentID (because of byte order)
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    IDNHDR_CHANNEL_MESSAGE *channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)&packetHdr[1];
    uint16_t contentID = ntohs(channelMsgHdr->contentID);

    // IDN channel message header: Set timestamp; Update internal timestamps.
    unsigned now = plt_getMonoTimeUS();
    channelMsgHdr->timestamp = htonl(now);
    ctx->frameTimestamp = now;
    if(contentID & IDNFLG_CONTENTID_CONFIG_LSTFRG) ctx->cfgTimestamp = now;

    // Message header: Calculate message length. Must not exceed 0xFF00 octets !!
    // Note: Pointer type uint8_t and substraction is defined as the difference of (array) elements.
    uint8_t *payloadLimit = &ctx->bufferPtr[ctx->payloadLen];
    unsigned msgLength = payloadLimit - (uint8_t *)channelMsgHdr;
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
            packetHdr->command = IDNCMD_RT_CNLMSG;
            packetHdr->flags = ctx->clientGroup;
            packetHdr->sequence = htons(ctx->sequence++);

            // Calculate remaining message length
            msgLength = payloadLimit - (uint8_t *)channelMsgHdr;
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
                if(idnSend(ctx, packetHdr, payloadLimit - (uint8_t *)packetHdr)) return -1;

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
        if(idnSend(ctx, packetHdr, payloadLimit - (uint8_t *)packetHdr)) return -1;
    }

    // Invalidate payload - cause error in case of invalid call order
    ctx->payloadLen = 0;

    return 0;
}


int idnSendVoid(void *context)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Sanity check (buffer already in use?)
    if(ctx->payloadLen != 0) return -1;

    // Make sure there is enough buffer
    if(ensureBufferCapacity(ctx, 0x1000)) return -1;

    // IDN-Hello packet header
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    packetHdr->command = IDNCMD_RT_CNLMSG;
    packetHdr->flags = ctx->clientGroup;
    packetHdr->sequence = htons(ctx->sequence++);

    // IDN-Stream channel message header
    IDNHDR_CHANNEL_MESSAGE *channelMsgHdr = (IDNHDR_CHANNEL_MESSAGE *)&packetHdr[1];
    uint16_t contentID = IDNFLG_CONTENTID_CHANNELMSG | IDNVAL_CNKTYPE_VOID;
    channelMsgHdr->contentID = htons(contentID);

    // Pointer to the end of the buffer for message length and packet length calculation
    uint8_t *payloadLimit = (uint8_t *)&channelMsgHdr[1];

    // Populate message header fields
    channelMsgHdr->totalSize = htons((unsigned short)(payloadLimit - (uint8_t *)channelMsgHdr));
    channelMsgHdr->timestamp = htonl(plt_getMonoTimeUS());

    // Send the packet
    if(idnSend(context, packetHdr, payloadLimit - (uint8_t *)packetHdr)) return -1;

    return 0;
}


int idnSendClose(void *context)
{
    IDNCONTEXT *ctx = (IDNCONTEXT *)context;

    // Sanity check (buffer already in use?)
    if(ctx->payloadLen != 0) return -1;

    // Make sure there is enough buffer
    if(ensureBufferCapacity(ctx, 0x1000)) return -1;

    // Close the channel: IDN-Hello packet header
    IDNHDR_PACKET *packetHdr = (IDNHDR_PACKET *)ctx->bufferPtr;
    packetHdr->command = IDNCMD_RT_CNLMSG;
    packetHdr->flags = ctx->clientGroup;
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
    uint8_t *payloadLimit = (uint8_t *)&channelConfigHdr[1];

    // Populate message header fields
    channelMsgHdr->totalSize = htons((unsigned short)(payloadLimit - (uint8_t *)channelMsgHdr));
    channelMsgHdr->timestamp = htonl(plt_getMonoTimeUS());

    // Send the packet
    if(idnSend(context, packetHdr, payloadLimit - (uint8_t *)packetHdr)) return -1;

    // ---------------------------------------------------------------------------------------------

    // Close the connection/session: IDN-Hello packet header
    packetHdr->command = IDNCMD_RT_CNLMSG_CLOSE;
    packetHdr->flags = ctx->clientGroup;
    packetHdr->sequence = htons(ctx->sequence++);

    // Send the packet (gracefully close session)
    if(idnSend(context, packetHdr, sizeof(IDNHDR_PACKET))) return -1;

    return 0;
}


// -------------------------------------------------------------------------------------------------
//  Entry point
// -------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    int usageFlag = 0;
    in_addr_t helloServerAddr = 0;
    unsigned char clientGroup = 0;
    unsigned char serviceID = 0;
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
        else if(!strcmp(argv[i], "-cg"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            int param = atoi(argv[i]);
            if((param < 0) || (param >= 16)) { usageFlag = 1; break; }
            else clientGroup = atoi(argv[i]);
        }
        else if(!strcmp(argv[i], "-sid"))
        {
            if(++i >= argc) { usageFlag = 1; break; }
            int param = atoi(argv[i]);
            if((param < 0) || (param >= 256)) { usageFlag = 1; break; }
            else serviceID = param;
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
        printf("  -hs      ipAddress   IP address of the IDN-Hello server.\n");
        printf("  -cg      clientGroup The client group (0..15, default = 0).\n");
        printf("  -idtf    filename    Name of the IDTF (ILDA Image Data Transfer Format) file.\n");
        printf("  -hold    time        Time in seconds to display single-frame files\n");
        printf("  -fr      frameRate   Number of frames per second. (default: 30)\n");
        printf("  -jf                  Jitter-Free (scan frames only once to match frame rate)\n");
        printf("  -pps     scanSpeed   Number of points/samples per second. (default: 30000)\n");
        printf("  -sft     colorShift  Number of points, the color is shifted (default: 0)\n");
        printf("  -scale   factor      Factor by which to scale the IDTF file (default: 1.0)\n");
        printf("  -mx                  Mirror x axis\n");
        printf("  -my                  Mirror y axis\n");
        printf("  -def-pal             Use the default palette as of IDTF rev. 11 (default).\n");
        printf("  -std-pal             Use the abandoned ILDA Standard Palette.\n");
        printf("\n");

        return 0;
    }


    // -------------------------------------------------------------------------

    printf("Connecting to IDN-Hello server at %s\n", inet_ntoa(*(struct in_addr *)&helloServerAddr));
    printf("Press Ctrl-C to stop\n");

    // Initialize driver function context
    IDNCONTEXT ctx = { 0 };
    ctx.fdSocket = -1;
    ctx.serverSockAddr.sin_family = AF_INET;
    ctx.serverSockAddr.sin_port = htons(IDNVAL_HELLO_UDP_PORT);
    ctx.serverSockAddr.sin_addr.s_addr = helloServerAddr;
    ctx.clientGroup = clientGroup;
    ctx.serviceID = serviceID;
    ctx.usFrameTime = 1000000 / frameRate;
    ctx.jitterFreeFlag = jitterFreeFlag;
    ctx.scanSpeed = scanSpeed;
    ctx.colorShift = colorShift;
    
    do
    {
        // Validate monotonic time reference
        if(plt_validateMonoTime() != 0)
        {
            logError("Monotonic time init failed");
            return -1;
        }

        // Initialize platform sockets
        int rcStartup = plt_sockStartup();
        if(rcStartup)
        {
            logError("Socket startup failed. error = %d", rcStartup);
            break;
        }

        // Open UDP socket
        ctx.fdSocket = plt_sockOpen(AF_INET, SOCK_DGRAM, 0);
        if(ctx.fdSocket < 0)
        {
            logError("socket() faile (error: %d)", plt_sockGetLastError());
            break;
        }

        // Initialize IDTF reader callback function table
        IDTF_CALLBACK_FUNC cbFunc = { 0 };
        cbFunc.openFrame = idnOpenFrameXYRGB;
        cbFunc.putSampleXYRGB = idnPutSampleXYRGB;
        cbFunc.pushFrame = idnPushFrameXYRGB;
        
        // Run IDTF reader
        ctx.startTime = plt_getMonoTimeUS();
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

    // Platform sockets cleanup
    if(plt_sockCleanup()) logError("Socket cleanup failed (error: %d)", plt_sockGetLastError());

    return 0;
}

