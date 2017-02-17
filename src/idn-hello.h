// -------------------------------------------------------------------------------------------------
//  File idn-hello.h
// 
//  Copyright (c) 2013-2017 DexLogic, Dirk Apitz
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
//  07/2013 Dirk Apitz, created
//  06/2015 Dirk Apitz, Scan response: protocol version moved, name len changed
//  12/2015 Dirk Apitz, Service map
//  05/2016 Dirk Apitz, Generic data types
//  01/2017 Dirk Apitz, ServiceMap
//
//  Version: 2017-02-17
// -------------------------------------------------------------------------------------------------


#ifndef IDN_HELLO_H
#define IDN_HELLO_H


// Standard libraries
#include <stdint.h>


// -------------------------------------------------------------------------------------------------
//  Defines
// -------------------------------------------------------------------------------------------------

// Packet commands
#define IDNCMD_VOID                         0x00
#define IDNCMD_PING_REQUEST                 0x08
#define IDNCMD_PING_RESPONSE                0x09

#define IDNCMD_SCAN_REQUEST                 0x10
#define IDNCMD_SCAN_RESPONSE                0x11
#define IDNCMD_SERVICEMAP_REQUEST           0x12
#define IDNCMD_SERVICEMAP_RESPONSE          0x13

#define IDNCMD_SERVICE_PARAMETERS_REQUEST   0x20
#define IDNCMD_SERVICE_PARAMETERS_RESPONSE  0x21
#define IDNCMD_UNIT_PARAMETERS_REQUEST      0x22
#define IDNCMD_UNIT_PARAMETERS_RESPONSE     0x23
#define IDNCMD_LINK_PARAMETERS_REQUEST      0x28
#define IDNCMD_LINK_PARAMETERS_RESPONSE     0x29

#define IDNCMD_MESSAGE                      0x40


// -------------------------------------------------------------------------------------------------
//  Typedefs
// -------------------------------------------------------------------------------------------------

typedef struct
{
    uint8_t command;                            // The command code (IDNCMD_*)
    uint8_t flags;
    uint16_t sequence;                          // Sequence counter, must count up
    
} IDNHDR_PACKET;


typedef struct _IDNHDR_SCAN_RESPONSE
{
    uint8_t structSize;                         // Size of this struct.
    uint8_t protocolVersion;                    // 4 bit major (msb), 4 bit minor (lsb)
    uint16_t reserved;
    uint8_t unitID[16];
    uint8_t hostName[20];                       // Not terminated, shorter names padded with '\0'

} IDNHDR_SCAN_RESPONSE;


typedef struct _IDNHDR_SERVICEMAP_RESPONSE
{
    uint8_t structSize;                         // Size of this struct.
    uint8_t entrySize;                          // Size of an entry - sizeof(IDNHDR_SERVICEMAP_ENTRY)
    uint8_t relayEntryCount;                    // Number of relay entries
    uint8_t serviceEntryCount;                  // Number of service entries

    // Followed by the relay table (of relayEntryCount entries)
    // Followed by the service table (of serviceEntryCount entries)

} IDNHDR_SERVICEMAP_RESPONSE;


typedef struct _IDNHDR_SERVICEMAP_ENTRY
{
    uint8_t serviceID;                          // The ID of the service, 0 = Default/Relay
    uint8_t serviceType;                        // The type of the service, 0 = Relay
    uint8_t flags;                              // Status flags and options
    uint8_t relayNumber;                        // The relay the service belongs to, 0 = none/root
    uint8_t name[20];                           // Not terminated, shorter names padded with '\0'

} IDNHDR_SERVICEMAP_ENTRY;



// Icon
// ??? Properties: Wavelength



#endif

