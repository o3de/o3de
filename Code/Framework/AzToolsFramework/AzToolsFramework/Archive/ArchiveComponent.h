/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/Component.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>

#include <AzFramework/Archive/IArchive.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>

namespace AzToolsFramework
{
    // the ArchiveComponent's job is to create and manipulate zip archives.
    class ArchiveComponent
        : public AZ::Component
        , private ArchiveCommandsBus::Handler
    {
    public:
        AZ_COMPONENT(ArchiveComponent, "{A19EEA33-3736-447F-ACF7-DAA4B6A179AA}");

        ArchiveComponent() = default;
        ~ArchiveComponent() override = default;

        ArchiveComponent(const ArchiveComponent&) = delete;
        ArchiveComponent& operator=(const ArchiveComponent&) = delete;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
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

    private:
        AZ::IO::FileIOBase* m_fileIO = nullptr;
        AZ::IO::IArchive* m_archive = nullptr;
        AZStd::vector<AZStd::thread> m_threads;

        bool CheckParamsForAdd(const AZStd::string& directory, const AZStd::string& file);
        bool CheckParamsForExtract(const AZStd::string& archive, const AZStd::string& directory);
        bool CheckParamsForCreate(const AZStd::string& archive, const AZStd::string& directory);
    };

} // namespace AzToolsFramework
