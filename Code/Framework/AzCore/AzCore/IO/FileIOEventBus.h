/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_SYSTEM_FILE_BUS_H
#define AZCORE_SYSTEM_FILE_BUS_H

#include <AzCore/IO/SystemFile.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace IO
    {
        /**
         * Interface for handling file io events. All events are syncronized
         */
        class FileIOEvents
            : public AZ::EBusTraits
        {
        public:
            virtual ~FileIOEvents() {}

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            //TODO rbbaklov or zolniery look into why a recursive lock was not needed previously
            typedef AZStd::recursive_mutex MutexType;  //< make sure all file events are thread safe as they will called from many threads
            //////////////////////////////////////////////////////////////////////////

            /**
             * You will either have a file (SystemFile) pointer or fileName pointer to the file name.
             * \param fileName is provided when there is NO SystemFile object (when you call static functions).
             */
            virtual void OnError(const SystemFile* file, const char* fileName, int errorCode) = 0;
        };

        typedef AZ::EBus<FileIOEvents> FileIOEventBus;
    }
}
#endif // AZCORE_SYSTEM_FILE_BUS_H
#pragma once
