/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace AzToolsFramework
{
    namespace Platform
    {
        static const char ErrorChannel[] = "ArchiveComponent_Linux";

        static const char ZipExePath[] = R"(/usr/bin/zip)";
        static const char UnzipExePath[] = R"(/usr/bin/unzip)";

        static const char CreateArchiveCmd[] = "-r \"%s\" . -i *";

        static const char ExtractArchiveCmd[] = R"(-o "%s" -d "%s")";

        static const char AddFileCmd[] = R"("%s" "%s")";

        static const char ExtractFileCmd[] = R"(%s "%s" %s)";
        static const char ExtractFileDestination[] = R"(%s "%s" "%s" -d "%s")";
        static const char ExtractOverwrite[] = "-o";
        static const char ExtractSkipExisting[] = "-n";
        static const char ListFilesInArchiveCmd[] = "-l %s";

        AZStd::string GetZipExePath()
        {
            return ZipExePath;
        }

        AZStd::string GetUnzipExePath()
        {
            return UnzipExePath;
        }

        AZ::Outcome<AZStd::string, AZStd::string> MakePath(const AZStd::string& path)
        {
            // Create the folder if it does not already exist
            if (!AZ::IO::FileIOBase::GetInstance()->Exists(path.c_str()))
            {
                auto result = AZ::IO::FileIOBase::GetInstance()->CreatePath(path.c_str());
                if (!result)
                {
                    return AZ::Failure(AZStd::string::format("Path creation failed. Input path: %s \n", path.c_str()));
                }
            }

            return AZ::Success(path);
        }

        AZ::Outcome<AZStd::string, AZStd::string> MakeCreateArchivePath(const AZStd::string& archivePath)
        {
            // Remove the file name from the input path
            // /some/folder/path/archive.zip -> /some/folder/path/
            AZStd::string strippedArchivePath = archivePath;
            AzFramework::StringFunc::Path::StripFullName(strippedArchivePath);

            if (strippedArchivePath.empty())
            {
                return AZ::Failure(AZStd::string::format("Stripped path name is empty. Cancelling path creation. Input path: %s\n", archivePath.c_str()));
            }

            return MakePath(strippedArchivePath);
        }

        AZ::Outcome<AZStd::string, AZStd::string> MakeExtractArchivePath(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool includeRoot)
        {
            if(!includeRoot)
            {
                // Create the folder for the input destination path with no modifications
                // /path/to/destination/
                return MakePath(destinationPath);
            }

            // Get the name of the input archive. This will be the name of the root folder for the archive extraction
            // /some/folder/path/archive.zip -> archive
            AZStd::string zipFileName;
            bool result = AzFramework::StringFunc::Path::GetFileName(archivePath.c_str(), zipFileName);
            if(!result)
            {
                return AZ::Failure(AZStd::string::format("Failed to get name of zip file from the archive path. Cancelling path creation. \n Input Archive Path: %s \n", archivePath.c_str()));
            }

            // Append the root folder name to the end of the destination path
            // /path/to/destination/ + archive -> /path/to/destination/archive
            AZStd::string destinationPathWithRoot;
            result = AzFramework::StringFunc::Path::Join(destinationPath.c_str(), zipFileName.c_str(), destinationPathWithRoot);
            if(!result)
            {
                return AZ::Failure(AZStd::string::format("Failed to append zip file name to the destination path. Cancelling path creation. \n Destination Path: %s \n Zip file name: %s \n", destinationPath.c_str(), zipFileName.c_str()));
            }

            // Append a separator so that it is formatted like a folder
            // /path/to/destination/archive -> /path/to/destination/archive/
            AzFramework::StringFunc::Path::AppendSeparator(destinationPathWithRoot);
            return MakePath(destinationPathWithRoot);
        }

        AZStd::string GetCreateArchiveCommand(const AZStd::string& archivePath, const AZStd::string& dirToArchive)
        {
            auto pathCreationResult = MakeCreateArchivePath(archivePath);
            if (!pathCreationResult)
            {
                AZ_Error(ErrorChannel, false, "%s", pathCreationResult.GetError().c_str());
                return "";
            }
            AZ_UNUSED(dirToArchive);
            return AZStd::string::format(CreateArchiveCmd, archivePath.c_str());
        }

        AZStd::string GetExtractArchiveCommand(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool includeRoot)
        {
            auto pathCreationResult = MakeExtractArchivePath(archivePath, destinationPath, includeRoot);
            if (!pathCreationResult)
            {
                AZ_Error(ErrorChannel, false, "%s", pathCreationResult.GetError().c_str());
                return "";
            }

            return AZStd::string::format(ExtractArchiveCmd, archivePath.c_str(), pathCreationResult.GetValue().c_str());
        }

        AZStd::string GetAddFilesToArchiveCommand(const AZStd::string& /*archivePath*/, const AZStd::string& /*listFilePath*/)
        {
            // Adding files into a archive using a list file is not currently supported
            return {};
        }

        bool IsAddFilesToArchiveCommandSupported()
        {
            // Adding files into a archive using a list file is not currently supported
            return false;
        }

        AZStd::string GetAddFileToArchiveCommand(const AZStd::string& archivePath, const AZStd::string& file)
        {
            if (!MakeCreateArchivePath(archivePath).IsSuccess())
            {
                AZ_Error(ErrorChannel, false, "Unable to make path for ( %s ).\n", archivePath.c_str());
                return {};
            }
            return AZStd::string::format(AddFileCmd, archivePath.c_str(), file.c_str());
        }

        AZStd::string GetExtractFileCommand(const AZStd::string& archivePath, const AZStd::string& fileInArchive, const AZStd::string& destinationPath, bool overWrite)
        {
            AZStd::string commandLineArgs;
            if (destinationPath.empty())
            {
                // Extract file in archive from archive path to the current directory, overwriting a file of the same name that exists there.
                commandLineArgs = AZStd::string::format(ExtractFileCmd, overWrite ? ExtractOverwrite : ExtractSkipExisting, archivePath.c_str(), fileInArchive.c_str());
            }
            else
            {
                if (!MakePath(destinationPath).IsSuccess())
                {
                    AZ_Error(ErrorChannel, false, "Unable to make path ( %s ).\n", destinationPath.c_str());
                    return {};
                }
                // Extract file in archive from archive path to destinationPath, overwriting a file of the same name that exists there.
                commandLineArgs = AZStd::string::format(ExtractFileDestination, overWrite ? ExtractOverwrite : ExtractSkipExisting, archivePath.c_str(), fileInArchive.c_str(), destinationPath.c_str());
            }

            return commandLineArgs;
        }

        AZStd::string GetListFilesInArchiveCommand(const AZStd::string& archivePath)
        {
            AZStd::string commandLineArgs = AZStd::string::format(ListFilesInArchiveCmd, archivePath.c_str());
            return commandLineArgs;
        }

        /*
        Sample Console Output of the unzip list command

         Archive:  /var/folders/1q/12nyzqc913qgm532y2c98mnm6w4_qv/T/ArchiveTests-ra8oMy/TestArchive.pak
         Length      Date    Time    Name
         ---------  ---------- -----   ----
                 0  10-14-2019 15:22   testfolder/
                 1  10-14-2019 15:22   testfolder/folderfile.txt
                 1  10-14-2019 15:22   basicfile.txt
                 1  10-14-2019 15:22   basicfile2.txt
                 0  10-14-2019 15:22   testfolder2/
                 1  10-14-2019 15:22   testfolder2/sharedfolderfile2.txt
                 1  10-14-2019 15:22   testfolder2/sharedfolderfile.txt
                 0  10-14-2019 15:22   testfolder3/
                 0  10-14-2019 15:22   testfolder3/testfolder4/
                 1  10-14-2019 15:22   testfolder3/testfolder4/depthfile.bat
         ---------                     -------
                 6                     10 files
         */

        void ParseConsoleOutputFromListFilesInArchive(const AZStd::string& consoleOutput, AZStd::vector<AZStd::string>& fileEntries)
        {
            AZStd::vector<AZStd::string> fileEntryData;
            AzFramework::StringFunc::Tokenize(consoleOutput.c_str(), fileEntryData, "\n");
            int startingLineIdx = 3; // first line that might contain the file name
            for (size_t lineIdx = startingLineIdx; lineIdx < fileEntryData.size(); ++lineIdx)
            {
                AZStd::string& line = fileEntryData[lineIdx];
                AZStd::vector<AZStd::string> lineEntryData;
                AzFramework::StringFunc::Tokenize(line.c_str(), lineEntryData, " ");
                AZStd::string& fileName = lineEntryData.back();

                if(fileName.back() == AZ_CORRECT_FILESYSTEM_SEPARATOR)
                {
                    // if the filename ends with a separator
                    // than it indicates that this is a directory
                    continue;
                }

                if(fileName.compare("-------") == 0)
                {
                    return;
                }

                fileEntries.emplace_back(fileName);
            }
        }
    } // namespace Platform
} // namespace AzToolsFramework
