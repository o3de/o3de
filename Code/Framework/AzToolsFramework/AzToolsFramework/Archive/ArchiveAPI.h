/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Uuid.h>

namespace AzToolsFramework
{
    // use bind if you need additional context.
    // Parameters:
    //  bool - If the archive command was successful or not.
    typedef AZStd::function<void(bool)> ArchiveResponseCallback;
    //  bool - If the archive command was successful or not.
    //  AZStd::string - The console output from the command.
    typedef AZStd::function<void(bool, AZStd::string)> ArchiveResponseOutputCallback;


    //! ArchiveCommands
    //! This bus handles messages relating to archive commands
    //! archive commands are ASYNCHRONOUS
    //! archive formats officially supported are .zip
    //! do not block the main thread waiting for a response, it is not okay
    //! you will not get a message delivered unless you tick the tickbus anyway!
    class ArchiveCommands
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<ArchiveCommands>;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;
        static const bool LocklessDispatch = true;
        virtual ~ArchiveCommands() {}

        //! Start an async task to extract an archive to the target directory
        //! taskHandles are used to cancel a task at some point in the future and are provided by the caller per task.
        //! Multiple tasks can be associated with the same handle
        virtual void ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback) = 0;
        virtual void ExtractArchiveOutput(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) = 0;
        // Maintaining backwards API compatibility - ExtractArchiveBlocking below passes in extractWithRoot as an option
        virtual void ExtractArchiveWithoutRoot(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) = 0;

        //! Start a sync task to extract an archive to the target directory
        //! If you do not want to extract the root folder then set extractWithRootDirectory to false. 
        virtual bool ExtractArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool extractWithRootDirectory) = 0;

        //! Extract a single file asynchronously from the archive to the destination.  
        //! Uses cwd if destinationPath empty.  overWrite = true for overwrite existing files, false for skipExisting
        //! taskHandles are used to cancel a task at some point in the future and are provided by the caller per task.
        //! Multiple tasks can be associated with the same handle
        virtual void ExtractFile(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) = 0;

        //! Extract a single file from the archive to the destination and block until finished.  
        //! Uses cwd if destinationPath empty.  overWrite = true for overwrite existing files, false for skipExisting
        virtual bool ExtractFileBlocking(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite) = 0;

        //! Start an async task to create an archive of the target directory (recursively)
        //! taskHandles are used to cancel a task at some point in the future and are provided by the caller per task.
        //! Multiple tasks can be associated with the same handle.
        virtual void CreateArchive(const AZStd::string& archivePath, const AZStd::string& dirToArchive, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) = 0;

        //! Start a sync task to create an archive of the target directory (recursively)
        virtual bool CreateArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& dirToArchive) = 0;
        
        //! Start an async task to retrieve the list of files and their relative paths within an archive (recursively)
        //! taskHandles are used to cancel a task at some point in the future and are provided by the caller per task.
        //! Multiple tasks can be associated with the same handle.
        virtual void ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& fileEntries, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) = 0;

        //! Start a sync task to retrieve the list of files and their relative paths within an archive (recursively)
        virtual bool ListFilesInArchiveBlocking(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& fileEntries) = 0;

        //! Start an async task to add a file to a preexisting archive. 
        //! fileToAdd must be a relative path to the file from the working directory. The path to the file from the root of the archive will be the same as the relative path to the file on disk. 
        //! taskHandles are used to cancel a task at some point in the future and are provided by the caller per task.
        //! Multiple tasks can be associated with the same handle.
        virtual void AddFileToArchive(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& fileToAdd, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) = 0;

        //! Start a sync task to add a file to a preexisting archive. 
        //! fileToAdd must be a relative path to the file from the working directory. The path to the file from the root of the archive will be the same as the relative path to the file on disk. 
        virtual bool AddFileToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& fileToAdd) = 0;

        //! Start an async task to add files to a archive. 
        //! File paths inside the list file must either be a relative path from the working directory or an absolute path. 
        virtual void AddFilesToArchive(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback) = 0;
        
        //! Start a sync task to add files to an archive. 
        //! File paths inside the list file must either be a relative path from the working directory or an absolute path.
        virtual bool AddFilesToArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& workingDirectory, const AZStd::string& listFilePath) = 0;

        //! Cancels tasks associtated with the given handle. Blocks until all tasks are cancelled.
        virtual void CancelTasks(AZ::Uuid taskHandle) = 0;
    };
    using ArchiveCommandsBus = AZ::EBus<ArchiveCommands>;
}; // namespace AzToolsFramework
