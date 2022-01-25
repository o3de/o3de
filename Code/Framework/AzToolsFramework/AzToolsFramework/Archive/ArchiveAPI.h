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
        //! @param archivePath The path of the archive to create
        //! @dirToArchive The directory to be added to the archive
        //! @return Future (bool) which can obtain the success value of the operation
        [[nodiscard]] virtual std::future<bool> CreateArchive(
            const AZStd::string& archivePath,
            const AZStd::string& dirToArchive) = 0;

        //! Extract an archive to the target directory
        //! @param archivePath The path of the archive to extract
        //! @param destinationPath The directory where files will be extracted to
        //! @return Future (bool) which can obtain the success value of the operation
        [[nodiscard]] virtual std::future<bool> ExtractArchive(
            const AZStd::string& archivePath,
            const AZStd::string& destinationPath) = 0;

        //! Extract a single file from the archive to the destination
        //! Destination path should not be empty
        //! @param archivePath The path of the archive to extract from
        //! @param fileInArchive A path to a file, relative to root of archive
        //! @param destinationPath The directory where file will be extracted to
        //! @return Future (bool) which can obtain the success value of the operation
        [[nodiscard]] virtual std::future<bool> ExtractFile(
            const AZStd::string& archivePath,
            const AZStd::string& fileInArchive,
            const AZStd::string& destinationPath) = 0;

        //! Retrieve the list of files contained in an archive (all files and subdirectories)
        //! @param archivePath The path of the archive to list
        //! @param outFileEntries An out parameter that will contain the file paths found
        //! @return True if successful, false otherwise
        virtual bool ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& outFileEntries) = 0;

        //! Add a file to an archive
        //! The archive might not exist yet
        //! The file path relative to the working directory will be replicated in the archive
        //! @param archivePath The path of the archive to add to
        //! @param workingDirectory A directory that will be the starting path of the file to be added
        //! @param fileToAdd A path to the file relative to the working directory
        //! @return Future (bool) which can obtain the success value of the operation
        [[nodiscard]] virtual std::future<bool> AddFileToArchive(
            const AZStd::string& archivePath,
            const AZStd::string& workingDirectory,
            const AZStd::string& fileToAdd) = 0;

        //! Add files to an archive provided from a file listing
        //! The archive might not exist yet
        //! File paths in the file list should be relative to root of the archive
        //! @param archivePath The path of the archive to add to
        //! @param workingDirectory A directory that will be the starting path of the list of files to add
        //! @param listFilePath Full path to a text file that contains the list of files to add
        //! @return Future (bool) which can obtain the success value of the operation
        [[nodiscard]] virtual std::future<bool> AddFilesToArchive(
            const AZStd::string& archivePath,
            const AZStd::string& workingDirectory,
            const AZStd::string& listFilePath) = 0;
    };

    using ArchiveCommandsBus = AZ::EBus<ArchiveCommands>;

}; // namespace AzToolsFramework
