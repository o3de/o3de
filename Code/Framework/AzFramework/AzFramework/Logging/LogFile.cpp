/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/AzFramework_Traits_Platform.h>
#include "LogFile.h"
#if AZ_TRAIT_AZFRAMEWORK_USE_C11_LOCALTIME_S
#include <direct.h>
#include <time.h>
#endif
#include <AzCore/std/time.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/IO/SystemFile.h> //for AZ_MAX_PATH_LEN
#include <AzCore/std/functional.h> // for function<> in the find files callback.

namespace LogFileInternal
{
    // a simple class that switches a bool back to false when leaving scope.
    // it does not protect against threading issues or anything, its only purpose is in case
    // an error occurs during logging, to not re-enter the "add log" function 
    class ScopedReentrancyProtector
    {
    public:
        ScopedReentrancyProtector(bool &target)
        {
            target = true;
            m_targetVariable = &target;
        }

        ~ScopedReentrancyProtector()
        {
            if (m_targetVariable)
            {
                *m_targetVariable = false;
            }
        }

    private:
        bool* m_targetVariable = nullptr;
    };
};

namespace AzFramework
{
    using namespace LogFileInternal;

    namespace
    {
        const char* suffix = ".log";
        const int maxLogFiles = 5;

        void TimeNowAsString(char* buffer, size_t len)
        {
            time_t rawtime;
            struct tm timeinfo;

            time(&rawtime);
#if AZ_TRAIT_AZFRAMEWORK_USE_C11_LOCALTIME_S
            localtime_s(&timeinfo, &rawtime);
#elif AZ_TRAIT_AZFRAMEWORK_USE_POSIX_LOCALTIME_R
            localtime_r(&rawtime, &timeinfo);
#endif
            // encode as ISO-8601 so that its standard... which is like YYYY-MM-DDTHH-mm-ssTZD e.g., 1997-07-16T19:20:30+01:00
            strftime(buffer, len, "%Y-%m-%dT%H:%M:%S", &timeinfo);
        }
    }

    LogFile::LogFile(const char* baseDirectory, const char* fileName, AZ::u64 rolloverLength, bool forceOverwriteFile /* = false */)
        : m_fileHandle(AZ::IO::InvalidHandle)
    {
        Init(baseDirectory, fileName, rolloverLength, forceOverwriteFile);
    }

    LogFile::LogFile(const char* absoluteFilePath, bool forceOverwriteFile /* = false */)
        : m_fileHandle(AZ::IO::InvalidHandle)
    {
        InitAbsolute(absoluteFilePath, forceOverwriteFile);
    }

    LogFile::~LogFile()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> locker(m_logProtector);
        if (m_fileIO && m_fileHandle)
        {
            m_fileIO->Close(m_fileHandle);
        }
    }

    void LogFile::SetMachineReadable(bool newMode)
    {
        m_machineReadable = newMode;
    }


    void LogFile::InitAbsolute(const char* absolutePath, bool forceOverwriteFile)
    {
        m_fileIO = AZ::IO::FileIOBase::GetInstance();

        if ((!m_fileIO) || (!absolutePath) || (absolutePath[0] == '\0'))
        {
            return;
        }
        
        m_filePath = absolutePath;
        m_directoryName = absolutePath;
        m_fileName.clear();
        m_runtimeRolloverLength = 0; // no rolling over.

        AzFramework::StringFunc::Path::StripFullName(m_directoryName);

        m_fileIO->CreatePath(m_directoryName.c_str());

        AZ::IO::Result result = AZ::IO::ResultCode::Error;
        if (!forceOverwriteFile && m_fileIO->Exists(m_filePath.c_str()))
        {
            result = m_fileIO->Open(m_filePath.c_str(), AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeText, m_fileHandle);
        }
        else
        {
            result = m_fileIO->Open(m_filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, m_fileHandle);
        }


        if (result.GetResultCode() != AZ::IO::ResultCode::Success)
        {
            AZ_Warning("Log Component", result.GetResultCode() == AZ::IO::ResultCode::Success, "Unable to open the log file for writing");
        }

    }

    void LogFile::Init(const char* baseDirectory, const char* fileName, AZ::u64 rolloverLength, bool forceOverwriteFile)
    {
        m_fileIO = AZ::IO::FileIOBase::GetInstance();

        if ((!m_fileIO) || (!fileName) || (fileName[0] == '\0'))
        {
            return;
        }

        m_runtimeRolloverLength = rolloverLength;
        m_fileName = AZStd::string(fileName);
        m_directoryName = AZStd::string(baseDirectory);

        //Construct the file path
        if (!AzFramework::StringFunc::Path::ConstructFull(baseDirectory, fileName, suffix, m_filePath))
        {
            return;
        }
        AZStd::string fileDir;
        if (!AzFramework::StringFunc::Path::GetFullPath(m_filePath.c_str(), fileDir))
        {
            return;
        }
        //Ensure that the path exists before trying to open the file for writing
        m_fileIO->CreatePath(fileDir.c_str());

        AZ::IO::Result result = AZ::IO::ResultCode::Error;
        if (!forceOverwriteFile && m_fileIO->Exists(m_filePath.c_str()))
        {
            result = m_fileIO->Open(m_filePath.c_str(), AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeText, m_fileHandle);
        }
        else
        {
            result = m_fileIO->Open(m_filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, m_fileHandle);
        }


        if (result.GetResultCode() != AZ::IO::ResultCode::Success)
        {
            AZ_Warning("Log Component", result.GetResultCode() == AZ::IO::ResultCode::Success, "Unable to open the log file for writing");
        }
    }

    AZStd::string LogFile::CreateNewBackupFileName()
    {
        AZStd::sys_time_t timeNow = AZStd::GetTimeNowSecond();
        AZStd::string timeString;
        AZStd::to_string(timeString, timeNow);
        AZStd::string newSuffix = "." + timeString + suffix;
        AZStd::string backupFileName;

        //Construct the backup file path
        if (!AzFramework::StringFunc::Path::ConstructFull(m_directoryName.c_str(), m_fileName.c_str(), newSuffix.c_str(), backupFileName))
        {
            AZ_Warning("Log Component", false, "Unable to construct the backup file path");
            return "";
        }

        return backupFileName;
    }

    void LogFile::CheckSizeAndCycleFiles()
    {
        if (m_fileHandle && m_fileIO)
        {
            AZ::u64 fileSize;
            AZ::IO::Result result = m_fileIO->Tell(m_fileHandle, fileSize);

            if (result.GetResultCode() != AZ::IO::ResultCode::Success)
            {
                return;
            }
            if ((m_runtimeRolloverLength) && (fileSize >= m_runtimeRolloverLength))
            {
                //If the file size is more than the rollover length than close the current log file,
                //rename it and open a new logfile having  the same name as the old one
                result = m_fileIO->Close(m_fileHandle);
                if (result.GetResultCode() != AZ::IO::ResultCode::Success)
                {
                    m_fileHandle = AZ::IO::InvalidHandle;
                }

                AZStd::string backupFileName = CreateNewBackupFileName();

                AZStd::string logDir;
                if (!AzFramework::StringFunc::Path::GetFullPath(m_filePath.c_str(), logDir))
                {
                    return;
                }

                char filter[AZ_MAX_PATH_LEN] = { 0 };
                azstrcpy(filter, AZ_MAX_PATH_LEN, m_fileName.c_str());
                azstrcat(filter, AZ_MAX_PATH_LEN, "*.log");

                AZStd::map<AZ::u64, AZStd::string> modTimeFilesMap;
                //Find all the log files
                m_fileIO->FindFiles(logDir.c_str(), filter, [&](const char* filePath) -> bool
                    {
                        //adding (modTime,filepath)
                        modTimeFilesMap.insert(AZStd::make_pair(m_fileIO->ModificationTime(filePath), AZStd::string(filePath)));
                        return true;
                    });

                //Since map is sorted by mod time deleting files from top will remove older log files
                //We will only be deleting log files if the total number exceeds the max allowed
                while (modTimeFilesMap.size() > maxLogFiles)
                {
                    auto firstEntry = modTimeFilesMap.begin();
                    m_fileIO->Remove(firstEntry->second.c_str());
                    modTimeFilesMap.erase(firstEntry);
                }

                result = m_fileIO->Rename(m_filePath.c_str(), backupFileName.c_str());
                if (result.GetResultCode() == AZ::IO::ResultCode::Success)
                {
                    result = m_fileIO->Open(m_filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, m_fileHandle);
                    if (result.GetResultCode() != AZ::IO::ResultCode::Success)
                    {
                        printf("Log Component:Unable to open the log file for writing");
                    }
                }
                else
                {
                    //If we are here than it means that renaming failed,the best option here is to try opening the old log file again for writing
                    result = m_fileIO->Open(m_filePath.c_str(), AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeText, m_fileHandle);
                    if (result.GetResultCode() != AZ::IO::ResultCode::Success)
                    {
                        printf("Log Component:Unable to open the log file for writing");
                    }
                }
            }
        }
    }

    void LogFile::AppendLog(LogFile::SeverityLevel severity, const char* category, const char* dataSource)
    {
        if (m_fileHandle && dataSource && category)
        {
            AppendLog(severity, dataSource, (int)strlen(dataSource), category, (int)strlen(category));
        }
        else if (m_fileHandle && dataSource)
        {
            AppendLog(severity, dataSource, (int)strlen(dataSource));
        }
    }

    void LogFile::AppendLog(LogFile::SeverityLevel severity, const char* dataSource)
    {
        if (m_fileHandle && dataSource)
        {
            AppendLog(severity, dataSource, (int)strlen(dataSource));
        }
    }

    void LogFile::AppendLog(LogFile::SeverityLevel severity, const char* dataSource, int dataLength, const char* category, int categoryLen, uintptr_t threadId, AZ::u64 time)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> locker(m_logProtector);

        if ((!dataSource) || (dataSource[0] == 0))
        {
            return;
        }

        if (m_reentryProtection)
        {
            return;
        }
        ScopedReentrancyProtector protector(m_reentryProtection);

        bool addNewLine = (dataSource[dataLength - 1] != '\n');

        char categorybuffer[64] = {0};
        // On some platforms, threadid is just a number but on other platforms it is a pointer of some kind
        // uintptr_t will ensure that the data will always fit
        uintptr_t threadID = threadId ? threadId : (uintptr_t)(AZStd::this_thread::get_id().m_id);
        // while it may be tempting to check the fileio Pointer here, any emit of any warning or error would be fatal
        // since we're already logging, and we don't want to log while you log.

        const char* categoryActual = ((category) && (categoryLen)) ? category : "none";


        if (m_fileIO && m_fileHandle && dataSource)
        {
            char buffer[80] = { 0 };

            if (m_machineReadable)
            {
                // in machine-readable mode we write double delimiters before each column so
                // ~~MS_SINCE_EPOCH~~SEVERITY~~THREADID~~WINDOW~~MESSAGE

                AZ::u64 rawTime = time ? time : AZStd::GetTimeUTCMilliSecond();

                azsnprintf(buffer, 80, "~~%llu~~%i", rawTime, severity);
                m_fileIO->Write(m_fileHandle, buffer, strlen(buffer));
                if (m_machineReadable) // Branching instead of using a ternary on the format string to avoid warning 4774 (format literal expected)
                {
                    azsnprintf(buffer, 80, "~~%p~~%s~~", reinterpret_cast<void*>(threadID), categoryActual);
                }
                else
                {
                    azsnprintf(buffer, 80, "{%p}[%14s]", reinterpret_cast<void*>(threadID), categoryActual);
                }
                m_fileIO->Write(m_fileHandle, buffer, strlen(buffer));

                m_fileIO->Write(m_fileHandle, dataSource, dataLength);
                if (addNewLine)
                {
                    m_fileIO->Write(m_fileHandle, "\n", 1);
                }
            }
            else
            {
                TimeNowAsString(buffer, 80);

                AZ::IO::Result result = m_fileIO->Write(m_fileHandle, buffer, strlen(buffer));
                if (result.GetResultCode() != AZ::IO::ResultCode::Success)
                {
                    return;
                }
                if (m_machineReadable) // Branching instead of using a ternary on the format string to avoid warning 4774 (format literal expected)
                {
                    azsnprintf(categorybuffer, 64, "~~%p~~%s~~", reinterpret_cast<void*>(threadID), categoryActual);
                }
                else
                {
                    azsnprintf(categorybuffer, 64, "{%p}[%14s]", reinterpret_cast<void*>(threadID), categoryActual);
                }

                if ((category) && (categoryLen))
                {
                    m_fileIO->Write(m_fileHandle, categorybuffer, strlen(categorybuffer));
                }

                switch (severity)
                {
                case SEV_DEBUG:
                    m_fileIO->Write(m_fileHandle, " DBG ", 5);
                    break;
                case SEV_WARNING:
                    m_fileIO->Write(m_fileHandle, " WRN ", 5);
                    break;
                case SEV_ERROR:
                    m_fileIO->Write(m_fileHandle, " ERR ", 5);
                    break;
                case SEV_ASSERT:
                    m_fileIO->Write(m_fileHandle, " AST ", 5);
                    break;
                case SEV_EXCEPTION:
                    m_fileIO->Write(m_fileHandle, " EXC ", 5);
                    break;
                default:
                    m_fileIO->Write(m_fileHandle, "     ", 5);
                    break;
                }

                m_fileIO->Write(m_fileHandle, dataSource, dataLength);
                if (addNewLine)
                {
                    m_fileIO->Write(m_fileHandle, "\n", 1);
                }
            }
            
            if ((severity == SEV_ERROR) || (severity == SEV_ASSERT) || (severity == SEV_EXCEPTION))
            {
                // make sure these survive a crash.
                FlushLog();
            }

            CheckSizeAndCycleFiles();
        }
    }

    void LogFile::FlushLog()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> locker(m_logProtector);
        
        if (m_fileIO && m_fileHandle)
        {
            m_fileIO->Flush(m_fileHandle);
        }
    }

};
