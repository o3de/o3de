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

#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>

#include <AzToolsFramework/Archive/ArchiveAPI.h>

namespace AzToolsFramework
{
    enum class SevenZipExitCode : AZ::u32
    {
        NoError = 0,
        Warning = 1,
        FatalError = 2,
        CommandLineError = 7,
        NotEnoughMemory = 8,
        UserStoppedProcess = 255
    };

    // the ArchiveComponent's job is to execute zip commands.
    // it parses the status of zip commands and returns results.
    class ArchiveComponent
        : public AZ::Component
        , private ArchiveCommands::Bus::Handler
    {
    public:
        AZ_COMPONENT(ArchiveComponent, "{A19EEA33-3736-447F-ACF7-DAA4B6A179AA}")

        ArchiveComponent() = default;
        ~ArchiveComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    private:
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // ArchiveCommands::Bus::Handler overrides
        void CreateArchive(const AZStd::string& archivePath, const AZStd::string& dirToArchive, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool CreateArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& dirToArchive) override;
        bool ExtractArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool extractWithRootDirectory) override;
        void ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback) override;
        void ExtractArchiveOutput(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void ExtractArchiveWithoutRoot(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void ExtractFile(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool ExtractFileBlocking(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite) override;
        void ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& fileEntries, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool ListFilesInArchiveBlocking(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& fileEntries) override;
        void AddFileToArchive(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& fileToAdd, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool AddFileToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& fileToAdd) override;
        bool AddFilesToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath) override;
        void AddFilesToArchive(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void CancelTasks(AZ::Uuid taskHandle) override;
        //////////////////////////////////////////////////////////////////////////
        
        // Launches the input zip exe as a background child process in a detached background thread, if the task handle is not null 
        // otherwise launches input zip exe in the calling thread.
        void LaunchZipExe(const AZStd::string& exePath, const AZStd::string& commandLineArgs, const ArchiveResponseOutputCallback& respCallback, AZ::Uuid taskHandle = AZ::Uuid::CreateNull(), const AZStd::string& workingDir = "", bool captureOutput = false);

        AZStd::string m_zipExePath;
        AZStd::string m_unzipExePath;

        // Struct for tracking background threads/tasks
        struct ThreadInfo
        {
            bool shouldStop = false;
            AZStd::set<AZStd::thread::id> threads;
        };

        AZStd::mutex m_threadControlMutex; // Guards m_threadInfoMap
        AZStd::condition_variable m_cv;
        AZStd::unordered_map<AZ::Uuid, ThreadInfo> m_threadInfoMap;
    };
} // namespace AzToolsFramework
