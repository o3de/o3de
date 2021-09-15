/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveComponent.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Archive/ZipDirStructures.h>
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

    namespace ArchiveUtils
    {
        // Read a file's contents into a provided buffer
        // returns true if read was successful, false otherwise
        bool ReadFile(const AZStd::string& filePath, AZ::IO::OpenMode openMode, AZStd::vector<char>& outBuffer)
        {
            auto fileIO = AZ::IO::FileIOBase::GetDirectInstance();
            if (!fileIO)
            {
                return false;
            }

            bool success = false;
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            if (fileIO->Open(filePath.c_str(), openMode, fileHandle))
            {
                AZ::u64 fileSize = 0;
                if (fileIO->Size(fileHandle, fileSize) && fileSize != 0)
                {
                    outBuffer.resize_no_construct(fileSize);

                    AZ::u64 bytesRead = 0;
                    if (fileIO->Read(fileHandle, outBuffer.data(), fileSize, true, &bytesRead))
                    {
                        success = (fileSize == bytesRead);
                    }
                }

                fileIO->Close(fileHandle);
            }

            return success;
        }

        // Reads a file as text that contains a list of file paths.
        // Tokenize the file by lines.
        // Calls the lineVisitor function for each line of the file.
        void ProcessFileList(const AZStd::string& filePath, AZStd::function<void(AZStd::string_view line)> lineVisitor)
        {
            AZStd::vector<char> fileBuffer;
            if (ReadFile(filePath, AZ::IO::OpenMode::ModeText | AZ::IO::OpenMode::ModeRead, fileBuffer))
            {
                AZ::StringFunc::TokenizeVisitor(AZStd::string_view{ fileBuffer.data(), fileBuffer.size() }, lineVisitor, "\n");
            }
        }

    } // namespace ArchiveUtils

    void ArchiveComponent::Activate()
    {
        m_zipExePath = Platform::GetZipExePath();
        m_unzipExePath = Platform::GetUnzipExePath();

        m_localFileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
        if (m_localFileIO == nullptr)
        {
            AZ_Error(s_traceName, false, "Failed to create a LocalFileIO instance!");
        }

        m_archive = AZ::Interface<AZ::IO::IArchive>::Get();
        if (m_archive == nullptr)
        {
            AZ_Error(s_traceName, false, "Failed to get IArchive interface!");
        }

        ArchiveCommands::Bus::Handler::BusConnect();
    }

    void ArchiveComponent::Deactivate()
    {
        ArchiveCommands::Bus::Handler::BusDisconnect();

        m_localFileIO.reset();
        m_archive = nullptr;

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
        AZStd::string commandLineArgs = Platform::GetCreateArchiveCommand(archivePath, dirToArchive);
        LaunchZipExe(m_zipExePath, commandLineArgs, respCallback, taskHandle);
    }

    bool ArchiveComponent::CreateArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& dirToArchive)
    {
        if (!m_localFileIO || !m_archive)
        {
            return false;
        }

        if (m_localFileIO->Exists(archivePath.c_str()))
        {
            AZ_Error(s_traceName, false, "Archive file (%s) already exists, cannot create a new archive there!");
            return false;
        }

        if (!m_localFileIO->Exists(dirToArchive.c_str()) || !m_localFileIO->IsDirectory(dirToArchive.c_str()))
        {
            AZ_Error(s_traceName, false, "Directory to archive (%s) is not a directory or doesn't exist!", dirToArchive.c_str());
            return false;
        }

        auto archive = m_archive->OpenArchive(archivePath, nullptr, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
        if (!archive)
        {
            AZ_Error(s_traceName, false, "Failed to create archive file (%s)", archivePath.c_str());
            return false;
        }

        auto foundFiles = AzFramework::FileFunc::FindFilesInPath(dirToArchive, "*", true);
        if (!foundFiles.IsSuccess())
        {
            AZ_Error(s_traceName, false, "Failed to find file listing under directory %d", dirToArchive.c_str());
            return false;
        }

        bool success = true;
        AZStd::vector<char> fileBuffer;
        const AZ::IO::Path workingPath{ dirToArchive };

        for (const auto& fileName : foundFiles.GetValue())
        {
            bool thisSuccess = false;

            AZ::IO::PathView relativePath = AZ::IO::PathView{ fileName }.LexicallyRelative(workingPath);

            AZStd::string fullPath = (workingPath / relativePath).c_str();
            if (ArchiveUtils::ReadFile(fullPath, AZ::IO::OpenMode::ModeRead, fileBuffer))
            {
                int result = archive->UpdateFile(
                    relativePath.Native(), fileBuffer.data(), fileBuffer.size(), AZ::IO::INestedArchive::METHOD_COMPRESS,
                    AZ::IO::INestedArchive::LEVEL_DEFAULT, CompressionCodec::Codec::ZLIB);

                thisSuccess = (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
                AZ_Error(
                    s_traceName, thisSuccess, "Error %d encountered while adding '%s' to archive '%s'", result, fileName.c_str(),
                    archive->GetFullPath());
            }
            else
            {
                AZ_Error(
                    s_traceName, false, "Error encountered while reading '%s' to add to archive '%s'", fileName.c_str(),
                    archive->GetFullPath());
            }

            success = (success && thisSuccess);
        }

        return success;
    }

    void ArchiveComponent::ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback)
    {
        ArchiveResponseOutputCallback responseHandler = [respCallback](bool result, [[maybe_unused]] AZStd::string outputStr) { respCallback(result); };

        AZStd::string commandLineArgs = Platform::GetExtractArchiveCommand(archivePath, destinationPath, true);

        if (commandLineArgs.empty())
        {
            // The platform-specific implementation has already thrown its own error, no need to throw another one
            return;
        }

        LaunchZipExe(m_unzipExePath, commandLineArgs, responseHandler, taskHandle);
    }

    bool ArchiveComponent::ExtractFileBlocking(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite)
    {
        auto fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO || !m_archive)
        {
            return false;
        }

        if (!fileIO->Exists(archivePath.c_str()))
        {
            AZ_Error(s_traceName, false, "Archive '%s' does not exist!", archivePath.c_str());
            return false;
        }

        if (!fileIO->Exists(destinationPath.c_str()))
        {
            if (!fileIO->CreatePath(destinationPath.c_str()))
            {
                AZ_Error(s_traceName, false, "Failed to create destination directory '%s'", destinationPath.c_str());
                return false;
            }
        }

        auto archive = m_archive->OpenArchive(archivePath);
        if (!archive)
        {
            AZ_Error(s_traceName, false, "Failed to open archive file (%s)", archivePath.c_str());
            return false;
        }

        AZ::IO::INestedArchive::Handle fileHandle = archive->FindFile(fileInArchive);
        if (!fileHandle)
        {
            AZ_Error(s_traceName, false, "File '%s' does not exist inside archive '%s'", fileInArchive.c_str(), archivePath.c_str());
            return false;
        }

        AZ::u64 fileSize = archive->GetFileSize(fileHandle);
        AZStd::vector<AZ::u8> fileBuffer;
        fileBuffer.resize_no_construct(fileSize);

        if (auto result = archive->ReadFile(fileHandle, fileBuffer.data());
            result != AZ::IO::ZipDir::ZD_ERROR_SUCCESS)
        {
            AZ_Error(
                s_traceName, false, "Failed to read file '%s' in archive '%s' with error %d", fileInArchive.c_str(), archivePath.c_str(),
                result);
            return false;
        }

        AZ::IO::HandleType destFileHandle = AZ::IO::InvalidHandle;
        AZ::IO::Path destinationFile{ destinationPath };
        destinationFile /= fileInArchive;
        AZ::IO::OpenMode openMode = (AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite);
        if (overWrite)
        {
            openMode |= AZ::IO::OpenMode::ModeUpdate;
        }
        if (!m_localFileIO->Open(destinationFile.c_str(), openMode, destFileHandle))
        {
            AZ_Error(s_traceName, false, "Failed to open destination file '%s' for writing", destinationFile.c_str());
            return false;
        }

        AZ::u64 bytesWritten = 0;
        if (!m_localFileIO->Write(destFileHandle, fileBuffer.data(), fileSize, &bytesWritten))
        {
            AZ_Error(s_traceName, false, "Failed to write destination file '%s'", destinationFile.c_str());
            m_localFileIO->Close(destFileHandle);
            return false;
        }

        m_localFileIO->Close(destFileHandle);

        return (fileSize == bytesWritten);
    }

    bool ArchiveComponent::ListFilesInArchiveBlocking(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& fileEntries)
    {
        auto fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO || !m_archive)
        {
            return false;
        }

        if (!fileIO->Exists(archivePath.c_str()))
        {
            AZ_Error(s_traceName, false, "Archive '%s' does not exist!", archivePath.c_str());
            return false;
        }

        auto archive = m_archive->OpenArchive(archivePath);
        if (!archive)
        {
            AZ_Error(s_traceName, false, "Failed to open archive file (%s)", archivePath.c_str());
            return false;
        }

        int result = archive->ListAllFiles(fileEntries);
        return (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
    }

    bool ArchiveComponent::AddFileToArchiveBlocking(
        const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& fileToAdd)
    {
        if (!m_localFileIO || !m_archive)
        {
            return false;
        }

        if (!m_localFileIO->Exists(workingDirectory.c_str()) || !m_localFileIO->IsDirectory(workingDirectory.c_str()))
        {
            AZ_Error(
                s_traceName, false, "Working directory for archive (%s) is not a directory or doesn't exist!", workingDirectory.c_str());
            return false;
        }

        auto archive = m_archive->OpenArchive(archivePath);
        if (!archive)
        {
            AZ_Error(s_traceName, false, "Failed to open archive file (%s)", archivePath.c_str());
            return false;
        }

        AZStd::string fullPath = (AZ::IO::Path{ workingDirectory } / fileToAdd).c_str();
        AZStd::vector<char> fileBuffer;
        bool success = false;
        if (ArchiveUtils::ReadFile(fullPath, AZ::IO::OpenMode::ModeRead, fileBuffer))
        {
            int result = archive->UpdateFile(
                fileToAdd, fileBuffer.data(), fileBuffer.size(), AZ::IO::INestedArchive::METHOD_COMPRESS,
                AZ::IO::INestedArchive::LEVEL_DEFAULT, CompressionCodec::Codec::ZLIB);

            success = (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
            AZ_Error(
                s_traceName, success, "Error %d encountered while adding '%s' to archive '%s'", result, fileToAdd.c_str(),
                archive->GetFullPath());
        }
        else
        {
            AZ_Error(
                s_traceName, false, "Error encountered while reading '%s' to add to archive '%s'", fileToAdd.c_str(),
                archive->GetFullPath());
        }

        return success;
    }

    bool ArchiveComponent::AddFilesToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath)
    {
        if (!m_localFileIO || !m_archive)
        {
            return false;
        }

        if (!m_localFileIO->Exists(workingDirectory.c_str()) || !m_localFileIO->IsDirectory(workingDirectory.c_str()))
        {
            AZ_Error(s_traceName, false, "Working directory for archive (%s) is not a directory or doesn't exist!", workingDirectory.c_str());
            return false;
        }

        if (!m_localFileIO->Exists(listFilePath.c_str()) || m_localFileIO->IsDirectory(listFilePath.c_str()))
        {
            AZ_Error(s_traceName, false, "File list (%s) is a directory or doesn't exist!", listFilePath.c_str());
            return false;
        }

        auto archive = m_archive->OpenArchive(archivePath);
        if (!archive)
        {
            AZ_Error(s_traceName, false, "Failed to create archive file (%s)", archivePath.c_str());
            return false;
        }

        bool success = true;    // starts true and turns false when any error is encountered.
        AZ::IO::Path basePath{ workingDirectory };

        auto PerLineCallback = [&success, &basePath, &archive](AZStd::string_view filePathLine) -> void
        {
            AZStd::vector<char> fileBuffer;
            AZStd::string fullPath = (basePath / filePathLine).c_str();
            if (ArchiveUtils::ReadFile(fullPath, AZ::IO::OpenMode::ModeRead, fileBuffer))
            {
                int result = archive->UpdateFile(
                    filePathLine, fileBuffer.data(), fileBuffer.size(), AZ::IO::INestedArchive::METHOD_COMPRESS,
                    AZ::IO::INestedArchive::LEVEL_NORMAL, CompressionCodec::Codec::ZLIB);

                bool thisSuccess = (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
                success = (success && thisSuccess);
                AZ_Error(
                    s_traceName, thisSuccess, "Error %d encountered while adding '%.*s' to archive '%s'", result, AZ_STRING_ARG(filePathLine),
                    archive->GetFullPath());
            }
            else
            {
                AZ_Error(
                    s_traceName, false, "Error encountered while reading '%.*s' to add to archive '%s'", AZ_STRING_ARG(filePathLine),
                    archive->GetFullPath());
            }
        };

        ArchiveUtils::ProcessFileList(listFilePath, PerLineCallback);

        return success;
    }

    bool ArchiveComponent::ExtractArchiveBlocking(
        const AZStd::string& archivePath, const AZStd::string& destinationPath, [[maybe_unused]] bool extractWithRootDirectory)
    {
        auto fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO || !m_archive)
        {
            return false;
        }

        if (!fileIO->Exists(archivePath.c_str()))
        {
            AZ_Error(s_traceName, false, "Archive '%s' does not exist!", archivePath.c_str());
            return false;
        }

        if (!fileIO->Exists(destinationPath.c_str()))
        {
            if (!fileIO->CreatePath(destinationPath.c_str()))
            {
                AZ_Error(s_traceName, false, "Failed to create destination directory '%s'", destinationPath.c_str());
                return false;
            }
        }

        auto archive = m_archive->OpenArchive(archivePath);
        if (!archive)
        {
            AZ_Error(s_traceName, false, "Failed to open archive file '%s'", archivePath.c_str());
            return false;
        }

        AZStd::vector<AZStd::string> filesInArchive;
        if (int result = archive->ListAllFiles(filesInArchive); result != AZ::IO::ZipDir::ZD_ERROR_SUCCESS)
        {
            AZ_Error(s_traceName, false, "Failed to get list of files in archive '%s'", archivePath.c_str());
            return false;
        }

        AZStd::vector<AZ::u8> fileBuffer;
        AZ::IO::Path destination{ destinationPath };
        AZ::u64 fileSize = 0;
        AZ::u64 numFilesWritten = 0;
        AZ::u64 bytesWritten = 0;
        AZ::IO::INestedArchive::Handle srcHandle{};
        AZ::IO::HandleType dstHandle = AZ::IO::InvalidHandle;
        const AZ::IO::OpenMode openMode = (AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeUpdate);

        for (const auto& filePath : filesInArchive)
        {
            srcHandle = archive->FindFile(filePath);
            AZ_Assert(srcHandle != nullptr, "File '%s' does not exist inside archive '%s'", filePath.c_str(), archivePath.c_str());

            fileSize = (srcHandle != nullptr) ? archive->GetFileSize(srcHandle) : 0;
            fileBuffer.resize_no_construct(fileSize);
            if (auto result = archive->ReadFile(srcHandle, fileBuffer.data()); result != AZ::IO::ZipDir::ZD_ERROR_SUCCESS)
            {
                AZ_Error(
                    s_traceName, false, "Failed to read file '%s' in archive '%s' with error %d", filePath.c_str(), archivePath.c_str(),
                    result);
                continue;
            }

            AZ::IO::Path destinationFile = destination / filePath;
            if (!m_localFileIO->Open(destinationFile.c_str(), openMode, dstHandle))
            {
                AZ_Error(s_traceName, false, "Failed to open '%s' for writing", destinationFile.c_str());
                continue;
            }

            if (!m_localFileIO->Write(dstHandle, fileBuffer.data(), fileSize, &bytesWritten))
            {
                AZ_Error(s_traceName, false, "Failed to write destination file '%s'", destinationFile.c_str());
            }
            else if (bytesWritten == fileSize)
            {
                ++numFilesWritten;
            }

            m_localFileIO->Close(dstHandle);
        }

        return (numFilesWritten == filesInArchive.size());
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
                return info.threads.find(processThread.get_id()) == info.threads.end();
            });
            processThread.detach();
        }
        else
        {
            sevenZJob();
        }
    }
} // namespace AzToolsFramework
