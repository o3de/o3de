/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Uuid.h>
#include <future>

namespace AzToolsFramework
{
    //! ArchiveCommands
    //! This bus handles messages relating to archive commands
    //! archive commands are ASYNCHRONOUS
    //! archive formats officially supported are .zip
    class ArchiveCommands
        : public AZ::EBusTraits
    {
    public:
        // EBus Traits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;
        static const bool LocklessDispatch = true;

        virtual ~ArchiveCommands() = default;

        //! Create an archive of the target directory (all files and subdirectories)
        virtual std::future<bool> CreateArchive(
            const AZStd::string& archivePath,
            const AZStd::string& dirToArchive) = 0;

        //! Start an async task to extract an archive to the target directory
        virtual std::future<bool> ExtractArchive(
            const AZStd::string& archivePath,
            const AZStd::string& destinationPath) = 0;

        //! Extract a single file from the archive to the destination and block until finished.
        //! Destination path should not be empty.
        virtual std::future<bool> ExtractFile(
            const AZStd::string& archivePath,
            const AZStd::string& fileInArchive,
            const AZStd::string& destinationPath) = 0;

        //! Start a sync task to retrieve the list of files and their relative paths within an archive (recursively)
        virtual bool ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& outFileEntries) = 0;

        //! Start an async task to add a file to a preexisting archive. 
        //! fileToAdd must be a relative path to the file from the working directory. The path to the file from the root of the archive will be the same as the relative path to the file on disk.
        virtual std::future<bool> AddFileToArchive(
            const AZStd::string& archivePath,
            const AZStd::string& workingDirectory,
            const AZStd::string& fileToAdd) = 0;

        //! Start an async task to add files to an archive. 
        //! File paths inside the list file must either be a relative path from the working directory or an absolute path.
        virtual std::future<bool> AddFilesToArchive(
            const AZStd::string& archivePath,
            const AZStd::string& workingDirectory,
            const AZStd::string& listFilePath) = 0;
    };

    using ArchiveCommandsBus = AZ::EBus<ArchiveCommands>;

}; // namespace AzToolsFramework
