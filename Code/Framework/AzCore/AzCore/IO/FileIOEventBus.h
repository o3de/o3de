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
         * File IO interface. All events return true if we executed the
         * specific operation and no other code will be executed. If we return false
         * the normal code for the specific event will be executed.
         * IMPORTANT: We support multiple listeners with the idea that many systems can listen
         * for event. This interface allows to actually perform the operations, in such cases make
         * sure only one of the listeners provides this service (otherwise depending on registration
         * order service providers may change)
         * IMPORTANT: We don't provide any sync for the FileIOBus. We do that for a couple of reasons.
         * 1. If you will handle file IO youself or keeptrack of statistics you code will most likely already do that
         * 2. It is NOT safe to BusConnect/BusDisconnect while the FileIO is in use (this is why you should connect in advance)
         * otherwise if you provide service you can end up connecting in a middle of reads/writes/etc.
         */
        class FileIO
            : public AZ::EBusTraits
        {
        public:
            virtual ~FileIO() {}
            virtual bool OnOpen(SystemFile& file, const char* fileName, int mode, int platformFlags, bool& isFileOpened) = 0;
            virtual bool OnClose(SystemFile& file) = 0;
            virtual bool OnSeek(SystemFile& file, SystemFile::SizeType offset, SystemFile::SeekMode mode) = 0;
            virtual bool OnRead(SystemFile& file, SystemFile::SizeType byteSize, void* buffer, SystemFile::SizeType& numRead) = 0;
            virtual bool OnWrite(SystemFile& file, const void* buffer, SystemFile::SizeType byteSize, SystemFile::SizeType& numWritten) = 0;
        };

        typedef AZ::EBus<FileIO> FileIOBus;

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
