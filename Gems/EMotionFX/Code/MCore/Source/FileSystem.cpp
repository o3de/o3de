/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FileSystem.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/std/functional.h>
#include "MCoreCommandManager.h"
#include "StringConversions.h"

namespace MCore
{
    // The folder path used to keep a backup in SaveToFileSecured.
    StaticString FileSystem::mSecureSavePath;

    // Save to file secured by a backup file.
    bool FileSystem::SaveToFileSecured(const char* filename, const AZStd::function<bool()>& saveFunction, CommandManager* commandManager)
    {
        // If the secure save path is not set, simply call the save function.
        if (mSecureSavePath.empty())
        {
            return saveFunction();
        }

        using namespace AZ::IO;
        FileIOBase* fileIo = FileIOBase::GetInstance();

        // Check if the file already exists, in this case a backup is needed.
        // Backup is needed to make sure we don't lose data when a crash or power failure occurs during saving.
        if (fileIo->Exists(filename))
        {
            // Extract the base filename without extension and the extension without the dot.
            AZStd::string baseFilename;
            AzFramework::StringFunc::Path::GetFileName(filename, baseFilename);
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(filename, extension, false /* include dot */);

            // Find a unique backup filename.
            AZ::u32 backupFileIndex = 0;
            AZStd::string backupFileIndexString;
            AZStd::string backupFilename = mSecureSavePath.c_str() + baseFilename + '.' + extension;
            while (fileIo->Exists(backupFilename.c_str()))
            {
                AZStd::to_string(backupFileIndexString, ++backupFileIndex);

                backupFilename = mSecureSavePath.c_str() +  baseFilename + backupFileIndexString + '.' + extension;
            }

            // Copy the file to the backup filename.
            // Rename is not used to avoid the read-only file state in the secure save path in case the file is in read-only.
            if (fileIo->Copy(filename, backupFilename.c_str()) == ResultCode::Error)
            {
                const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot copy file '<b>%s</b>' to backup file '<b>%s</b>'.", filename, backupFilename.c_str());
                if (commandManager)
                {
                    commandManager->AddError(errorMessage);
                }
                AZ_Error("EMotionFX", false, errorMessage.c_str());
                return false;
            }

            // Set the recover filename.
            AZStd::string recoverFilename = backupFilename;
            recoverFilename += ".recover";

            // Save the recover json file that contains the link to the original filename.
            HandleType fileHandle = InvalidHandle;
            if (fileIo->Open(recoverFilename.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, fileHandle))
            {
                rapidjson::Document json;
                rapidjson::Value& root = json.SetObject();

                rapidjson::Value orgFilenameItem(rapidjson::kStringType);
                orgFilenameItem.SetString(filename, json.GetAllocator());
                root.AddMember("OriginalFileName", orgFilenameItem, json.GetAllocator());

                rapidjson::StringBuffer buffer;
                rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
                json.Accept(writer);

                AZ::u64 bytesWritten = 0;
                if (!fileIo->Write(fileHandle, buffer.GetString(), buffer.GetSize(), &bytesWritten))
                {
                    AZ_Error("EMotionFX", false, "Failed to write recover file: %s", recoverFilename.c_str());
                }

                fileIo->Close(fileHandle);
            }

            // Call the customized save file function.
            if (saveFunction())
            {
                // Remove the backup file.
                if (fileIo->Remove(backupFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot delete backup file '<b>%s</b>'.", backupFilename.c_str());
                    if (commandManager)
                    {
                        commandManager->AddError(errorMessage);
                    }
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                // Remove the recover file.
                if (fileIo->Remove(recoverFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot delete recover file '<b>%s</b>'.", recoverFilename.c_str());
                    if (commandManager)
                    {
                        commandManager->AddError(errorMessage);
                    }
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                // Return true even if the files can not be removed because the save succeeded.
                return true;
            }
            // Saving failed.
            else
            {
                // Remove the partially saved file if the file exists.
                // It's needed because if the file already exists it's not possible to rename.
                if (fileIo->Exists(filename) &&
                    fileIo->Remove(filename) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot delete the partially saved file '<b>%s</b>'.", filename);
                    if (commandManager)
                    {
                        commandManager->AddError(errorMessage);
                    }
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                    return false;
                }

                // Copy the backup file over to where the original file was.
                if (fileIo->Copy(backupFilename.c_str(), filename) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot copy backup file '<b>%s</b>' to '<b>%s</b>'.", backupFilename.c_str(), filename);
                    if (commandManager)
                    {
                        commandManager->AddError(errorMessage);
                    }
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                // Remove the backup file.
                if (fileIo->Remove(backupFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot delete backup file '<b>%s</b>'.", backupFilename.c_str());
                    if (commandManager)
                    {
                        commandManager->AddError(errorMessage);
                    }
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                // Remove the recover file.
                if (fileIo->Remove(recoverFilename.c_str()) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot delete recover file '<b>%s</b>'.", recoverFilename.c_str());
                    if (commandManager)
                    {
                        commandManager->AddError(errorMessage);
                    }
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                return false;
            }
        }
        // If the file doesn't exist.
        else
        {
            // save the file
            if (!saveFunction())
            {
                // Remove the partially saved file if the file exists.
                if (fileIo->Exists(filename) &&
                    fileIo->Remove(filename) == ResultCode::Error)
                {
                    const AZStd::string errorMessage = AZStd::string::format("MCore::FileSystem::SaveToFileSecured() - Cannot delete the partially saved file '<b>%s</b>'.", filename);
                    if (commandManager)
                    {
                        commandManager->AddError(errorMessage);
                    }
                    AZ_Error("EMotionFX", false, errorMessage.c_str());
                }

                return false;
            }

            return true;
        }
    }
} // namespace MCore
