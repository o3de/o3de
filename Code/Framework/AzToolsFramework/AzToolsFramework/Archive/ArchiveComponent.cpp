/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveComponent.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/FileFunc/FileFunc.h>

namespace AzToolsFramework
{
    // Forward declare platform specific functions
    namespace Platform
    {
        AZStd::string GetZipExePath();
        AZStd::string GetUnzipExePath();

        AZStd::string GetCreateArchiveCommand(const AZStd::string& archivePath, const AZStd::string& dirToArchive);
        AZStd::string GetExtractArchiveCommand(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool includeRoot);
        AZStd::string GetAddFileToArchiveCommand(const AZStd::string& archivePath, const AZStd::string& file);
        AZStd::string GetAddFilesToArchiveCommand(const AZStd::string& archivePath, const AZStd::string& listFilePath);
        AZStd::string GetExtractFileCommand(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite);
        AZStd::string GetListFilesInArchiveCommand(const AZStd::string& archivePath);
        void ParseConsoleOutputFromListFilesInArchive(const AZStd::string& consoleOutput, AZStd::vector<AZStd::string>& fileEntries);
    }

    const char s_traceName[] = "ArchiveComponent";
    const unsigned int g_sleepDuration = 1;

    // Echoes all results of stdout and stderr to console and never blocks
    class ConsoleEchoCommunicator
    {
    public:
        ConsoleEchoCommunicator(AzFramework::ProcessCommunicator* communicator)
            : m_communicator(communicator)
        {
        }

        ~ConsoleEchoCommunicator()
        {
        }

        // Call this periodically to drain the buffers
        void Pump()
        {
            if (m_communicator->IsValid())
            {
                AZ::u32 readBufferSize = 0;
                AZStd::string readBuffer;
                // Don't call readOutput unless there is output or else it will block...
                readBufferSize = m_communicator->PeekOutput();
                if (readBufferSize)
                {
                    readBuffer.resize_no_construct(readBufferSize + 1);
                    readBuffer[readBufferSize] = '\0';
                    m_communicator->ReadOutput(readBuffer.data(), readBufferSize);
                    EchoBuffer(readBuffer);
                }
                readBufferSize = m_communicator->PeekError();
                if (readBufferSize)
                {
                    readBuffer.resize_no_construct(readBufferSize + 1);
                    readBuffer[readBufferSize] = '\0';
                    m_communicator->ReadError(readBuffer.data(), readBufferSize);
                    EchoBuffer(readBuffer);
                }
            }
        }

    private:
        void EchoBuffer(const AZStd::string& buffer)
        {
            size_t startIndex = 0;
            size_t endIndex = 0;
            const size_t bufferSize = buffer.size();
            for (size_t i = 0; i < bufferSize; ++i)
            {
                if (buffer[i] == '\n' || buffer[i] == '\0')
                {
                    endIndex = i;
                    bool isEmptyMessage = (endIndex - startIndex == 1) && (buffer[startIndex] == '\r');
                    if (!isEmptyMessage)
                    {
                        AZ_Printf(s_traceName, "%s", buffer.substr(startIndex, endIndex - startIndex).c_str());
                    }
                    startIndex = endIndex + 1;
                }
            }
        }

        AzFramework::ProcessCommunicator* m_communicator = nullptr;
    };

    void ArchiveComponent::Activate()
    {
        m_zipExePath = Platform::GetZipExePath();
        m_unzipExePath = Platform::GetUnzipExePath();

        ArchiveCommands::Bus::Handler::BusConnect();
    }

    void ArchiveComponent::Deactivate()
    {
        ArchiveCommands::Bus::Handler::BusDisconnect();

        AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
        for (auto pair : m_threadInfoMap)
        {
            ThreadInfo& info = pair.second;
            info.shouldStop = true;
            m_cv.wait(lock, [&info]() {
                return info.threads.size() == 0;
            });
        }
        m_threadInfoMap.clear();
    }

    void ArchiveComponent::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArchiveComponent, AZ::Component>()
                ->Version(2)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC("AssetBuilder", 0xc739c7d7) }))
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ArchiveComponent>(
                    "Archive", "Handles creation and extraction of zip archives.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    void ArchiveComponent::CreateArchive(const AZStd::string& archivePath, const AZStd::string& dirToArchive, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = AZStd::string::format(R"(a -tzip -mx=1 "%s" -r "%s\*")", archivePath.c_str(), dirToArchive.c_str());
        LaunchZipExe(m_zipExePath, commandLineArgs, respCallback, taskHandle);
    }

    bool ArchiveComponent::CreateArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& dirToArchive)
    {
        bool success = false;
        auto createArchiveCallback = [&success](bool result, AZStd::string consoleOutput) {
            success = result;
        };

        AZStd::string commandLineArgs = Platform::GetCreateArchiveCommand(archivePath, dirToArchive);

        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return false;
        }

        LaunchZipExe(m_zipExePath, commandLineArgs, createArchiveCallback, AZ::Uuid::CreateNull(), dirToArchive, false);
        return success;
    }

    void ArchiveComponent::ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback)
    {
        ArchiveResponseOutputCallback responseHandler = [respCallback](bool result, AZStd::string /*outputStr*/) { respCallback(result); };
        ExtractArchiveOutput(archivePath, destinationPath, taskHandle, responseHandler);
    }

    void ArchiveComponent::ExtractArchiveOutput(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = Platform::GetExtractArchiveCommand(archivePath, destinationPath, true);

        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return;
        }

        LaunchZipExe(m_unzipExePath, commandLineArgs, respCallback, taskHandle);
    }

    void ArchiveComponent::ExtractArchiveWithoutRoot(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = Platform::GetExtractArchiveCommand(archivePath, destinationPath, false);

        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return;
        }

        LaunchZipExe(m_unzipExePath, commandLineArgs, respCallback, taskHandle);
    }

    void ArchiveComponent::ExtractFile(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = AzToolsFramework::Platform::GetExtractFileCommand(archivePath, fileInArchive, destinationPath, overWrite);
        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return;
        }
        LaunchZipExe(m_unzipExePath, commandLineArgs, respCallback, taskHandle);
    }

    bool ArchiveComponent::ExtractFileBlocking(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite)
    {
        AZStd::string commandLineArgs = AzToolsFramework::Platform::GetExtractFileCommand(archivePath, fileInArchive, destinationPath, overWrite);
        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return false;
        }

        bool success = false;
        auto createArchiveCallback = [&success](bool result, AZStd::string consoleOutput) {
            success = result;
        };
        LaunchZipExe(m_unzipExePath, commandLineArgs, createArchiveCallback);
        return success;
    }

    void ArchiveComponent::ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& fileEntries, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = Platform::GetListFilesInArchiveCommand(archivePath);
        
        auto parseOutput = [respCallback, taskHandle, &fileEntries](bool exitCode, AZStd::string consoleOutput)
        {
            Platform::ParseConsoleOutputFromListFilesInArchive(consoleOutput, fileEntries);
            AZ::TickBus::QueueFunction(respCallback, exitCode, AZStd::move(consoleOutput));
        };
        LaunchZipExe(m_unzipExePath, commandLineArgs, parseOutput, taskHandle, "", true);
    }

    bool ArchiveComponent::ListFilesInArchiveBlocking(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& fileEntries)
    {
        AZStd::string listOutput;
        AZStd::string commandLineArgs = Platform::GetListFilesInArchiveCommand(archivePath.c_str());
        bool success = false;

        auto parseOutput = [&success, &fileEntries](bool result, AZStd::string consoleOutput)
        {
            Platform::ParseConsoleOutputFromListFilesInArchive(consoleOutput, fileEntries);
            success = result;
        };
        LaunchZipExe(m_unzipExePath, commandLineArgs, parseOutput, AZ::Uuid::CreateNull(), "", true);
        return success;
    }

    void ArchiveComponent::AddFileToArchive(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& fileToAdd, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = Platform::GetAddFileToArchiveCommand(archivePath, fileToAdd);
        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return;
        }

        LaunchZipExe(m_zipExePath, commandLineArgs, respCallback, taskHandle, workingDirectory);
    }

    bool ArchiveComponent::AddFileToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& fileToAdd)
    {
        AZStd::string commandLineArgs = Platform::GetAddFileToArchiveCommand(archivePath, fileToAdd);
        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return false;
        }
        bool success = false;
        auto addFileToArchiveCallback = [&success](bool result, AZStd::string consoleOutput) {
            success = result;
        };

        LaunchZipExe(m_zipExePath, commandLineArgs, addFileToArchiveCallback, AZ::Uuid::CreateNull(), workingDirectory);
        return success;
    }

    bool ArchiveComponent::AddFilesToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath)
    {
        bool success = false;

        auto addFileToArchiveCallback = [&success](bool result, AZStd::string consoleOutput) {
            success = result;
        };

        AZStd::string commandLineArgs = Platform::GetAddFilesToArchiveCommand(archivePath.c_str(), listFilePath.c_str());

        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return false;
        }
        LaunchZipExe(m_zipExePath, commandLineArgs, addFileToArchiveCallback, AZ::Uuid::CreateNull(), workingDirectory);
        return success;
    }

    void ArchiveComponent::AddFilesToArchive(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = Platform::GetAddFilesToArchiveCommand(archivePath, listFilePath);
        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return;
        }

        LaunchZipExe(m_zipExePath, commandLineArgs, respCallback, taskHandle, workingDirectory);
    }


    bool ArchiveComponent::ExtractArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool extractWithRootDirectory)
    {
        AZStd::string commandLineArgs = Platform::GetExtractArchiveCommand(archivePath, destinationPath, extractWithRootDirectory);

        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return false;
        }

        bool success = false;
        auto extractArchiveCallback = [&success](bool result, AZStd::string consoleOutput) {
            success = result;
        };

        LaunchZipExe(m_unzipExePath, commandLineArgs, extractArchiveCallback);
        return success;
    }

    void ArchiveComponent::CancelTasks(AZ::Uuid taskHandle)
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);

        auto it = m_threadInfoMap.find(taskHandle);
        if (it == m_threadInfoMap.end())
        {
            return;
        }

        ThreadInfo& info = it->second;
        info.shouldStop = true;
        m_cv.wait(lock, [&info]() {
            return info.threads.size() == 0;
        });
        m_threadInfoMap.erase(it);
    }
    
    void ArchiveComponent::LaunchZipExe(const AZStd::string& exePath, const AZStd::string& commandLineArgs, const ArchiveResponseOutputCallback& respCallback, AZ::Uuid taskHandle, const AZStd::string& workingDir, bool captureOutput)
    {
        auto sevenZJob = [=]()
        {
            if (!taskHandle.IsNull())
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                m_threadInfoMap[taskHandle].threads.insert(AZStd::this_thread::get_id());
                m_cv.notify_all();
            }

            AzFramework::ProcessLauncher::ProcessLaunchInfo info;
            info.m_commandlineParameters = exePath + " " + commandLineArgs;
            
            info.m_showWindow = false;
            if (!workingDir.empty())
            {
                info.m_workingDirectory = workingDir;
            }
            AZStd::unique_ptr<AzFramework::ProcessWatcher> watcher(AzFramework::ProcessWatcher::LaunchProcess(info, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT));

            AZStd::string consoleOutput;
            AZ::u32 exitCode = static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess);
            if (watcher)
            {
                // callback requires output captured from 7z 
                if (captureOutput)
                {
                    AZStd::string consoleBuffer;
                    while (watcher->IsProcessRunning(&exitCode))
                    {
                        if (!taskHandle.IsNull())
                        {
                            AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                            if (m_threadInfoMap[taskHandle].shouldStop)
                            {
                                watcher->TerminateProcess(static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess));
                            }
                        }
                        watcher->WaitForProcessToExit(g_sleepDuration, &exitCode);
                        AZ::u32 outputSize = watcher->GetCommunicator()->PeekOutput();
                        if (outputSize)
                        {
                            consoleBuffer.resize(outputSize);
                            watcher->GetCommunicator()->ReadOutput(consoleBuffer.data(), outputSize);
                            consoleOutput += consoleBuffer;
                        }
                    }
                }
                else
                {
                    ConsoleEchoCommunicator echoCommunicator(watcher->GetCommunicator());
                    while (watcher->IsProcessRunning(&exitCode))
                    {
                        if (!taskHandle.IsNull())
                        {
                            AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                            if (m_threadInfoMap[taskHandle].shouldStop)
                            {
                                watcher->TerminateProcess(static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess));
                            }
                        }
                        watcher->WaitForProcessToExit(g_sleepDuration, &exitCode);
                        echoCommunicator.Pump();
                    }
                }
            }

            if (taskHandle.IsNull())
            {
                respCallback(exitCode == static_cast<AZ::u32>(SevenZipExitCode::NoError), AZStd::move(consoleOutput));
            }
            else
            {
                AZ::TickBus::QueueFunction(respCallback, (exitCode == static_cast<AZ::u32>(SevenZipExitCode::NoError)), AZStd::move(consoleOutput));
            }

            if (!taskHandle.IsNull())
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                ThreadInfo& tInfo = m_threadInfoMap[taskHandle];
                tInfo.threads.erase(AZStd::this_thread::get_id());
                m_cv.notify_all();
            }
        };
        if (!taskHandle.IsNull())
        {
            AZStd::thread processThread(sevenZJob);
            AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
            ThreadInfo& info = m_threadInfoMap[taskHandle];
            m_cv.wait(lock, [&info, &processThread]() {
                return info.threads.find(processThread.get_id()) != info.threads.end();
            });
            processThread.detach();
        }
        else
        {
            sevenZJob();
        }
    }
} // namespace AzToolsFramework
