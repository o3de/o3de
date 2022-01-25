/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        , private ArchiveCommandsBus::Handler
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
        // ArchiveCommandsBus::Handler overrides
        [[nodiscard]] std::future<bool> CreateArchive(
            const AZStd::string& archivePath,
            const AZStd::string& dirToArchive) override;

        [[nodiscard]] std::future<bool> ExtractArchive(
            const AZStd::string& archivePath,
            const AZStd::string& destinationPath) override;

        [[nodiscard]] std::future<bool> ExtractFile(
            const AZStd::string& archivePath,
            const AZStd::string& fileInArchive,
            const AZStd::string& destinationPath) override;

        bool ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& outFileEntries) override;

        [[nodiscard]] std::future<bool> AddFileToArchive(
            const AZStd::string& archivePath,
            const AZStd::string& workingDirectory,
            const AZStd::string& fileToAdd) override;

        [[nodiscard]] std::future<bool> AddFilesToArchive(
            const AZStd::string& archivePath,
            const AZStd::string& workingDirectory,
            const AZStd::string& listFilePath) override;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AzToolsFramework
