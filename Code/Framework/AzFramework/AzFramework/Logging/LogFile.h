/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZFRAMEWORK_LOGFILE_H
#define AZFRAMEWORK_LOGFILE_H

#include <AzCore/base.h>

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/parallel/mutex.h>

namespace AzFramework
{
    //! LogFile
    //! This class stores information related to the log file and is responsible for writing logs 
    class LogFile
    {
    public:
        AZ_CLASS_ALLOCATOR(LogFile, AZ::SystemAllocator)

        enum SeverityLevel
        {
            SEV_DEBUG,
            SEV_NORMAL,
            SEV_WARNING,
            SEV_ERROR,
            SEV_ASSERT,
            SEV_EXCEPTION,
        };

        //! initialize it this way in order to place files in that folder with additional files being rolled over into every rolloverLength bytes.
        LogFile(const char* baseDirectory, const char* fileName, AZ::u64 rollverLength, bool forceOverwriteFile = false);

        //! You can also initialize it as an absolute path, in which case it will not perform rollovers or path changes
        LogFile(const char* absolutePath, bool forceOverwriteFile = false);
        ~LogFile();

        void AppendLog(SeverityLevel severity, const char* category, const char* datasource);
        void AppendLog(SeverityLevel severity, const char* dataSource);
        void AppendLog(SeverityLevel severity, const char* dataSource, int dataLength, const char* category = NULL, int categoryLen = 0, uintptr_t threadId = 0, AZ::u64 time = 0);

        //! Only call FlushLog in cases of emergencies such as access violation termination.  Do not call it after appending.
        void FlushLog();

        //! If you flip the log file into Machine Readable = false, it will no longer delimit, and it will format it for human eyes instead
        //! This makes it much slower or ambiguous to parse with machines, but may be easier for humans to read.
        //! This is set to true by default, because the recommended way to peruse logs is to use a log viewer.
        void SetMachineReadable(bool newMode); 

    private:

        void Init(const char* baseDirectory, const char* fileName, AZ::u64 rolloverLength, bool forceOverwriteFile);
        void InitAbsolute(const char* absolutePath, bool forceOverwriteFile);
        void CheckSizeAndCycleFiles();
        AZStd::string CreateNewBackupFileName();

        AZ::IO::FileIOBase* m_fileIO;
        AZ::IO::HandleType m_fileHandle = AZ::IO::InvalidHandle;
        AZStd::string m_filePath;
        AZStd::string m_fileName;
        AZStd::string m_directoryName;
        AZ::u64 m_runtimeRolloverLength;
        AZStd::recursive_mutex m_logProtector;
        bool m_machineReadable = true; // if you switch

        bool m_reentryProtection = false;
    };
}

#endif //AZFRAMEWORK_LOGFILE_H
