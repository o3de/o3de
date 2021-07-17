/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#import <Cocoa/Cocoa.h>

#include <QString>

QString operatingSystemVersionString()
{
    NSString* operatingSystemVersionString = [[NSProcessInfo processInfo] operatingSystemVersionString];
    return QString::fromNSString(operatingSystemVersionString);
}

QString graphicsCardName()
{
    QString name;

    // Get dictionary of all the PCI Devicces
    CFMutableDictionaryRef matchDict = IOServiceMatching("IOPCIDevice");

    // Create an iterator
    io_iterator_t iterator;

    if (IOServiceGetMatchingServices(kIOMasterPortDefault,matchDict,
                                     &iterator) == kIOReturnSuccess)
    {
        // Iterator for devices found
        io_registry_entry_t regEntry;

        while ((regEntry = IOIteratorNext(iterator))) {
            // Put this services object into a dictionary object.
            CFMutableDictionaryRef serviceDictionary;
            if (IORegistryEntryCreateCFProperties(regEntry,
                                                  &serviceDictionary,
                                                  kCFAllocatorDefault,
                                                  kNilOptions) != kIOReturnSuccess)
            {
                // Service dictionary creation failed.
                IOObjectRelease(regEntry);
                continue;
            }
            const void *GPUModel = CFDictionaryGetValue(serviceDictionary, @"model");

            if (GPUModel != nil) {
                if (CFGetTypeID(GPUModel) == CFDataGetTypeID()) {
                    // Create a string from the CFDataRef.
                    NSString *modelName = [[NSString alloc] initWithData:
                                           (NSData *)GPUModel encoding:NSASCIIStringEncoding];

                    name = QString::fromNSString(modelName);
                    [modelName release];
                }
            }
            // Release the dictionary
            CFRelease(serviceDictionary);
            // Release the serviceObject
            IOObjectRelease(regEntry);
        }
        // Release the iterator
        IOObjectRelease(iterator);
    }

    return name;
}
