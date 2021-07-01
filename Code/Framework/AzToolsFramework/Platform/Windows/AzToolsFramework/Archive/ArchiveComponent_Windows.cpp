/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Archive/ArchiveComponent.h>

namespace AzToolsFramework
{
    namespace Platform
    {
        const char CreateArchiveCmd[] = R"(a -tzip -mx=1 "%s" -r "%s\*")";

        // -aos is for skipping extract on existing files
        const char ExtractArchiveCmd[] = R"(x -mmt=off "%s" -o"%s\*" -aos)";
        const char ExtractArchiveWithoutRootCmd[] = R"(x -mmt=off "%s" -o"%s" -aos)";
        const char AddFilesCmd[] = R"(a -tzip "%s" @"%s")";
        const char AddFileCmd[] = R"(a -tzip "%s" "%s")";
        const char ExtractFileCmd[] = R"(e -mmt=off "%s" "%s" %s)";
        const char ExtractFileDestination[] = R"(e -mmt=off "%s" -o"%s" "%s" %s)";
        const char ExtractOverwrite[] = "-aoa";
        const char ExtractSkipExisting[] = "-aos";
        const char ListFilesInArchiveCmd[] = R"(l -r -slt "%s")";

        AZStd::string Get7zExePath()
        {
            const char* rootPath = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(rootPath, &AZ::ComponentApplicationRequests::GetEngineRoot);
            AZStd::string exePath;
            AzFramework::StringFunc::Path::ConstructFull(rootPath, "Tools", "7za", ".exe", exePath);
            return exePath;
        }

        AZStd::string GetZipExePath()
        {
            return Get7zExePath();
        }

        AZStd::string GetUnzipExePath()
        {
            return Get7zExePath();
        }

        AZStd::string GetCreateArchiveCommand(const AZStd::string& archivePath, const AZStd::string& dirToArchive)
        {
            return AZStd::string::format(CreateArchiveCmd, archivePath.c_str(), dirToArchive.c_str());
        }

        AZStd::string GetExtractArchiveCommand(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool includeRoot)
        {
            if (includeRoot)
            {
                // Extract archive path to destinationPath\<archiveFileName> and skipping extracting of existing files
                return AZStd::string::format(ExtractArchiveCmd, archivePath.c_str(), destinationPath.c_str());
            }
            else
            {
                // Extract archive path to destinationPath and skipping extracting of existing files
                return AZStd::string::format(ExtractArchiveWithoutRootCmd, archivePath.c_str(), destinationPath.c_str());
            }
        }

        AZStd::string GetAddFilesToArchiveCommand(const AZStd::string& archivePath, const AZStd::string& listFilePath)
        {
            return AZStd::string::format(AddFilesCmd, archivePath.c_str(), listFilePath.c_str());
        }

        AZStd::string GetAddFileToArchiveCommand(const AZStd::string& archivePath, const AZStd::string& file)
        {
            return AZStd::string::format(AddFileCmd, archivePath.c_str(), file.c_str());
        }

        AZStd::string GetExtractFileCommand(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite)
        {
            AZStd::string commandLineArgs;
            if (destinationPath.empty())
            {
                // Extract file in archive from archive path to the current directory, overwriting a file of the same name that exists there.
                commandLineArgs = AZStd::string::format(ExtractFileCmd, archivePath.c_str(), fileInArchive.c_str(), overWrite ? ExtractOverwrite : ExtractSkipExisting);
            }
            else
            {
                // Extract file in archive from archive path to destinationPath, overwriting a file of the same name that exists there.
                commandLineArgs = AZStd::string::format(ExtractFileDestination, archivePath.c_str(), destinationPath.c_str(), fileInArchive.c_str(), overWrite ? ExtractOverwrite : ExtractSkipExisting);
            }

            return commandLineArgs;
        }

        AZStd::string GetListFilesInArchiveCommand(const AZStd::string& archivePath)
        {
            AZStd::string commandLineArgs = AZStd::string::format(ListFilesInArchiveCmd, archivePath.c_str());
            return commandLineArgs;
        }

    /*
    File output for our list archive commands takes the following two patterns for files vs directories:

    Path = basicfile2.txt
    Folder = -
    Size = 1
    Packed Size = 1
    Modified = 2019-03-26 18:31:10
    Created = 2019-03-26 18:31:10
    Accessed = 2019-03-26 18:31:10
    Attributes = A
    Encrypted = -
    Comment =
    CRC = 32D70693
    Method = Store
    Characteristics = NTFS
    Host OS = FAT
    Version = 10
    Volume Index = 0
    Offset = 44

    Path = testfolder
    Folder = +
    Size = 0
    Packed Size = 0
    Modified = 2019-03-26 18:31:10
    Created = 2019-03-26 18:31:10
    Accessed = 2019-03-26 18:31:10
    Attributes = D
    Encrypted = -
    Comment =
    CRC =
    Method = Store
    Characteristics = NTFS
    Host OS = FAT
    Version = 20
    Volume Index = 0
    Offset = 89

    */
        
        void ParseConsoleOutputFromListFilesInArchive(const AZStd::string& consoleOutput, AZStd::vector<AZStd::string>& fileEntries)
        {
            AZStd::vector<AZStd::string> fileEntryData;
            AzFramework::StringFunc::Tokenize(consoleOutput.c_str(), fileEntryData, "\r\n");
            for (size_t slotNum = 0; slotNum < fileEntryData.size(); ++slotNum)
            {
                AZStd::string& line = fileEntryData[slotNum];
                if (AzFramework::StringFunc::StartsWith(line, "Path = "))
                {
                    if ((slotNum + 1) < fileEntryData.size())
                    {
                        // We're checking one past each entry we find for the Folder entry and skipping anything marked as a folder
                        // See sample output above
                        if (AzFramework::StringFunc::StartsWith(fileEntryData[slotNum + 1], "Folder = -"))
                        {
                            AzFramework::StringFunc::Replace(line, "Path = ", "", false, true);
                            fileEntries.emplace_back(AZStd::move(line));
                            slotNum++;
                        }
                    }
                }
            }
        }
    } // namespace Platform
} // namespace AzToolsFramework
