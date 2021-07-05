/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>

namespace AzToolsFramework
{
    class NullArchiveComponent
        : public AZ::Component
        , private ArchiveCommands::Bus::Handler
    {
    public:
        AZ_COMPONENT(NullArchiveComponent, "{D665B6B1-5FF4-4203-B19F-BBDB82587129}")

        NullArchiveComponent() = default;
        ~NullArchiveComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    private:
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // ArchiveCommands::Bus::Handler overrides
         // ArchiveCommands::Bus::Handler overrides
        void CreateArchive(const AZStd::string& archivePath, const AZStd::string& dirToArchive, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool CreateArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& dirToArchive) override;
        bool ExtractArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool extractWithRootDirectory) override;
        void ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback) override;
        void ExtractArchiveOutput(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void ExtractArchiveWithoutRoot(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void ExtractFile(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool ExtractFileBlocking(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite) override;
        void ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& consoleOutput, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool ListFilesInArchiveBlocking(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& consoleOutput) override;
        void AddFileToArchive(const AZStd::string& archivePath, const AZStd::string& fileToAdd, const AZStd::string& pathInArchive, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        bool AddFileToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& fileToAdd, const AZStd::string& pathInArchive) override;
        bool AddFilesToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath) override; 
        void AddFilesToArchive(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) override;
        void CancelTasks(AZ::Uuid taskHandle) override;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AzToolsFramework
