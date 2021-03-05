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
