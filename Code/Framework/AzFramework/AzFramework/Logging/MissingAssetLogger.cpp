/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Logging/MissingAssetLogger.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/Logging/LogFile.h>

namespace AzFramework
{
    constexpr const char StartLogFileDelimiter[] = "------------------------ START LOG ------------------------";
    constexpr const char LogFileBaseName[] = "PakMissingAssets";
    constexpr int RolloverLength = 4 * 1024 * 1024;

    MissingAssetLogger::MissingAssetLogger()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            return;
        }

        const char* logDirectory = fileIO->GetAlias("@log@");

        if (!logDirectory)
        {
            //We will not log anything if the log alias is empty
            AZ_Warning("Log Component", logDirectory, "Please set the log alias first, before trying to log data");
            return;
        }

        m_logFile = AZStd::make_unique<LogFile>(logDirectory, LogFileBaseName, RolloverLength);
        m_logFile->AppendLog(AzFramework::LogFile::SEV_NORMAL, StartLogFileDelimiter);

        BusConnect();
    }

    MissingAssetLogger::~MissingAssetLogger()
    {
        BusDisconnect();
    }

    void MissingAssetLogger::FileMissing(const char* filePath)
    {
        if (m_logFile)
        {
            m_logFile->AppendLog(AzFramework::LogFile::SEV_WARNING, AZStd::string::format("Missing from bundle: %s", filePath).c_str());
        }
    }

}
