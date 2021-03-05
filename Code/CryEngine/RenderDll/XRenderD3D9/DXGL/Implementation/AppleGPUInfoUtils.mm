/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "AppleGPUInfoUtils.h"


#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>

// this file needs to be sparate since CryEngine headers conflict with the
// Cocoa definitions

// this will be removed in the next version of OS X, but for now
// we can still use it.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

long GetVRAMForDisplay(const int dspNum)
{
    CGError                    err = CGDisplayNoErr;
    io_service_t              *dspPorts = NULL;
    CGDirectDisplayID    *displays = NULL;
    CGDisplayCount        dspCount = 0;
    CFTypeRef                 typeCode;
    uint32_t vram = 0;
    
    // How many active displays do we have?
    err = CGGetActiveDisplayList(0, NULL, &dspCount);
    
    if (err != kCGErrorSuccess)
        return -1;
    
    // If the display number is higher than the available display count, return
    if (dspCount < dspNum)
        return -1;
    
    // Allocate enough memory to hold all the display IDs we have
    displays = (CGDirectDisplayID*)calloc((size_t)dspCount, sizeof(CGDirectDisplayID));
   
    // Allocate memory for our service ports
    dspPorts = (io_service_t*)calloc((size_t)dspCount, sizeof(io_service_t));
    
    // Get the list of active displays
    err = CGGetActiveDisplayList(dspCount,
                                 displays,
                                 &dspCount);
    
    if (err == kCGErrorSuccess)
    {
        // Get the service port for the display
        dspPorts[dspNum] = CGDisplayIOServicePort(displays[dspNum]);
        // Ask IOKit for the VRAM size property
        typeCode = IORegistryEntryCreateCFProperty(dspPorts[dspNum],
                                                   CFSTR(kIOFBMemorySizeKey),
                                                   kCFAllocatorDefault,
                                                   kNilOptions);
        
        // Ensure we have valid data from IOKit
        if(typeCode && CFGetTypeID(typeCode) == CFNumberGetTypeID())
        {
            // If so, convert the CFNumber into a plain unsigned long
            CFNumberGetValue((CFNumberRef)typeCode, kCFNumberSInt32Type, &vram);
            if(typeCode)
                CFRelease(typeCode);
        }
    }
    
    free(displays);
    free(dspPorts);
    // Return the total number of displays we found
    return (long)vram;
}
#pragma clang diagnostic pop
